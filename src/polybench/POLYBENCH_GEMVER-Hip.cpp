//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-19, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~// 

#include "POLYBENCH_GEMVER.hpp"

#include "RAJA/RAJA.hpp"

#if defined(RAJA_ENABLE_HIP)

#include "common/HipDataUtils.hpp"

#include <iostream>

namespace rajaperf 
{
namespace polybench
{

//
// Define thread block size for HIP execution
//
const size_t block_size = 256;

#define POLYBENCH_GEMVER_DATA_SETUP_HIP \
  allocAndInitHipDeviceData(A, m_A, m_n * m_n); \
  allocAndInitHipDeviceData(u1, m_u1, m_n); \
  allocAndInitHipDeviceData(v1, m_v1, m_n); \
  allocAndInitHipDeviceData(u2, m_u2, m_n); \
  allocAndInitHipDeviceData(v2, m_v2, m_n); \
  allocAndInitHipDeviceData(w, m_w, m_n); \
  allocAndInitHipDeviceData(x, m_x, m_n); \
  allocAndInitHipDeviceData(y, m_y, m_n); \
  allocAndInitHipDeviceData(z, m_z, m_n); 


#define POLYBENCH_GEMVER_TEARDOWN_HIP \
  getHipDeviceData(m_w, w, m_n); \
  deallocHipDeviceData(A); \
  deallocHipDeviceData(u1); \
  deallocHipDeviceData(v1); \
  deallocHipDeviceData(u2); \
  deallocHipDeviceData(v2); \
  deallocHipDeviceData(w); \
  deallocHipDeviceData(x); \
  deallocHipDeviceData(y); \
  deallocHipDeviceData(z); 

__global__ void poly_gemmver_1(Real_ptr A, 
                               Real_ptr u1, Real_ptr v1, 
                               Real_ptr u2, Real_ptr v2,
                               Index_type n)
{
  Index_type i = blockIdx.y;
  Index_type j = threadIdx.x; 

  POLYBENCH_GEMVER_BODY1;
}

__global__ void poly_gemmver_2(Real_ptr A, 
                               Real_ptr x, Real_ptr y,
                               Real_type beta, 
                               Index_type n)
{
  Index_type i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < n) { 
    POLYBENCH_GEMVER_BODY2;
    for (Index_type j = 0; j < n; ++j) {
      POLYBENCH_GEMVER_BODY3;
    }
    POLYBENCH_GEMVER_BODY4;
  }
}

__global__ void poly_gemmver_3(Real_ptr x, Real_ptr z,
                               Index_type n)
{
  Index_type i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < n) {
    POLYBENCH_GEMVER_BODY5;
  }
}

__global__ void poly_gemmver_4(Real_ptr A,
                               Real_ptr x, Real_ptr w,
                               Real_type alpha,
                               Index_type n)
{
  Index_type i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < n) { 
    POLYBENCH_GEMVER_BODY6;
    for (Index_type j = 0; j < n; ++j) {
      POLYBENCH_GEMVER_BODY7;
    }
    POLYBENCH_GEMVER_BODY8;
  }
}


void POLYBENCH_GEMVER::runHipVariant(VariantID vid)
{
  const Index_type run_reps = getRunReps();
  
  POLYBENCH_GEMVER_DATA_SETUP;

  if ( vid == Base_HIP ) {

    POLYBENCH_GEMVER_DATA_SETUP_HIP;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      dim3 nblocks(1, n, 1);
      dim3 nthreads_per_block(n, 1, 1);
      hipLaunchKernelGGL((poly_gemmver_1) , dim3(nblocks), dim3(nthreads_per_block), 0, 0, 
                                A, u1, v1, u2, v2, n);

      size_t grid_size = RAJA_DIVIDE_CEILING_INT(m_n, block_size); 

      hipLaunchKernelGGL((poly_gemmver_2), dim3(grid_size), dim3(block_size), 0, 0,
                                                A, x, y,
                                                beta,
                                                n);

      hipLaunchKernelGGL((poly_gemmver_3), dim3(grid_size), dim3(block_size), 0, 0,
                                                x, z,
                                                n); 

      hipLaunchKernelGGL((poly_gemmver_4), dim3(grid_size), dim3(block_size), 0, 0,
                                                A, x, w,
                                                alpha,
                                                n);
    }
    stopTimer();

    POLYBENCH_GEMVER_TEARDOWN_HIP;

  } else if (vid == RAJA_HIP) {

    POLYBENCH_GEMVER_DATA_SETUP_HIP;

    POLYBENCH_GEMVER_VIEWS_RAJA;

    using EXEC_POL1 =
      RAJA::KernelPolicy<
        RAJA::statement::HipKernelAsync<
          RAJA::statement::For<0, RAJA::hip_block_y_loop,
            RAJA::statement::For<1, RAJA::hip_thread_x_loop,
              RAJA::statement::Lambda<0>
            >
          >
        >
      >;

    using EXEC_POL24 = 
      RAJA::KernelPolicy<
        RAJA::statement::HipKernelAsync<
          RAJA::statement::Tile<0, RAJA::statement::tile_fixed<block_size>, 
                                   RAJA::hip_block_x_loop,
            RAJA::statement::For<0, RAJA::hip_thread_x_direct,
              RAJA::statement::Lambda<0>,
              RAJA::statement::For<1, RAJA::seq_exec,
                RAJA::statement::Lambda<1>
              >,
              RAJA::statement::Lambda<2>
            >
          >
        >
      >;
 
    using EXEC_POL3 = RAJA::hip_exec<block_size, true /*async*/>;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      RAJA::kernel<EXEC_POL1>( RAJA::make_tuple(RAJA::RangeSegment{0, n},
                                                RAJA::RangeSegment{0, n}),
        [=] __device__ (Index_type i, Index_type j) {
          POLYBENCH_GEMVER_BODY1_RAJA;
        }
      );

      RAJA::kernel_param<EXEC_POL24>(
        RAJA::make_tuple(RAJA::RangeSegment{0, n},
                         RAJA::RangeSegment{0, n}),
        RAJA::make_tuple(static_cast<Real_type>(0.0)),

        [=] __device__ (Index_type /* i */, Index_type /* j */, Real_type &dot) {
          POLYBENCH_GEMVER_BODY2_RAJA;
        },
        [=] __device__ (Index_type i, Index_type j, Real_type &dot) {
          POLYBENCH_GEMVER_BODY3_RAJA;
        },
        [=] __device__ (Index_type i, Index_type /* j */, Real_type &dot) {
          POLYBENCH_GEMVER_BODY4_RAJA;
        }
      );

      RAJA::forall<EXEC_POL3> (RAJA::RangeSegment{0, n},
        [=] __device__ (Index_type i) {
          POLYBENCH_GEMVER_BODY5_RAJA;
        }
      );

      RAJA::kernel_param<EXEC_POL24>(
        RAJA::make_tuple(RAJA::RangeSegment{0, n},
                         RAJA::RangeSegment{0, n}),
        RAJA::make_tuple(static_cast<Real_type>(0.0)),

        [=] __device__ (Index_type i, Index_type /* j */, Real_type &dot) {
          POLYBENCH_GEMVER_BODY6_RAJA;
        },
        [=] __device__ (Index_type i, Index_type j, Real_type &dot) {
          POLYBENCH_GEMVER_BODY7_RAJA;
        },
        [=] __device__ (Index_type i, Index_type /* j */, Real_type &dot) {
          POLYBENCH_GEMVER_BODY8_RAJA;
        }
      );
      
    }
    stopTimer();

    POLYBENCH_GEMVER_TEARDOWN_HIP;

  } else {
      std::cout << "\n  POLYBENCH_GEMVER : Unknown Hip variant id = " << vid << std::endl;
  }

}

} // end namespace polybench
} // end namespace rajaperf

#endif  // RAJA_ENABLE_HIP
  
