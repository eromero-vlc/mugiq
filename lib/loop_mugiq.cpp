#include <loop_mugiq.h>
#include <cublas_v2.h>
#include <mpi.h>

template <typename Float>
Loop_Mugiq<Float>::Loop_Mugiq(MugiqLoopParam *loopParams_,
			      Eigsolve_Mugiq *eigsolve_) :
  cPrm(nullptr),
  dSt(nullptr),
  eigsolve(eigsolve_),
  dataPos_d(nullptr),
  dataPos_h(nullptr),
  dataMom_d(nullptr),
  dataMom_h(nullptr),
  dataMom_gs(nullptr),
  dataMom(nullptr),
  nElemMomTot(0),
  nElemMomLoc(0)
{
  printfQuda("\n*************************************************\n");
  printfQuda("%s: Creating Loop computation environment\n", __func__);
  
  cPrm = new LoopComputeParam(loopParams_, eigsolve->mg_env->mg_solver->B[0]);
  if(cPrm->doNonLocal) dSt = new LoopDispState<Float>(loopParams_);

  allocateDataMemory();
  copyGammaToConstMem();
  if(cPrm->doMomProj) createPhaseMatrix();
  
  printfQuda("*************************************************\n\n");

  printLoopComputeParams();
}

  
template <typename Float>
Loop_Mugiq<Float>::~Loop_Mugiq(){

  freeDataMemory();

  if(cPrm->doNonLocal) delete dSt;
  delete cPrm;
}


template <typename Float>
void Loop_Mugiq<Float>::allocateDataMemory(){

  nElemMomTot = cPrm->Ndata * cPrm->Nmom * cPrm->totT;
  nElemMomLoc = cPrm->Ndata * cPrm->Nmom * cPrm->locT;
  nElemPosLoc = cPrm->Ndata * cPrm->locV4;
  nElemPhMat  = cPrm->Nmom  * cPrm->locV3;
   
  if(cPrm->doMomProj){
    //- Allocate host data buffers
    dataMom    = static_cast<complex<Float>*>(calloc(nElemMomTot, SizeCplxFloat));
    dataMom_gs = static_cast<complex<Float>*>(calloc(nElemMomLoc, SizeCplxFloat));
    dataMom_h  = static_cast<complex<Float>*>(calloc(nElemMomLoc, SizeCplxFloat));
    
    if(dataMom    == NULL) errorQuda("%s: Could not allocate buffer: dataMom\n", __func__);
    if(dataMom_gs == NULL) errorQuda("%s: Could not allocate buffer: dataMom_gs\n", __func__);
    if(dataMom_h  == NULL) errorQuda("%s: Could not allocate buffer: dataMom_h\n", __func__);
  }
  else{
    dataPos_h = static_cast<complex<Float>*>(calloc(nElemPosLoc, SizeCplxFloat));
    if(dataPos_h  == NULL) errorQuda("%s: Could not allocate buffer: dataPos_h\n", __func__);
  }
  
  printfQuda("%s: Host buffers allocated\n", __func__);
  //------------------------------
  
  //- Allocate device data buffers

  //- That's the device loop-trace data buffer, always needed!
  cudaMalloc((void**)&(dataPos_d), SizeCplxFloat*nElemPosLoc);
  checkCudaError();
  cudaMemset(dataPos_d, 0, SizeCplxFloat*nElemPosLoc);

  if(cPrm->doMomProj){
    cudaMalloc( (void**)&(phaseMatrix_d), SizeCplxFloat*nElemPhMat);
    checkCudaError();
    cudaMemset(phaseMatrix_d, 0, SizeCplxFloat*nElemPhMat);
    
    cudaMalloc((void**)&(dataMom_d), SizeCplxFloat*nElemMomLoc);
    checkCudaError();
    cudaMemset(dataMom_d, 0, SizeCplxFloat*nElemMomLoc);

    cudaMalloc((void**)&(dataPosMP_d), SizeCplxFloat*nElemPosLoc);
    checkCudaError();
    cudaMemset(dataPosMP_d, 0, SizeCplxFloat*nElemPosLoc);
    
  }
  
  printfQuda("%s: Device buffers allocated\n", __func__);
  //------------------------------
  
}


// That's just a wrapper to copy the Gamma-matrix coefficient structure to constant memory
template <typename Float>
void Loop_Mugiq<Float>::copyGammaToConstMem(){
  copyGammaCoeffStructToSymbol<Float>();
  printfQuda("%s: Gamma coefficient structure copied to constant memory\n", __func__);
}


// Wrapper to create the Phase Matrix on GPU
template <typename Float>
void Loop_Mugiq<Float>::createPhaseMatrix(){
  createPhaseMatrixGPU<Float>(phaseMatrix_d, cPrm->momMatrix,
  			      cPrm->locV3, cPrm->Nmom, (int)cPrm->FTSign,
			      cPrm->localL, cPrm->totalL);
  
  printfQuda("%s: Phase matrix created\n", __func__);
}


template <typename Float>
void Loop_Mugiq<Float>::freeDataMemory(){

  if(dataMom){
    free(dataMom);
    dataMom = nullptr;
  }
  if(dataMom_gs){
    free(dataMom_gs);
    dataMom_gs = nullptr;
  }
  if(dataMom_h){
    free(dataMom_h);
    dataMom_h = nullptr;
  }
  if(dataPos_h){
    free(dataPos_h);
    dataPos_h = nullptr;
  }  
  printfQuda("%s: Host buffers freed\n", __func__);
  //------------------------------

  if(dataPos_d){
    cudaFree(dataPos_d);
    dataPos_d = nullptr;
  }
  if(dataPosMP_d){
    cudaFree(dataPosMP_d);
    dataPosMP_d = nullptr;
  }
  if(dataMom_d){
    cudaFree(dataMom_d);
    dataMom_d = nullptr;
  }

  if(phaseMatrix_d){
    cudaFree(phaseMatrix_d);
    phaseMatrix_d = nullptr;
  }
  printfQuda("%s: Device buffers freed\n", __func__);
  //------------------------------

}


template <typename Float>
void Loop_Mugiq<Float>::printLoopComputeParams(){

  
  printfQuda("******************************************\n");
  printfQuda("    Parameters of the Loop Computation\n");
  printfQuda("Will%s perform Momentum Projection (Fourier Transform)\n", cPrm->doMomProj ? "" : " NOT");
  if(cPrm->doMomProj){
    printfQuda("Momentum Projection will be performed on GPU using cuBlas\n");
    printfQuda("Number of momenta: %d\n", cPrm->Nmom);
    printfQuda("Fourier transform Exp. Sign: %d\n", (int) cPrm->FTSign);
  }
  printfQuda("Will%s perform loop on non-local currents\n", cPrm->doNonLocal ? "" : " NOT");
  if(cPrm->doNonLocal){
    printfQuda("Non-local path string: %s\n", cPrm->pathString);
    printfQuda("Non-local path length: %d\n", cPrm->pathLen);
  }    
  printfQuda("Local  lattice size (x,y,z,t): %d %d %d %d \n", cPrm->localL[0], cPrm->localL[1], cPrm->localL[2], cPrm->localL[3]);
  printfQuda("Global lattice size (x,y,z,t): %d %d %d %d \n", cPrm->totalL[0], cPrm->totalL[1], cPrm->totalL[2], cPrm->totalL[3]);
  printfQuda("Global time extent: %d\n", cPrm->totT);
  printfQuda("Local  time extent: %d\n", cPrm->locT);
  printfQuda("Local  volume: %lld\n", cPrm->locV4);
  printfQuda("Local  3d volume: %lld\n", cPrm->locV3);
  printfQuda("Global 3d volume: %lld\n", cPrm->totV3);
  printfQuda("Transverse shift max. depth (not applicable now): %d\n", cPrm->max_depth);
  printfQuda("******************************************\n");
  
}


template <typename Float>
void Loop_Mugiq<Float>::printData_ASCII(){

  for(int im=0;im<cPrm->Nmom;im++){
    for(int id=0;id<cPrm->Ndata;id++){
      printfQuda("Loop for momentum (%+d,%+d,%+d), Gamma[%d]:\n",
		 cPrm->momMatrix[MOM_MATRIX_IDX(0,im)],
		 cPrm->momMatrix[MOM_MATRIX_IDX(1,im)],
		 cPrm->momMatrix[MOM_MATRIX_IDX(2,im)], id);
      for(int it=0;it<cPrm->totT;it++){
	//- FIXME: Check if loop Index is correct
	int loopIdx = id + cPrm->Ndata*it + cPrm->Ndata*cPrm->totT*im;
	printfQuda("%d %+.8e %+.8e\n", it, dataMom[loopIdx].real(), dataMom[loopIdx].imag());
      }
    }
  }
  
}



template <typename Float>
void Loop_Mugiq<Float>::prolongateEvec(ColorSpinorField *fineEvec, ColorSpinorField *coarseEvec){

  MG_Mugiq &mg_env = *(eigsolve->getMGEnv());
  
  //- Create one fine and N_coarse temporary coarse fields
  //- Will be used for prolongating the coarse eigenvectors back to the fine lattice
  std::vector<ColorSpinorField *> tmpCSF;
  ColorSpinorParam csParam(*(mg_env.mg_solver->B[0]));
  QudaPrecision coarsePrec = coarseEvec->Precision();
  csParam.create = QUDA_ZERO_FIELD_CREATE;
  csParam.setPrecision(coarsePrec);
  
  tmpCSF.push_back(ColorSpinorField::Create(csParam)); //- tmpCSF[0] is a fine field
  for(int lev=0;lev<mg_env.nCoarseLevels;lev++){
    tmpCSF.push_back(tmpCSF[lev]->CreateCoarse(mg_env.mgParams->geo_block_size[lev],
                                               mg_env.mgParams->spin_block_size[lev],
                                               mg_env.mgParams->n_vec[lev],
                                               coarsePrec,
                                               mg_env.mgParams->setup_location[lev+1]));
  }//-lev
  
  //- Prolongate the coarse eigenvectors recursively to get
  //- to the finest level  
  *(tmpCSF[mg_env.nCoarseLevels]) = *coarseEvec;
  for(int lev=mg_env.nCoarseLevels;lev>1;lev--){
    blas::zero(*tmpCSF[lev-1]);
    if(!mg_env.transfer[lev-1]) errorQuda("%s: Transfer operator for level %d does not exist!\n", __func__, lev);
    mg_env.transfer[lev-1]->P(*(tmpCSF[lev-1]), *(tmpCSF[lev]));
  }
  blas::zero(*fineEvec);
  if(!mg_env.transfer[0]) errorQuda("%s: Transfer operator for finest level does not exist!\n", __func__);
  mg_env.transfer[0]->P(*fineEvec, *(tmpCSF[1]));

  for(int i=0;i<static_cast<int>(tmpCSF.size());i++) delete tmpCSF[i];
  
}


template <typename Float>
void Loop_Mugiq<Float>::performMomentumProjection(){

  const long long locV3 = cPrm->locV3;
  const int locT  = cPrm->locT;
  const int totT  = cPrm->totT;
  const int Nmom  = cPrm->Nmom;
  const int Ndata = cPrm->Ndata;

  convertIdxOrderToMomProj<Float>(dataPosMP_d, dataPos_d, cPrm->nParity, cPrm->localL);
  
  /** Perform momentum projection
   *-----------------------------
   * Matrix Multiplication Out = PH^T * In.
   *  phaseMatrix_dev=(locV3,Nmom) is the phase matrix in column-major format, its transpose is used for multiplication
   *  dataPosMP_d = (locV3,NGamma*locT) is the device input loop-trace matrix with shuffled(converted) indices
   *  dataMom_d = (Nmom,NGamma*locT) is the output matrix in column-major format (device)
   */  
    
  cublasHandle_t handle;
  cublasStatus_t stat = cublasCreate(&handle);
  complex<Float> al = complex<Float>{1.0,0.0};
  complex<Float> be = complex<Float>{0.0,0.0};

  if(typeid(Float) == typeid(double)){
    stat = cublasZgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, Nmom, Ndata*locT, locV3,
                       (cuDoubleComplex*)&al, (cuDoubleComplex*)phaseMatrix_d, locV3,
                       (cuDoubleComplex*)dataPosMP_d, locV3,
		       (cuDoubleComplex*)&be,
                       (cuDoubleComplex*)dataMom_d, Nmom);
  }
  else if(typeid(Float) == typeid(float)){
    stat = cublasCgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, Nmom, Ndata*locT, locV3,
                       (cuComplex*)&al, (cuComplex*)phaseMatrix_d, locV3,
                       (cuComplex*)dataPosMP_d, locV3,
		       (cuComplex*)&be,
                       (cuComplex*)dataMom_d, Nmom);
  }
  else errorQuda("%s: Precision not supported!\n", __func__);

  if(stat != CUBLAS_STATUS_SUCCESS)
    errorQuda("%s: Momentum projection failed!\n", __func__);  

  
  //-- extract the result from GPU to CPU
  stat = cublasGetMatrix(Nmom, Ndata*locT, sizeof(complex<Float>), dataMom_d, Nmom, dataMom_h, Nmom);
  if(stat != CUBLAS_STATUS_SUCCESS) errorQuda("%s: cuBlas data copying to CPU failed!\n", __func__);
  // ---------------------------------------------------------------------------------------

  
  /** Perform reduction over all processes
   * -------------------------------------
   * Create separate communicators
   * All processes with the same comm_coord(3) belong to COMM_SPACE communicator.
   * When performing the reduction over the COMM_SPACE communicator, the global sum
   * will be performed across all processes with the same time-coordinate,
   * and the result will be placed at the "root" of each of the "time" groups.
   * This means that the global result will exist only at the "time" processes, where each will
   * hold the sum for its corresponing time slices.
   * (In the case where only the time-direction is partitioned, MPI_Reduce is essentially a memcpy).
   *
   * Then a Gathering is required, in order to put the global result from each of the "time" processes
   * into the final buffer (dataMom). This gathering must take place only across the "time" processes,
   * therefore another communicator involving only these processes must be created (COMM_TIME).
   * Finally, we need to Broadcast the final result to ALL processes, such that it is accessible to all of them.
   *
   * The final buffer follows order Mom-inside-Gamma-inside-T: im + Nmom*ig + Nmom*Ndata*t
   */

  MPI_Datatype dataTypeMPI;
  if     ( typeid(Float) == typeid(float) ) dataTypeMPI = MPI_COMPLEX;
  else if( typeid(Float) == typeid(double)) dataTypeMPI = MPI_DOUBLE_COMPLEX;

  //-- Create space-communicator
  int space_rank, space_size;
  MPI_Comm COMM_SPACE;
  int tCoord = comm_coord(3);
  int cRank = comm_rank();
  MPI_Comm_split(MPI_COMM_WORLD, tCoord, cRank, &COMM_SPACE);
  MPI_Comm_rank(COMM_SPACE,&space_rank);
  MPI_Comm_size(COMM_SPACE,&space_size);

  //-- Create time communicator
  int time_rank, time_size;
  int time_tag = 1000;
  MPI_Comm COMM_TIME;
  int time_color = comm_rank();   //-- Determine the "color" which distinguishes the "time" processes from the rest
  if( (comm_coord(0) == 0) &&
      (comm_coord(1) == 0) &&
      (comm_coord(2) == 0) ) time_color = (time_tag>comm_size()) ? time_tag : time_tag+comm_size();

  MPI_Comm_split(MPI_COMM_WORLD, time_color, tCoord, &COMM_TIME);
  MPI_Comm_rank(COMM_TIME,&time_rank);
  MPI_Comm_size(COMM_TIME,&time_size);

  
  MPI_Reduce(dataMom_h, dataMom_gs, Nmom*Ndata*locT, dataTypeMPI, MPI_SUM, 0, COMM_SPACE);

  
  MPI_Gather(dataMom_gs, Nmom*Ndata*locT, dataTypeMPI,
             dataMom   , Nmom*Ndata*locT, dataTypeMPI,
             0, COMM_TIME);

  
  MPI_Bcast(dataMom, Nmom*Ndata*totT, dataTypeMPI, 0, MPI_COMM_WORLD);

  
  //-- cleanup & return
  MPI_Comm_free(&COMM_SPACE);
  MPI_Comm_free(&COMM_TIME);

  cublasDestroy(handle);

}


//- This is the function that actually performs the trace
//- It's a public function, and it's called from the interface
template <typename Float>
void Loop_Mugiq<Float>::computeCoarseLoop(){

  int nEv = eigsolve->eigParams->nEv; // Number of eigenvectors

  //- Create a fine field, this will hold the prolongated version of each eigenvector
  ColorSpinorParam csParam(*(eigsolve->mg_env->mg_solver->B[0]));
  QudaPrecision coarsePrec = eigsolve->eVecs[0]->Precision();
  csParam.create = QUDA_ZERO_FIELD_CREATE;
  csParam.setPrecision(coarsePrec);
  ColorSpinorField *fineEvecL = ColorSpinorField::Create(csParam);
  ColorSpinorField *fineEvecR = ColorSpinorField::Create(csParam);

  cudaMemset(dataPos_d, 0, SizeCplxFloat*nElemPosLoc);
  for(int n=0;n<nEv;n++){
    Float sigma = (Float)(*(eigsolve->eVals_sigma))[n];
    printfQuda("**************** %+.16e\n", sigma);

    if(eigsolve->computeCoarse) prolongateEvec(fineEvecL, eigsolve->eVecs[n]);
    else *fineEvecL = *(eigsolve->eVecs[n]);

    if(!cPrm->doNonLocal) *fineEvecR = *fineEvecL;
    else{
      //-Perform Shifts
      //- TODO: Probably not as simple as that, but not much more complicated either
      //      dSt->performShift(fineEvecR, fineEvecL);
      //- For now, just set them equal
      *fineEvecR = *fineEvecL;
    }

    performLoopContraction<Float>(dataPos_d, fineEvecL, fineEvecR, sigma);
    printfQuda("%s: Loop trace for eigenvector %d completed\n", __func__, n);
  } //- Eigenvectors

  
  if(cPrm->doMomProj){
    performMomentumProjection();
    printfQuda("%s: Momentum projection completed\n", __func__);
  }
  else{
    cudaMemcpy(dataPos_h, dataPos_d, SizeCplxFloat*nElemPosLoc, cudaMemcpyDeviceToHost);
    cudaDeviceSynchronize();
    checkCudaError();
  }
  
  delete fineEvecL;
  delete fineEvecR;
  
}


//- Explicit instantiation of the templates of the Loop_Mugiq class
//- float and double will be the only typename templates that support is required,
//- so this is a 'feature' rather than a 'bug'
template class Loop_Mugiq<float>;
template class Loop_Mugiq<double>;