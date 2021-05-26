#ifndef _INTERFACE_MUGIQ_H
#define _INTERFACE_MUGIQ_H

#include <mugiq.h>

namespace mugiq {

using namespace quda;

//- Forward declarations of QUDA-interface functions not declared in MuGiQ's .h files
quda::cudaGaugeField *checkGauge(QudaInvertParam *param);

} // namespace muqiq

#endif // _INTERFACE_MUGIQ_H
