# Copyright (c) 2011 Daniel Pfeifer <daniel@pfeifer-mail.de>

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(vmmlib)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 7)
set(VERSION_PATCH 0)
set(RELEASE_VERSION OFF) # OFF or ABI version
set(LAST_RELEASE 1.6.1)

option(VMMLIB_USE_BLAS "Disable to turn blas detection off" ON)
mark_as_advanced(VMMLIB_USE_BLAS)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)

set(MACPORTS_DIR eyescalePorts) # override for now...

include(Common)
include(GitTargets)
include(FindPackages)

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-shadow -Wno-sign-compare -Wno-extra -Wno-strict-aliasing")
endif()

include_directories(BEFORE ${CMAKE_SOURCE_DIR}/include)

if(APPLE)
  find_library(LAPACK_LIBRARIES Accelerate)
  if(LAPACK_LIBRARIES)
    set(LAPACK_FOUND TRUE)
  endif(LAPACK_LIBRARIES)
elseif(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#  enable_language(Fortran OPTIONAL) # eile: Why needed?
  if (VMMLIB_USE_BLAS)
    find_package(BLAS)
    if(BLAS_FOUND)
      list(GET BLAS_LIBRARIES 0 BLAS_LIBRARY_DIR)
      get_filename_component(BLAS_LIBRARY_DIR ${BLAS_LIBRARY_DIR} PATH)
      find_path(BLAS_INCLUDE_DIR cblas.h ${BLAS_LIBRARY_DIR}/../include
        NO_DEFAULT_PATH)
      if(BLAS_INCLUDE_DIR)
        find_package(LAPACK)
      else()
        message(STATUS "Missing cblas.h in ${BLAS_LIBRARY_DIR}/../include, disabling BLAS support")
        set(BLAS_FOUND)
      endif()
    endif()
  endif()
endif()

set(DOCS README.md AUTHORS LICENSE.txt ACKNOWLEDGEMENTS)
set(BLAS_LIBRARIES blas)

update_file(${CMAKE_CURRENT_SOURCE_DIR}/include/vmmlib/version.in.hpp
  ${OUTPUT_INCLUDE_DIR}/vmmlib/version.hpp)

set(HEADERS
  ${OUTPUT_INCLUDE_DIR}/vmmlib/version.hpp
  include/vmmlib/aabb.hpp
  include/vmmlib/blas_daxpy.hpp
  include/vmmlib/blas_dgemm.hpp
  include/vmmlib/blas_dot.hpp
  include/vmmlib/blas_includes.hpp
  include/vmmlib/blas_types.hpp
  include/vmmlib/cp3_tensor.hpp
  include/vmmlib/enable_if.hpp
  include/vmmlib/exception.hpp
  include/vmmlib/frustum.hpp
  include/vmmlib/frustum_culler.hpp
  include/vmmlib/intersection.hpp
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
  include/vmmlib/lowpass_filter.hpp
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
  include/vmmlib/t3_hooi.hpp
  include/vmmlib/t3_hopm.hpp
  include/vmmlib/t3_hosvd.hpp
  include/vmmlib/t3_ihopm.hpp
  include/vmmlib/t3_ihooi.hpp
  include/vmmlib/t3_ttm.hpp
  include/vmmlib/tensor3.hpp
  include/vmmlib/tensor3_iterator.hpp
  include/vmmlib/tucker3_exporter.hpp
  include/vmmlib/tucker3_importer.hpp
  include/vmmlib/tucker3_tensor.hpp
  include/vmmlib/util.hpp
  include/vmmlib/validator.hpp
  include/vmmlib/vector.hpp
  include/vmmlib/vector_traits.hpp
  include/vmmlib/visibility.hpp
  include/vmmlib/vmmlib.hpp
  include/vmmlib/vmmlib_config.hpp
  include/vmmlib/tensor4.hpp
  include/vmmlib/t4_converter.hpp
  include/vmmlib/t4_hooi.hpp
  include/vmmlib/t4_hosvd.hpp
  include/vmmlib/t4_ttm.hpp
  include/vmmlib/tucker4_tensor.hpp
)

set(TESTS
  tests/frustum_test.cpp
  tests/intersection_test.cpp
  tests/jacobi_test.cpp
  tests/matrix_test.cpp
  tests/perf_test.cpp
  tests/performance_test.cpp
  tests/qr_decomposition_test.cpp
  tests/quaternion_test.cpp
  tests/svd_test.cpp
  tests/timer.cpp
  tests/unit_test.cpp
  tests/unit_test_globals.cpp
  tests/lowpass_filter_test.cpp
  tests/vector_test.cpp
  tests/util_test.cpp
  tests/vmmlib_unit_tests_main.cpp
# tests/matrix_compare_perf_test.cpp
# tests/stringtests.cpp
# tests/vmmlib_lapack_main.cpp
# tests/vmmlib_performance_tests_main.cpp
)

if(LAPACK_FOUND)
  add_definitions(-DVMMLIB_USE_LAPACK)
  list(APPEND TESTS
    tests/blas_daxpy_test.cpp
    tests/blas_dgemm_test.cpp
    tests/blas_dot_test.cpp
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
    tests/t3_ihooi_test.cpp
    tests/t3_ihopm_test.cpp
    tests/t3_ttm_test.cpp
    tests/tensor3_iterator_test.cpp
    tests/tensor3_test.cpp
    tests/tucker3_exporter_importer_test.cpp
    tests/tucker3_tensor_test.cpp
    tests/t4_hooi_test.cpp
    tests/t4_converter_test.cpp
    tests/tensor4_test.cpp
    tests/t4_ttm_test.cpp

)
endif()
if(BLAS_FOUND)
  add_definitions(-DVMMLIB_USE_BLAS)
endif()

if(NOT MSVC)
  add_executable(vmmlib_test ${HEADERS} ${TESTS})
  if(LAPACK_FOUND)
    target_link_libraries(vmmlib_test ${LAPACK_LIBRARIES})
  endif()
  if(BLAS_FOUND)
    target_link_libraries(vmmlib_test ${BLAS_LIBRARIES})
  endif()

  get_target_property(EXECUTABLE vmmlib_test LOCATION)
  STRING(REGEX REPLACE "\\$\\(.*\\)" "\${CTEST_CONFIGURATION_TYPE}"
    EXECUTABLE "${EXECUTABLE}")
  add_test(vmmlib_test ${EXECUTABLE})

  # workaround: 'make test' does not build tests beforehand
  add_custom_target(tests COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS vmmlib_test)
  add_custom_target(vmmlib-test DEPENDS tests) # Travis CI hack
endif()

install(FILES ${HEADERS} DESTINATION include/vmmlib COMPONENT dev)
install(FILES ${DOCS} DESTINATION share/vmmlib COMPONENT dev)

include(DoxygenRule) # must be after all targets
include(CPackConfig)
include(CTest)
include(PackageConfig)

