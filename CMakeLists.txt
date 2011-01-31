project(libreprap)
cmake_minimum_required(VERSION 2.4.6)
add_definitions(-Wall -Wextra)

add_library(reprap SHARED
  serial.c
  common.c
  fived.c
  tonokip.c
  comms.c
  util.c
  )

install(TARGETS reprap DESTINATION lib)
install(FILES comms.h DESTINATION include/reprap)
install(FILES util.h DESTINATION include/reprap)
