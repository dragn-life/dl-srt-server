cmake_minimum_required(VERSION 3.29)
project(msp_srt_server)

set(CMAKE_CXX_STANDARD 17)

# Define the paths manually (adjust these to your specific paths)
set(INCLUDE_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/include")
set(LIBRARY_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/lib")
set(SRT_LIB "${LIBRARY_DIR}/srt.lib")
set(SSL_LIB "${LIBRARY_DIR}/libssl.lib")
set(CRYPTO_LIB "${LIBRARY_DIR}/libcrypto.lib")

# Include directors and libraries
include_directories(${INCLUDE_DIR})

add_executable(msp_srt_server main.cpp main.cpp)

# Link libraries
target_link_libraries(msp_srt_server ${SSL_LIB} ${CRYPTO_LIB} ${SRT_LIB} Ws2_32.lib)

