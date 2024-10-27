#
# dl_srt_server
# Copyright (C) 2024 DragN Life LLC (Adam B)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 3.29)
project(dl_srt_server VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

# Define the paths manually (adjust these to your specific paths)
set(INCLUDE_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/include")
set(LIBRARY_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/lib")
set(SRT_LIB "${LIBRARY_DIR}/srt.lib")
set(SSL_LIB "${LIBRARY_DIR}/libssl.lib")
set(CRYPTO_LIB "${LIBRARY_DIR}/libcrypto.lib")

# Include directors and libraries
include_directories(${INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src) # Include the source directory for tests

# Static library of core components
add_library(dl_srt_server_lib STATIC
        src/core/SRTServer.cpp
        src/core/StreamSession.cpp
        src/core/StreamManager.cpp
        src/utils/SRTHandler.cpp
)

# Main executable
add_executable(dl_srt_server
        src/main.cpp
)

# Link libraries to main executable
target_link_libraries(dl_srt_server
        PRIVATE
        dl_srt_server_lib
        ${SSL_LIB}
        ${CRYPTO_LIB}
        ${SRT_LIB}
        Ws2_32.lib
)

# Testing configuration
option(BUILD_TESTS "Build the tests" ON)

if (BUILD_TESTS)
    # Add Google Test
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    # Add the tests
    add_executable(dl_srt_server_tests
            tests/MockSRTHandler.cpp
            tests/MockSRTHandler.h
            tests/StreamSessionTest.cpp
    )

    target_link_libraries(dl_srt_server_tests
            PRIVATE
            dl_srt_server_lib
            GTest::gtest_main
            GTest::gmock_main
            ${SSL_LIB}
            ${CRYPTO_LIB}
            ${SRT_LIB}
            Ws2_32.lib
    )

    include(GoogleTest)
    gtest_discover_tests(dl_srt_server_tests)
endif ()
