cmake_minimum_required(VERSION 3.10)

#set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(fft_correlator_e1c)

set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows/share")

find_package(FFTW3 CONFIG REQUIRED)
  
add_executable(fft_correlator_e1c fft_correlator_e1c.cpp)

target_link_libraries(fft_correlator_e1c PRIVATE FFTW3::fftw3)