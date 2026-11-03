#
# Copyright 2020 Adobe. All rights reserved.
# This file is licensed to you under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License. You may obtain a copy
# of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
# OF ANY KIND, either express or implied. See the License for the specific language
# governing permissions and limitations under the License.
#

# spdlog (https://github.com/gabime/spdlog)
# License: MIT

if(TARGET spdlog::spdlog)
    return()
endif()

message(STATUS "Third-party: creating target 'spdlog::spdlog'")

option(SPDLOG_INSTALL "Generate the install target" ON)
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "spdlog")

include(CPM)
CPMAddPackage("gh:gabime/spdlog@1.12.0")

# spdlog 1.12.0 bundles fmt 9.1.0, whose ostream.h unconditionally includes the
# libc++ internal header <__std_stream> on Windows. llvm-mingw's libc++ does not
# ship that header, breaking the build. Guard the libc++/Windows console path
# behind __has_include so it is only used when the header is actually present
# (upstream libc++ keeps it; llvm-mingw falls back to the generic path). This is
# applied idempotently on every configure so a clean checkout reproduces it.
if(spdlog_SOURCE_DIR)
    set(_spdlog_ostream_h "${spdlog_SOURCE_DIR}/include/spdlog/fmt/bundled/ostream.h")
    if(EXISTS "${_spdlog_ostream_h}")
        file(READ "${_spdlog_ostream_h}" _spdlog_ostream_content)
        string(REPLACE
            "#elif defined(_WIN32) && defined(_LIBCPP_VERSION)\n"
            "#elif defined(_WIN32) && defined(_LIBCPP_VERSION) && __has_include(<__std_stream>)\n"
            _spdlog_ostream_patched "${_spdlog_ostream_content}")
        if(NOT _spdlog_ostream_content STREQUAL _spdlog_ostream_patched)
            message(STATUS "Patching spdlog bundled fmt ostream.h for libc++/<__std_stream>")
            file(WRITE "${_spdlog_ostream_h}" "${_spdlog_ostream_patched}")
        endif()
    endif()
endif()

set_target_properties(spdlog PROPERTIES POSITION_INDEPENDENT_CODE ON)

set_target_properties(spdlog PROPERTIES FOLDER external)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" OR
   "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(spdlog PRIVATE
        "-Wno-sign-conversion"
    )
endif()
