cmake_minimum_required(VERSION 3.10)

#set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(fft_correlator)

set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows/share")

find_package(FFTW3 CONFIG REQUIRED)
  
add_executable(fft_correlator fft_correlator.cpp)

target_link_libraries(fft_correlator PRIVATE FFTW3::fftw3)