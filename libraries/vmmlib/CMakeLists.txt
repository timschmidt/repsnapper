# Copyright (c) 2011 Daniel Pfeifer <daniel@pfeifer-mail.de>

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(VMMLib)

enable_testing()

include_directories(include)

if(APPLE)
  find_library(LAPACK_LIBRARIES accelerate)
  if(LAPACK_LIBRARIES)
    set(LAPACK_FOUND TRUE)
  endif(LAPACK_LIBRARIES)
else(APPLE)
  enable_language(Fortran)
  find_package(LAPACK)
endif(APPLE)

if(LAPACK_FOUND)
  add_definitions(-DVMMLIB_USE_LAPACK=1)
endif(LAPACK_FOUND)

add_executable(vmmlib_test
  tests/cp3_tensor_test.cpp
  tests/frustum_test.cpp
  tests/jacobi_test.cpp
  tests/lapack_gaussian_elimination_test.cpp
  tests/lapack_linear_least_squares_test.cpp
  tests/lapack_svd_test.cpp
  tests/lapack_sym_eigs_test.cpp
# tests/matrix_compare_perf_test.cpp
  tests/matrix_pseudoinverse_test.cpp
  tests/matrix_test.cpp
  tests/perf_test.cpp
  tests/performance_test.cpp
  tests/qr_decomposition_test.cpp
  tests/quaternion_test.cpp
# tests/stringtests.cpp
  tests/svd_lapack_vs_old.cpp
  tests/svd_test.cpp
  tests/tensor3_iterator_test.cpp
  tests/tensor3_test.cpp
  tests/timer.cpp
  tests/tucker3_tensor_test.cpp
  tests/unit_test.cpp
  tests/vector_test.cpp
# tests/vmmlib_lapack_main.cpp
# tests/vmmlib_performance_tests_main.cpp
  tests/vmmlib_unit_tests_main.cpp
  )

target_link_libraries(vmmlib_test
  ${LAPACK_LIBRARIES}
  )

add_test(vmmlib_test vmmlib_test)
