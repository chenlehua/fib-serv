cmake_minimum_required(VERSION 3.21)
project(fib-serv)

set(TARGET_FILE "fib-serv")
set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_C_STANDARD 17)

# a simple way to check non-standard C header files (includes the atomic-related one).
include(CheckIncludeFiles)
check_include_files("pthread.h;stdatomic.h;sys/socket.h;netinet/in.h;unistd.h" EDEPS)
if (EPTHREAD EQUAL 1)
    message(FATAL_ERROR "Necessary header files are not found!")
endif ()

include_directories(. /usr/local/include)

aux_source_directory(./src DIR_SRCS)
add_subdirectory(libs/)

# load packages
find_package(uriparser 0.9.6 CONFIG REQUIRED char)

add_executable(${TARGET_FILE} ${DIR_SRCS})
target_link_libraries(${TARGET_FILE} PUBLIC core m pthread uriparser::uriparser)