project(libreprap)
cmake_minimum_required(VERSION 2.4.6)
add_definitions(-Wall -Wextra)

add_library(reprap SHARED
  reprap.c
  serial.c
  )

install(TARGETS reprap DESTINATION lib)
install(FILES reprap.h DESTINATION include)
