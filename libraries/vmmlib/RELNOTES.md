# Release Notes
2013/05/02

vmmlib - a templatized C++ vector and matrix math library

Its basic functionality includes a vector and a matrix class, with additional functionality for the often-used 3d and 4d vectors and 3x3 and 4x4 matrices.
More advanced features include solvers, frustum computations and frustum culling classes, and spatial data structures. vmmlib also offers support for manipulating 3rd-order and 4th-order tensors, as well as several algorithms for tensor approximation.

Vmmlib is implemented using C++ templates, making it versatile. Being a header library, it is very easy to integrate into other (your) libraries and programs. There is no need to build and install a library, just include the headers and you’re set.
The BSD license allows the usage both in open source and commercial closed source software.

# New in this release

###CMake
* use now only CMake to create a Makefile

###Use Tensor Approximation Classes (based on LAPACK, BLAS)
* define VMMLIB_USE_LAPACK in Makefile (changed from VMMLIB_BASIC_ONLY)

### Matrix
* added matrix validators

###Tensor Approximation Classes
* memory mapping classes for large tensor data structures (tensor_mmapper.hpp) 
* created t3_converter: used to import/export tensors; converting/reading/writing files; quantization
* moved tensor3 reading/writing/quantization/tensor times matrix multiplications to t3_converter.hpp
* added tensor times matrix multiplications (ttm) and tensor times vector multiplication (ttv): multiply tensors along various modes
* use OpenMP for some parallel computations (tensor3, ttm)
* added tensor4 data structure (including t4_converter for converting/reading/writing files)
* added class for handling statistics on tensor algorithms: tensor_stats
* updated incremental methods (added ihooi; updated ihopm)

###BLAS Wrapper
* added BLAS DAXPY wrapper

###Filtering
* added intersection, lowpass_filter class
* updates in frustum_culler

###Unit Tests
* boolean global results of every test are now consistent with every subtest

## Documentation

The following have been added:

* Tensor Approximation tutorial: www.ifi.uzh.ch/vmml/links/TutorTensorAprox.html
* Tensor Approximation slides: http://files.ifi.uzh.ch/vmml/ta_tutorial/vmmlib_ta_classes.pdf
* "toy" example project http://files.ifi.uzh.ch/vmml/ta_tutorial/vmmlib_play.zip

## Bug Fixes

vmmlib 1.6 includes several fixes over the last release, such as:

* various test fixes
* precision bug of CP3 tensor reconstruction error measurement
* tensor4 "<<" operator
* computation of residual norm in HOOI
* corrected formula for quaternion slerp

## Known Issues

* memory mapping for windows to be tested
* test for slerp and matrix validators are not yet implemented
* tests that depend on rand() are deactivated: they may break with different stdlib versions
* tests that depend on LAPACK and BLAS are not fully supported for Windows

## Planned Future Extensions

* decomposition and reconstruction algorithms for 4D tensors

# About

## Platform Support

* Linux
* Mac OS X
* WIN64 operating systems

## Documentation

## Support

* comments and requests can be issued at http://github.com/VMML/vmmlib/issues
* contributions can be merged into vmmlib via a pull request

# Errata
