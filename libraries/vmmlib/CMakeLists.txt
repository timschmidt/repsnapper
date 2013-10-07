# Copyright (c) 2011 Daniel Pfeifer <daniel@pfeifer-mail.de>

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(VMMLib)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 3)
set(VERSION_PATCH 5)
set(RELEASE_VERSION 1) # OFF or ABI version

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)
include(Common)
include(DoxygenSetup)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-shadow -Wno-sign-compare -Wno-extra -Wno-strict-aliasing")

include_directories(BEFORE ${CMAKE_SOURCE_DIR}/include)

if(APPLE)
  find_library(LAPACK_LIBRARIES Accelerate)
  if(LAPACK_LIBRARIES)
    set(LAPACK_FOUND TRUE)
  endif(LAPACK_LIBRARIES)
else(APPLE)
#  enable_language(Fortran OPTIONAL) # eile: Why needed?
  find_package(BLAS)
  if(BLAS_FOUND)
    find_package(LAPACK)
  endif()
endif(APPLE)

set(DOCS README AUTHORS LICENSE.txt ACKNOWLEDGEMENTS)

set(HEADERS
  include/vmmlib/ACKNOWLEDGEMENTS
  include/vmmlib/LICENSE
  include/vmmlib/aabb.hpp
  include/vmmlib/blas_daxpy.hpp
  include/vmmlib/blas_dgemm.hpp
  include/vmmlib/blas_dot.hpp
  include/vmmlib/blas_includes.hpp
  include/vmmlib/blas_types.hpp
  include/vmmlib/cp3_tensor.hpp
  include/vmmlib/cublas_dgemm.cu
  include/vmmlib/cublas_includes.hpp
  include/vmmlib/cublas_types.hpp
  include/vmmlib/enable_if.hpp
  include/vmmlib/exception.hpp
  include/vmmlib/frustum.hpp
  include/vmmlib/frustum_culler.hpp
  include/vmmlib/jacobi_solver.hpp
  include/vmmlib/lapack.hpp
  include/vmmlib/lapack/detail/clapack.h
  include/vmmlib/lapack/detail/f2c.h
  include/vmmlib/lapack_gaussian_elimination.hpp
  include/vmmlib/lapack_includes.hpp
  include/vmmlib/lapack_linear_least_squares.hpp
  include/vmmlib/lapack_svd.hpp
  include/vmmlib/lapack_sym_eigs.hpp
  include/vmmlib/lapack_types.hpp
  include/vmmlib/linear_least_squares.hpp
  include/vmmlib/math.hpp
  include/vmmlib/matrix.hpp
  include/vmmlib/matrix_functors.hpp
  include/vmmlib/matrix_pseudoinverse.hpp
  include/vmmlib/matrix_traits.hpp
  include/vmmlib/qr_decomposition.hpp
  include/vmmlib/qtucker3_tensor.hpp
  include/vmmlib/quaternion.hpp
  include/vmmlib/svd.hpp
  include/vmmlib/t3_converter.hpp
  include/vmmlib/t3_cublas_hooi.hpp
  include/vmmlib/t3_cublas_hosvd.hpp
  include/vmmlib/t3_cublas_ttm.hpp
  include/vmmlib/t3_hooi.hpp
  include/vmmlib/t3_hopm.hpp
  include/vmmlib/t3_hosvd.hpp
  include/vmmlib/t3_ihopm.hpp
  include/vmmlib/t3_ttm.hpp
  include/vmmlib/tensor3.hpp
  include/vmmlib/tensor3_iterator.hpp
  include/vmmlib/tucker3_exporter.hpp
  include/vmmlib/tucker3_importer.hpp
  include/vmmlib/tucker3_tensor.hpp
  include/vmmlib/validator.hpp
  include/vmmlib/vector.hpp
  include/vmmlib/vector_traits.hpp
  include/vmmlib/visibility.hpp
  include/vmmlib/vmmlib.hpp
  include/vmmlib/vmmlib_config.hpp
)

set(TESTS
  tests/frustum_test.cpp
  tests/jacobi_test.cpp
  tests/matrix_test.cpp
  tests/perf_test.cpp
  tests/performance_test.cpp
  tests/qr_decomposition_test.cpp
  tests/quaternion_test.cpp
  tests/svd_test.cpp
  tests/timer.cpp
  tests/unit_test.cpp
  tests/vector_test.cpp
  tests/vmmlib_unit_tests_main.cpp
# tests/matrix_compare_perf_test.cpp
# tests/stringtests.cpp
# tests/vmmlib_lapack_main.cpp
# tests/vmmlib_performance_tests_main.cpp
)

if(LAPACK_FOUND)
  add_definitions(-DVMMLIB_USE_LAPACK)
  list(APPEND TESTS
    tests/blas_dot_test.cpp
    tests/blas_daxpy_test.cpp
    tests/blas_dgemm_test.cpp
    tests/cp3_tensor_test.cpp
    tests/lapack_gaussian_elimination_test.cpp
    tests/lapack_linear_least_squares_test.cpp
    tests/lapack_svd_test.cpp
    tests/lapack_sym_eigs_test.cpp
    tests/matrix_pseudoinverse_test.cpp
    tests/qtucker3_tensor_test.cpp
    tests/svd_lapack_vs_old.cpp
    tests/t3_hooi_test.cpp
    tests/t3_hopm_test.cpp
    tests/t3_hosvd_test.cpp
    tests/t3_ihopm_test.cpp
    tests/t3_ttm_test.cpp
    tests/tensor3_iterator_test.cpp
    tests/tensor3_test.cpp
    tests/tucker3_exporter_importer_test.cpp
    tests/tucker3_tensor_test.cpp
)
endif()

if(NOT MSVC)
  add_executable(vmmlib_test ${HEADERS} ${TESTS})
  if(LAPACK_FOUND)
    target_link_libraries(vmmlib_test ${LAPACK_LIBRARIES})
  endif()
  if(BLAS_FOUND)
    target_link_libraries(vmmlib_test ${BLAS_LIBRARIES})
  endif()
  add_test(vmmlib_test vmmlib_test)
endif()

include(DoxygenRule) # must be after all targets

install(FILES ${HEADERS} DESTINATION include/vmmlib COMPONENT dev)
install(FILES ${DOCS} DESTINATION share/vmmlib COMPONENT dev)
install(FILES CMake/FindVMMLIB.cmake DESTINATION ${CMAKE_MODULE_INSTALL_PATH}
  COMPONENT dev)

include(CPackConfig)
