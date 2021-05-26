#ifndef _MUGIQ_DISPLACE_KERNELS_CUH
#define _MUGIQ_DISPLACE_KERNELS_CUH

#include <contract_util.cuh>

namespace mugiq {

using namespace quda;

template <typename Float, typename Arg, QudaFieldOrder order>
__global__ void covariantDisplacementVector_kernel(Arg *arg, DisplaceDir dispDir, DisplaceSign dispSign);

} // namespace muqiq

#endif // _MUGIQ_DISPLACE_KERNELS_CUH
