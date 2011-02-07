project(libreprap)
cmake_minimum_required(VERSION 2.6)

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
