cmake_minimum_required(VERSION 2.6)

set(COMPILE_DEFINITIONS -Wall -g -O3)
set(CMAKE_BUILD_TYPE Release)

include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

link_directories(/opt/vc/lib)

add_executable(raspi2fb raspi2fb.c syslogUtilities.c)

target_link_libraries(raspi2fb bcm_host bsd)

set_property(TARGET raspi2fb PROPERTY SKIP_BUILD_RPATH TRUE)
install (TARGETS raspi2fb RUNTIME DESTINATION bin)
