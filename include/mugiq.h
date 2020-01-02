#ifndef _MUGIQ_H
#define _MUGIQ_H

/** 
 * @file mugiq.h
 * @brief Main header file for the MUGIQ library
 *
 */

#include <quda.h>
#include <enum_mugiq.h>

#ifdef __cplusplus
extern "C" {
#endif
  
  /** Wrapper function that calls the QUDA eigensolver to compute eigenvectors and eigenvalues
   * @param h_evecs  Array of pointers to application eigenvectors
   * @param h_evals  Host side eigenvalues
   * @param param Contains all metadata regarding the type of solve.
   */
  void computeEvecsQudaWrapper(void **eVecs_host, double _Complex *eVals_host, QudaEigParam *eigParams);

  
  /** MuGiq interface function that computes eigenvectors and eigenvalues
   * @param eigParams Contains all metadata regarding the type of solve.
   */
  void computeEvecsMuGiq(QudaEigParam *eigParams);

  /** MuGiq interface function that computes eigenvectors and eigenvalues of coarse operators using MG
   * @param mgParams Contains all MG metadata regarding the type of eigensolve.
   */
  void computeEvecsMuGiq_MG(QudaMultigridParam mgParams);


  
#ifdef __cplusplus
}
#endif


#endif // _MUGIQ_H
