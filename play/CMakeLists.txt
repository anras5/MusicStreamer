cmake_minimum_required(VERSION 3.12)
project(sieci)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_CXX_STANDARD 20)

include_directories(external/irrKlang/include/)
link_directories(external/irrKlang/linux-gcc-64/)
set(ALL_LIBS
    libIrrKlang.so
)

add_executable(play src/play.cpp)
target_link_libraries(play
    ${ALL_LIBS}
)