project(libreprap)
cmake_minimum_required(VERSION 2.6)

set(CPACK_PACKAGE_NAME "libreprap")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Common asynchronous communications library for reprap machines")
set(CPACK_PACKAGE_VENDOR "libreprap project")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_GENERATOR "TGZ")
include(CPack)

add_definitions(-Wall -Wextra)

add_library(reprap SHARED
  serial.c
  common.c
  fived.c
  tonokip.c
  comms.c
  util.c
  )

find_package(PkgConfig)
pkg_check_modules(USB libusb-1.0)
if(USB_FOUND)
  add_definitions(-DUSB)
  include_directories(${USB_INCLUDE_DIRS})
  message(STATUS "Building with USB support.")
else(USB_FOUND)
  message(STATUS "WARNING: NOT buiding with USB support.")
endif(USB_FOUND)

install(TARGETS reprap DESTINATION lib)
install(FILES comms.h DESTINATION include/reprap)
install(FILES util.h DESTINATION include/reprap)
