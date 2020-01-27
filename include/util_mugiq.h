#ifndef _MUGIQ_UTIL_H
#define _MUGIQ_UTIL_H

#include <color_spinor_field.h>
#include <eigsolve_mugiq.h>

#define N_SPIN_ 4
#define N_COLOR_ 3
#define N_GAMMA_ 16
#define SPINOR_SITE_LEN_ (N_SPIN_ * N_COLOR_)
#define GAUGE_SITE_LEN_ (N_COLOR_ * N_COLOR_)
#define GAMMA_LEN_ (N_SPIN_ * N_SPIN_)

#define GAMMA_GEN_IDX(s,c)  ((c) + N_COLOR_*(s))

#define MUGIQ_MAX_FINE_VEC 24
#define MUGIQ_MAX_COARSE_VEC 256

#define THREADS_PER_BLOCK 64

using namespace quda;

//- Forward declarations of QUDA-interface functions not declared in the .h files, and are called here
quda::cudaGaugeField *checkGauge(QudaInvertParam *param);


//- Forward declarations of functions called from the interface functions
void assembleLoopCoarsePart(Eigsolve_Mugiq *eigsolve);

#endif // _MUGIQ_UTIL_H
