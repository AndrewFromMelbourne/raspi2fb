cmake_minimum_required(VERSION 2.6)

SET(COMPILE_DEFINITIONS -Wall -g -O3)

include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

link_directories(/opt/vc/lib)

add_executable(raspi2fb raspi2fb.c syslogUtilities.c)

target_link_libraries(raspi2fb bcm_host bsd)
