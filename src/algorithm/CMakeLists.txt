###############################################################################
# Copyright (c) 2017-22, Lawrence Livermore National Security, LLC
# and RAJA Performance Suite project contributors.
# See the RAJAPerf/LICENSE file for details.
#
# SPDX-License-Identifier: (BSD-3-Clause)
###############################################################################

blt_add_library(
  NAME algorithm
  SOURCES SCAN.cpp
          SCAN-Seq.cpp
          SCAN-Hip.cpp
          SCAN-Cuda.cpp
          SCAN-OMP.cpp
          SORT.cpp
          SORT-Seq.cpp
          SORT-Hip.cpp
          SORT-Cuda.cpp
          SORT-OMP.cpp
          SORTPAIRS.cpp
          SORTPAIRS-Seq.cpp
          SORTPAIRS-Hip.cpp
          SORTPAIRS-Cuda.cpp
          SORTPAIRS-OMP.cpp
          REDUCE_SUM.cpp
          REDUCE_SUM-Seq.cpp
          REDUCE_SUM-Hip.cpp
          REDUCE_SUM-Cuda.cpp
          REDUCE_SUM-OMP.cpp
          REDUCE_SUM-OMPTarget.cpp
  DEPENDS_ON common ${RAJA_PERFSUITE_DEPENDS}
  )
