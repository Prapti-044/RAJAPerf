//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-20, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "MAT_MAT_SHARED.hpp"

#include <iostream>

namespace rajaperf {
namespace basic {

void MAT_MAT_SHARED::runSeqVariant(VariantID vid) {

  const Index_type run_reps = getRunReps();
  const Index_type N = m_N;

  MAT_MAT_SHARED_DATA_SETUP;
  const Index_type Nx = RAJA_DIVIDE_CEILING_INT(N, TL_SZ);
  const Index_type Ny = RAJA_DIVIDE_CEILING_INT(N, TL_SZ);

  switch (vid) {

  case Base_Seq: {

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      for (Index_type by = 0; by < Ny; ++by) {
        for (Index_type bx = 0; bx < Nx; ++bx) {

          //Work around for when compiling with CLANG and HIP
          //See notes in MAT_MAT_SHARED.hpp
          MAT_MAT_SHARED_BODY_0_CLANG_HIP_CPU

          for (Index_type ty = 0; ty < TL_SZ; ++ty) {
            for (Index_type tx = 0; tx < TL_SZ; ++tx) {
              MAT_MAT_SHARED_BODY_1
            }
          }

          for (Index_type k = 0; k < (TL_SZ + N - 1) / TL_SZ; ++k) {

            for (Index_type ty = 0; ty < TL_SZ; ++ty) {
              for (Index_type tx = 0; tx < TL_SZ; ++tx) {
                MAT_MAT_SHARED_BODY_2
              }
            }

            for (Index_type ty = 0; ty < TL_SZ; ++ty) {
              for (Index_type tx = 0; tx < TL_SZ; ++tx) {
                MAT_MAT_SHARED_BODY_3
              }
            }

          } // Sequential loop

          for (Index_type ty = 0; ty < TL_SZ; ++ty) {
            for (Index_type tx = 0; tx < TL_SZ; ++tx) {
              MAT_MAT_SHARED_BODY_4
            }
          }
        }
      }

    } // number of iterations
    stopTimer();

    break;
  }

#if defined(RUN_RAJA_SEQ)
  case Lambda_Seq: {


    startTimer();
    for (Index_type irep = 0; irep < run_reps; ++irep) {

      auto outer_y = [&](Index_type by) {
        auto outer_x = [&](Index_type bx) {
          MAT_MAT_SHARED_BODY_0

          auto inner_y_1 = [&](Index_type ty) {
            auto inner_x_1 = [&](Index_type tx) { MAT_MAT_SHARED_BODY_1 };

            for (Index_type tx = 0; tx < TL_SZ; ++tx) {
              if (tx < TL_SZ)
                inner_x_1(tx);
            }
          };

          for (Index_type ty = 0; ty < TL_SZ; ++ty) {
            if (ty < TL_SZ)
              inner_y_1(ty);
          }

          for (Index_type k = 0; k < (TL_SZ + N - 1) / TL_SZ; ++k) {

            auto inner_y_2 = [&](Index_type ty) {
              auto inner_x_2 = [&](Index_type tx) { MAT_MAT_SHARED_BODY_2 };

              for (Index_type tx = 0; tx < TL_SZ; ++tx) {
                inner_x_2(tx);
              }
            };

            for (Index_type ty = 0; ty < TL_SZ; ++ty) {
              inner_y_2(ty);
            }

            auto inner_y_3 = [&](Index_type ty) {
              auto inner_x_3 = [&](Index_type tx) { MAT_MAT_SHARED_BODY_3 };

              for (Index_type tx = 0; tx < TL_SZ; ++tx) {
                inner_x_3(tx);
              }
            };

            for (Index_type ty = 0; ty < TL_SZ; ++ty) {
              inner_y_3(ty);
            }
          }

          auto inner_y_4 = [&](Index_type ty) {
            auto inner_x_4 = [&](Index_type tx) { MAT_MAT_SHARED_BODY_4 };

            for (Index_type tx = 0; tx < TL_SZ; ++tx) {
              inner_x_4(tx);
            }
          };

          for (Index_type ty = 0; ty < TL_SZ; ++ty) {
            inner_y_4(ty);
          }
        }; // outer_x

        for (Index_type bx = 0; bx < Nx; ++bx) {
          outer_x(bx);
        }
      };

      for (Index_type by = 0; by < Ny; ++by) {
        outer_y(by);
      }

    } // irep
    stopTimer();

    break;
  }

  case RAJA_Seq: {

#if defined(RAJA_ENABLE_CUDA)
    using device_launch = RAJA::expt::cuda_launch_t<true>;
    using gpu_block_x_policy = RAJA::cuda_block_x_direct;
    using gpu_block_y_policy = RAJA::cuda_block_y_direct;
    using gpu_thread_x_policy = RAJA::cuda_thread_x_direct;
    using gpu_thread_y_policy = RAJA::cuda_thread_y_direct;
#endif

#if defined(RAJA_ENABLE_HIP)
    using device_launch = RAJA::expt::hip_launch_t<true>;
    using gpu_block_x_policy = RAJA::hip_block_x_direct;
    using gpu_block_y_policy = RAJA::hip_block_y_direct;
    using gpu_thread_x_policy = RAJA::hip_thread_x_direct;
    using gpu_thread_y_policy = RAJA::hip_thread_y_direct;
#endif

    //Currently Teams requires two policies if compiled with a device
    using launch_policy = RAJA::expt::LaunchPolicy<RAJA::expt::seq_launch_t
#if defined(RAJA_DEVICE_ACTIVE)
                                                   ,device_launch
#endif
                                                   >;

    using teams_x = RAJA::expt::LoopPolicy<RAJA::loop_exec
#if defined(RAJA_DEVICE_ACTIVE)
                                           ,gpu_block_x_policy
#endif
                                           >;

    using teams_y = RAJA::expt::LoopPolicy<RAJA::loop_exec
#if defined(RAJA_DEVICE_ACTIVE)
                                           ,gpu_block_y_policy
#endif
                                           >;

    using threads_x = RAJA::expt::LoopPolicy<RAJA::loop_exec
#if defined(RAJA_DEVICE_ACTIVE)
                                             ,gpu_thread_x_policy
#endif
                                             >;

    using threads_y = RAJA::expt::LoopPolicy<RAJA::loop_exec
#if defined(RAJA_DEVICE_ACTIVE)
                                             ,gpu_thread_y_policy
#endif
                                             >;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      //Resources is empty as the host does not need a compute grid to be specified
      RAJA::expt::launch<launch_policy>(RAJA::expt::HOST, RAJA::expt::Resources(),
          [=] RAJA_HOST_DEVICE(RAJA::expt::LaunchContext ctx) {

            RAJA::expt::loop<teams_y>(ctx, RAJA::RangeSegment(0, Ny), [&](Index_type by) {
               RAJA::expt::loop<teams_x>(ctx, RAJA::RangeSegment(0, Nx), [&](Index_type bx) {

                   MAT_MAT_SHARED_BODY_0

                   RAJA::expt::loop<threads_y>(ctx, RAJA::RangeSegment(0, TL_SZ), [&](Index_type ty) {
                     RAJA::expt::loop<threads_x>(ctx, RAJA::RangeSegment(0, TL_SZ), [&](Index_type tx) {
                         MAT_MAT_SHARED_BODY_1
                     });
                   });

                   for (Index_type k = 0; k < (TL_SZ + N - 1) / TL_SZ; k++) {

                     RAJA::expt::loop<threads_y>(ctx, RAJA::RangeSegment(0, TL_SZ), [&](Index_type ty) {
                       RAJA::expt::loop<threads_x>(ctx, RAJA::RangeSegment(0, TL_SZ), [&](Index_type tx) {
                           MAT_MAT_SHARED_BODY_2
                        });
                      });

                      ctx.teamSync();

                      RAJA::expt::loop<threads_y>(ctx, RAJA::RangeSegment(0, TL_SZ), [&](Index_type ty) {
                        RAJA::expt::loop<threads_x>(ctx, RAJA::RangeSegment(0, TL_SZ), [&](Index_type tx) {
                            MAT_MAT_SHARED_BODY_3
                        });
                      });

                      ctx.teamSync();
                    }

                    RAJA::expt::loop<threads_y>(ctx, RAJA::RangeSegment(0, TL_SZ), [&](Index_type ty) {
                      RAJA::expt::loop<threads_x>(ctx, RAJA::RangeSegment(0, TL_SZ), [&](Index_type tx) {
                          MAT_MAT_SHARED_BODY_4
                      });
                    });
               });
            });
          }); // kernel
    }
    stopTimer();

    break;
  }
#endif // RUN_RAJA_SEQ

  default: {
    std::cout << "\n  MAT_MAT_SHARED : Unknown variant id = " << vid
              << std::endl;
  }
  }
}

} // end namespace basic
} // end namespace rajaperf
