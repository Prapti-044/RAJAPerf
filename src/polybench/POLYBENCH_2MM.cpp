/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   Implementation file for Polybench kernel 2MM
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017, Lawrence Livermore National Security, LLC.
//
// Produced at the Lawrence Livermore National Laboratory
//
// LLNL-CODE-xxxxxx
//
// All rights reserved.
//
// This file is part of the RAJA Performance Suite.
//
// For additional details, please read the file LICENSE.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//


#include "POLYBENCH_2MM.hpp"

#include "common/DataUtils.hpp"
#include <RAJA/RAJA.hpp>


#include <iostream>

namespace rajaperf 
{
namespace polybench
{

#define POLYBENCH_2MM_DATA \
  ResReal_ptr tmp = m_tmp; \
  ResReal_ptr A = m_A; \
  ResReal_ptr B = m_B; \
  ResReal_ptr C = m_C; \
  ResReal_ptr D = m_D; \
  Real_type alpha = m_alpha; \
  Real_type beta = m_beta; \
  alpha = 1.5; \
  beta = 1.2; \
  int i, j; \
  for(i=0; i < m_ni; ++i) \
    for(j=0; j < m_nl; ++j) \
      *(D + i * m_nj + j) = (Real_type) (i * (j+2) % m_nk) / m_nk;


// The following 2MM_BODY is a prototype of the kernel copied over from the polybench suite and is not used in the runKernel calls  
// It's just for illustrative purposes
#define POLYBENCH_2MM_BODY \
  int i, j, k;\
  /* D := alpha*A*B*C + beta*D */ \
  for (i = 0; i < m_ni; i++) \
    for (j = 0; j < m_nj; j++) { \
	      m_tmp[i][j] = 0.0; \
	      for (k = 0; k < m_nk; ++k) \
	        m_tmp[i][j] += m_alpha * m_A[i][k] * m_B[k][j]; \
    } \
  for (i = 0; i < m_ni; i++) \
    for (j = 0; j < m_nl; j++) { \
	    m_D[i][j] *= m_beta; \
	    for (k = 0; k < m_nj; ++k) \
	      m_D[i][j] += m_tmp[i][k] * m_C[k][j]; \
    } 

#define POLYBENCH_2MM_BODY1 \
  *(tmp + i * nj + j) = 0.0;

#define POLYBENCH_2MM_BODY2 \
  *(tmp + i * nj + j) += alpha * *(A + i * nk + k) * *(B + k * nj + j);

#define POLYBENCH_2MM_BODY3 \
  *(D + i * nl + l) *= beta;

#define POLYBENCH_2MM_BODY4 \
  *(D + i * nl + l) += *(tmp + i * nj + j) * *(C + j * nl + l);


#if defined(RAJA_ENABLE_CUDA)

  //
  // Define thread block size for CUDA execution
  //
  const size_t block_size = 256;


#define POLYBENCH_2MM_DATA_SETUP_CUDA \
  Real_ptr tmp = m_tmp; \
  Real_ptr A = m_A; \
  Real_ptr B = m_B; \
  Real_ptr C = m_C; \
  Real_ptr D = m_D; \
  Real_type alpha = m_alpha; \
  Real_type beta = m_beta; \
  alpha = 1.5; \
  beta = 1.2; \
  int i, j; \
  for(i=0; i < m_ni; ++i) \
    for(j=0; j < m_nl; ++j) \
      *(D + i * m_nj + j) = (Real_type) (i * (j+2) % m_nk) / m_nk; \
\
  allocAndInitCudaDeviceData(tmp, m_tmp, m_ni * m_nj); \
  allocAndInitCudaDeviceData(A, m_A, m_ni * m_nk); \
  allocAndInitCudaDeviceData(B, m_B, m_nk * m_nj); \
  allocAndInitCudaDeviceData(C, m_C, m_nj * m_nl); \
  allocAndInitCudaDeviceData(D, m_D, m_ni * m_nl); 


#define POLYBENCH_2MM_TEARDOWN_CUDA \
  getCudaDeviceData(m_D, D, m_ni * m_nl); \
  deallocCudaDeviceData(tmp); \
  deallocCudaDeviceData(A); \
  deallocCudaDeviceData(B); \
  deallocCudaDeviceData(C); \
  deallocCudaDeviceData(D);

__global__ void polybench_2mm_cuda_1(Real_ptr tmp, Real_ptr A,
                       Real_ptr B, Real_ptr C, Real_ptr D,
                       Real_type alpha, Real_type beta, Index_type ni, Index_type nj,
                       Index_type nk, Index_type nl)
{
   Index_type ii = blockIdx.x * blockDim.x + threadIdx.x;
   Index_type i,j,k;
   if (ii < ni * nj) {
     *(tmp + ii) = 0.0;
     i = ii/nj; j = ii % nj;
     for(k=0; k < nk; k++) {
       POLYBENCH_2MM_BODY2;              
     }
   }


}

__global__ void polybench_2mm_cuda_2(Real_ptr tmp, Real_ptr A,
                       Real_ptr B, Real_ptr C, Real_ptr D,
                       Real_type alpha, Real_type beta, Index_type ni, Index_type nj,
                       Index_type nk, Index_type nl)
{
   Index_type ii = blockIdx.x * blockDim.x + threadIdx.x;
   Index_type i,l,j;
   if (ii < ni * nl) {
     *(D + ii) *= beta;
     i = ii/nl; l = ii % nl;
     for(j=0; j < nj; j++) {
       POLYBENCH_2MM_BODY4;              
     }
   }
}


#endif // if defined(RAJA_ENABLE_CUDA)
  



  


POLYBENCH_2MM::POLYBENCH_2MM(const RunParams& params)
  : KernelBase(rajaperf::Polybench_2MM, params)
{
  setDefaultSamples(1);
}

POLYBENCH_2MM::~POLYBENCH_2MM() 
{
}

void POLYBENCH_2MM::setUp(VariantID vid)
{

  m_alpha = 1.5;
  m_beta = 1.2;
  SizeSpec_T lsizespec = KernelBase::getSizeSpec();
  switch(lsizespec) {
    case Mini:
      m_ni=16; m_nj=18; m_nk=22; m_nl=24;
      m_run_samples = 100000;
      break;
    case Small:
      m_ni=40; m_nj=50; m_nk=70; m_nl=80;
      m_run_samples = 10000;
      break;
    case Medium:
      m_ni=180; m_nj=190; m_nk=210; m_nl=220;
      m_run_samples = 100;
      break;
    case Large:
      m_ni=800; m_nj=900; m_nk=1100; m_nl=1200;
      m_run_samples = 1;
      break;
    case Extralarge:
      m_ni=1600; m_nj=1800; m_nk=2200; m_nl=2400;
      m_run_samples = 1;
      break;
    default:
      m_ni=180; m_nj=190; m_nk=210; m_nl=220;
      m_run_samples = 100;
      break;
  }
  allocAndInitData(m_tmp, m_ni * m_nj, vid);
  allocAndInitData(m_A, m_ni * m_nk, vid);
  allocAndInitData(m_B, m_nk * m_nj, vid);
  allocAndInitData(m_C, m_nj * m_nl, vid);
  allocAndInitData(m_D, m_ni * m_nl, vid);
  // Redo the initialization polybench style
  int i,j ;
  for(i=0; i < m_ni; i++) 
    for(j=0; j < m_nj; j++) 
      *(m_A + i * m_nj + j) = (Real_type) (i*j % m_ni) / m_ni; 
  for(i=0; i < m_nk ; i++) 
    for(j=0; j < m_nj; j++) 
      *(m_B + i * m_nj + j) = (Real_type) (i * (j+1) % m_nj) / m_nj; 
  for(i=0; i < m_nj; i++) 
    for(j=0; j < m_nl; j++) 
      *(m_C + i * m_nj + j) = (Real_type) (i * (j+3) % m_nl) / m_nl; 

  // D's initialization also gets redone in the data macro prior to each kernel variant      
  for(i=0; i < m_ni; i++) \
    for(j=0; j < m_nl; j++) \
      *(m_D + i * m_nl + j) = (Real_type) (i * (j+2) % m_nk) / m_nk;

}

void POLYBENCH_2MM::runKernel(VariantID vid)
{
  const Index_type run_samples = getRunSamples();
  const Index_type ni = m_ni;
  const Index_type nj = m_nj;
  const Index_type nk = m_nk;
  const Index_type nl = m_nl;

  fprintf(stderr,"Polybench_2MM Current Dimensions: ni %d, nj %d, nk %d, nl %d\n",ni,nj,nk,nl);

  switch ( vid ) {

    case Baseline_Seq : {

      POLYBENCH_2MM_DATA;
      startTimer();
      for (SampIndex_type isamp = 0; isamp < m_run_samples; ++isamp) {
#if 1
        for (Index_type i = 0; i < ni; i++ ) 
          for(Index_type j = 0; j < nj; j++) {
            POLYBENCH_2MM_BODY1;
            for(Index_type k = 0; k < nk; k++)
              POLYBENCH_2MM_BODY2;
          }

        for(Index_type i = 0; i < ni; i++)
          for(Index_type l = 0; l < nl; l++) {
            POLYBENCH_2MM_BODY3;
            for(Index_type j = 0; j < nj; j++)
              POLYBENCH_2MM_BODY4;
          }  
        
#endif

      }
      stopTimer();

      break;
    }

    case RAJA_Seq : {

      POLYBENCH_2MM_DATA;
      resetTimer();
      startTimer();
      for (SampIndex_type isamp = 0; isamp < m_run_samples; ++isamp) {

#if 0 
        // The following kernel  generates a small checksum error : Will's variant using reductions
        RAJA::forallN<RAJA::NestedPolicy<RAJA::ExecList<RAJA::seq_exec,RAJA::seq_exec>>> (RAJA::RangeSegment{0, ni}, RAJA::RangeSegment{0, nj}, [=] (int i, int j) {
          RAJA::ReduceSum<RAJA::seq_reduce, Real_type> t(0);
          RAJA::forall<RAJA::seq_exec> (RAJA::RangeSegment{0, nk}, [=] (int k) {
            t += alpha * *(A + i *nk + k) * *(B + k * nj + j);
          });
          *(tmp + i * nj + j) = t;
        });
        RAJA::forallN<RAJA::NestedPolicy<RAJA::ExecList<RAJA::seq_exec,RAJA::seq_exec>>> (RAJA::RangeSegment{0, ni}, RAJA::RangeSegment{0, nl}, [=] (int i, int l) {
          RAJA::ReduceSum<RAJA::seq_reduce, Real_type> d(0);
          RAJA::forall<RAJA::seq_exec> (RAJA::RangeSegment{0, nj}, [=] (int j) {
            d += *(tmp + i * nj + j) * *(C + j * nl + l);
          });
          *(D + i * nl + l) = *(D + i * nl + l) * beta + d;
        });

#endif

#if  1       

        RAJA::forallN<RAJA::NestedPolicy<RAJA::ExecList<RAJA::seq_exec,RAJA::seq_exec>>> (RAJA::RangeSegment{0, ni}, RAJA::RangeSegment{0, nj}, [=] (int i, int j) {
          POLYBENCH_2MM_BODY1;

          RAJA::forall<RAJA::seq_exec> (RAJA::RangeSegment{0, nk}, [=] (int k) {
            POLYBENCH_2MM_BODY2; 
          });
        });
        RAJA::forallN<RAJA::NestedPolicy<RAJA::ExecList<RAJA::seq_exec,RAJA::seq_exec>>> (RAJA::RangeSegment{0, ni}, RAJA::RangeSegment{0, nl}, [=] (int i, int l) {
          POLYBENCH_2MM_BODY3;

          RAJA::forall<RAJA::seq_exec> (RAJA::RangeSegment{0, nj}, [=] (int j) {
            POLYBENCH_2MM_BODY4;
          });
        });

#endif

      }
      stopTimer();

      break;
    }

    case Baseline_OpenMP : {
#if defined(_OPENMP)      
      POLYBENCH_2MM_DATA;
      startTimer();
      for (SampIndex_type isamp = 0; isamp < m_run_samples; ++isamp) {
        #pragma omp parallel for  
        for (Index_type i = 0; i < ni; i++ ) 
          for(Index_type j = 0; j < nj; j++) {
            //if(j == 0) printf("Thread : %d\n",omp_get_thread_num());
            POLYBENCH_2MM_BODY1;
            for(Index_type k = 0; k < nk; k++) {

              POLYBENCH_2MM_BODY2;
            }
          }


        #pragma omp parallel for   
        for(Index_type i = 0; i < ni; i++)
          for(Index_type l = 0; l < nl; l++) {
            //if(l == 0) printf("Thread : %d\n",omp_get_thread_num());
            POLYBENCH_2MM_BODY3;
            for(Index_type j = 0; j < nj; j++)
              POLYBENCH_2MM_BODY4;
          }  

      }
      stopTimer();
#endif
      break;
    }

    case RAJALike_OpenMP : {
      // case is not defined...
      break;
    }

    case RAJA_OpenMP : {
#if defined(_OPENMP)      
      POLYBENCH_2MM_DATA;
      startTimer();
      for (SampIndex_type isamp = 0; isamp < m_run_samples; ++isamp) {
        RAJA::forallN<RAJA::NestedPolicy<RAJA::ExecList<RAJA::omp_parallel_for_exec,RAJA::seq_exec>>> (RAJA::RangeSegment{0, ni}, RAJA::RangeSegment{0, nj}, [=] (int i, int j) {
          POLYBENCH_2MM_BODY1;

          RAJA::forall<RAJA::seq_exec> (RAJA::RangeSegment{0, nk}, [=] (int k) {
            POLYBENCH_2MM_BODY2; 
          });
        });
        RAJA::forallN<RAJA::NestedPolicy<RAJA::ExecList<RAJA::omp_parallel_for_exec,RAJA::seq_exec>>> (RAJA::RangeSegment{0, ni}, RAJA::RangeSegment{0, nl}, [=] (int i, int l) {
          POLYBENCH_2MM_BODY3;

          RAJA::forall<RAJA::seq_exec> (RAJA::RangeSegment{0, nj}, [=] (int j) {
            POLYBENCH_2MM_BODY4;
          });
        });


      }
      stopTimer();
#endif
      break;
    }

#if defined(RAJA_ENABLE_CUDA)
    case Baseline_CUDA : {

      POLYBENCH_2MM_DATA_SETUP_CUDA;
      startTimer();
      for (SampIndex_type isamp = 0; isamp < m_run_samples; ++isamp) {
        size_t grid_size = RAJA_DIVIDE_CEILING_INT(m_ni * m_nj, block_size);
        polybench_2mm_cuda_1<<<grid_size,block_size>>>(tmp,A,B,C,D,alpha,beta,m_ni,m_nj,m_nk,m_nl);

        grid_size = RAJA_DIVIDE_CEILING_INT(m_ni * m_nl, block_size);
        polybench_2mm_cuda_2<<<grid_size,block_size>>>(tmp,A,B,C,D,alpha,beta,m_ni,m_nj,m_nk,m_nl);
      }
      stopTimer();
      POLYBENCH_2MM_TEARDOWN_CUDA;
      break;
    }

    case RAJA_CUDA : {

      POLYBENCH_2MM_DATA_SETUP_CUDA;
      startTimer();
      for (SampIndex_type isamp = 0; isamp < m_run_samples; ++isamp) {
       
        RAJA::forall<RAJA::cuda_exec<block_size>> (RAJA::RangeSegment{0, ni * nj}, [=] __device__ (int ii) {
          Index_type i,j,k;
          *(tmp + ii) = 0.0;
          i = ii/nj; j = ii % nj;
          for(k=0;k<nk;k++) {
            POLYBENCH_2MM_BODY2; 
          }
        });


        RAJA::forall<RAJA::cuda_exec<block_size>> (RAJA::RangeSegment{0, ni * nl}, [=] __device__ (int ii) {
          *(D + ii) *= beta;
          Index_type i,l,j;
          i = ii/nl; l = ii % nl;
          for(j=0;j<nj;j++) {
            POLYBENCH_2MM_BODY4;
          }  
        });

      }
      stopTimer();
      POLYBENCH_2MM_TEARDOWN_CUDA;
      break;
    }

#endif

#if 0
    case Baseline_OpenMP4x :
    case RAJA_OpenMP4x : {
      // Fill these in later...you get the idea...
      break;
    }
#endif

    default : {
      std::cout << "\n  Unknown variant id = " << vid << std::endl;
    }

  }

}

void POLYBENCH_2MM::updateChecksum(VariantID vid)
{
  checksum[vid] += calcChecksum(m_D, m_ni * m_nl);
}

void POLYBENCH_2MM::tearDown(VariantID vid)
{
  (void) vid;
  deallocData(m_tmp);
  deallocData(m_A);
  deallocData(m_B);
  deallocData(m_C);
  deallocData(m_D);
}

} // end namespace basic
} // end namespace rajaperf
