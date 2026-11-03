# Detects whether this is a top-level project
get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
    set(POLYFEM_TOPLEVEL_PROJECT OFF)
else()
    set(POLYFEM_TOPLEVEL_PROJECT ON)
endif()

# Check required CMake version
set(REQUIRED_CMAKE_VERSION "3.25.0")
if(POLYFEM_TOPLEVEL_PROJECT)
    cmake_minimum_required(VERSION ${REQUIRED_CMAKE_VERSION})
else()
    # Don't use cmake_minimum_required here to avoid implicitly overriding parent policies
    if(${CMAKE_VERSION} VERSION_LESS ${REQUIRED_CMAKE_VERSION})
        message(FATAL_ERROR "CMake required version to build PolyFEM is ${REQUIRED_CMAKE_VERSION}")
    endif()
endif()

set(CMAKE_DISABLE_FIND_PACKAGE_AVX TRUE)

# Include user-provided default options if available. We do that before the main
# `project()` so that we can define the C/C++ compilers from the option file.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/PolyFEMOptions.cmake)
    message(STATUS "Using local options file: ${CMAKE_CURRENT_SOURCE_DIR}/PolyFEMOptions.cmake")
    include(${CMAKE_CURRENT_SOURCE_DIR}/PolyFEMOptions.cmake)
endif()

# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  option(POLYFEM_WITH_CCACHE "Enable ccache when building PolyFEM" ${POLYFEM_TOPLEVEL_PROJECT})
else()
  option(POLYFEM_WITH_CCACHE "Enable ccache when building PolyFEM" OFF)
endif()
if(POLYFEM_WITH_CCACHE AND CCACHE_PROGRAM)
  message(STATUS "Enabling Ccache support")
  set(ccacheEnv
    CCACHE_BASEDIR=${CMAKE_BINARY_DIR}
    CCACHE_SLOPPINESS=clang_index_store,include_file_ctime,include_file_mtime,locale,pch_defines,time_macros
  )
  foreach(lang IN ITEMS C CXX)
    set(CMAKE_${lang}_COMPILER_LAUNCHER
      ${CMAKE_COMMAND} -E env ${ccacheEnv} ${CCACHE_PROGRAM}
    )
  endforeach()
endif()

################################################################################
# CMake Policies
################################################################################

cmake_policy(SET CMP0054 NEW) # Only interpret if() arguments as variables or keywords when unquoted.
cmake_policy(SET CMP0076 NEW) # target_sources() command converts relative paths to absolute.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
  cmake_policy(SET CMP0135 NEW) # Set the timestamps of all extracted contents to the time of the extraction.
endif()

################################################################################
project(PolyFEM DESCRIPTION "A polyvalent C++ FEM library" LANGUAGES C CXX)

# Polyfem options
set(POLYFEM_SMALL_N  80 CACHE STRING "Maximum length for stack-allocated vectors (gradient + Hessian).")
set(POLYFEM_BIG_N   1000 CACHE STRING "Maximum length for stack-allocated vectors (gradient only).")

# The garment-retargeting sources are cloth-fit's domain layer. Keeping them
# optional lets weftfit/retarget consume this fork as *base PolyFEM* (the
# computational substrate) with -DPOLYFEM_WITH_GARMENT=OFF, then build the garment
# core on top through the mesh ports. Default ON so cloth-fit's own build (lib +
# NIF + garment CLI) is unchanged.
option(POLYFEM_WITH_GARMENT "Build the garment-retargeting domain layer (cloth-fit)" ON)

# Polyfem options for enabling/disabling optional libraries
option(POLYFEM_WITH_TESTS     "Build tests"                                 ON)
option(POLYFEM_WITH_USD       "Build OpenUSD mesh I/O (USDReader/USDWriter)" OFF)
set(POLYFEM_THREADING "TBB" CACHE STRING "Multithreading library to use (options: CPP, TBB, NONE)")
set_property(CACHE POLYFEM_THREADING PROPERTY STRINGS "CPP" "TBB" "NONE")
option(POLYFEM_CODE_COVERAGE "Enable coverage reporting" OFF)
option(POLYFEM_WITH_PARAVIEWO "Enable advanced export with paraviewo" OFF)

set(POLYSOLVE_WITH_AMGCL OFF)

add_library(polyfem_coverage_config INTERFACE)
if(POLYFEM_CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Add required flags (GCC & LLVM/Clang)
  target_compile_options(polyfem_coverage_config INTERFACE
    -g         # generate debug info
    --coverage # sets all required flags
  )
  target_link_options(polyfem_coverage_config INTERFACE --coverage)
endif()

# Set default minimum C++ standard
if(POLYFEM_TOPLEVEL_PROJECT)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

### Configuration
set(POLYFEM_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/polyfem")
set(POLYFEM_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src")

# set(CPM_polysolve_SOURCE "/Users/zizhouhuang/Desktop/polysolve")
# set(CPM_ipc-toolkit_SOURCE "/Users/zizhouhuang/Desktop/ipc-toolkit")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/polyfem/")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/recipes/")

# General CMake utils
include(polyfem_autogen)
include(polyfem_cpm_cache)
include(polyfem_use_colors)

# Generate position independent code by default
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

################################################################################
# Polyfem library
################################################################################

# Add an empty library and fill in the list of sources in `src/CMakeLists.txt`.
add_library(polyfem)
add_library(polyfem::polyfem ALIAS polyfem)

target_link_libraries(polyfem PUBLIC polyfem_coverage_config)

# Add source and header files to polyfem
add_subdirectory("${POLYFEM_SOURCE_DIR}")

# Public include directory for PolyFEM
target_include_directories(polyfem PUBLIC "${POLYFEM_INCLUDE_DIR}")

# Use C++17
target_compile_features(polyfem PUBLIC cxx_std_17)

# No limit yay
target_compile_definitions(polyfem PUBLIC -DEIGEN_STACK_ALLOCATION_LIMIT=0)

# 8MB
# target_compile_definitions(polyfem PUBLIC -DEIGEN_STACK_ALLOCATION_LIMIT=8388608)

# Max stack-size small vectors (for gradient and Hessian)
target_compile_definitions(polyfem PUBLIC -DPOLYFEM_SMALL_N=${POLYFEM_SMALL_N})
target_compile_definitions(polyfem PUBLIC -DPOLYFEM_BIG_N=${POLYFEM_BIG_N})

# llvm-mingw links libc++ and libunwind as DLLs by default. Those are toolchain
# files, not build outputs, so they never reach the artifact, and the bridge then
# fails to load on any machine without the toolchain -- which is every machine the
# binary is shipped to. Link them in rather than shipping two more files.
function(polyfem_static_cxx_runtime target)
    if(WIN32 AND NOT MSVC)
        # Plain -static only. -static-libstdc++ sends clang's mingw driver looking
        # for libmsvcprt.a, which llvm-mingw does not ship, and -static-libgcc is
        # a GCC flag with no libgcc here to match it.
        target_link_options(${target} PRIVATE -static)
    endif()
endfunction()

set(POLYFEM_JSON_SPEC_DIR "${PROJECT_SOURCE_DIR}/json-specs")
target_compile_definitions(polyfem PUBLIC POLYFEM_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_compile_definitions(polyfem PUBLIC POLYFEM_JSON_SPEC_DIR="${POLYFEM_JSON_SPEC_DIR}")
target_compile_definitions(polyfem PUBLIC POLYFEM_INPUT_SPEC="${POLYFEM_JSON_SPEC_DIR}/input-spec.json")

if (MSVC)
    add_compile_options(/bigobj)
endif ()

################################################################################
# CUDA
################################################################################

if(IPC_TOOLKIT_WITH_CUDA OR POLYSOLVE_WITH_CUSOLVER)
    include(CheckLanguage)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
    else()
        message(FATAL_ERROR "No CUDA support found!")
    endif()

    set_target_properties(polyfem PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
        set(CMAKE_CUDA_ARCHITECTURES "native")
        set_target_properties(ipc_toolkit PROPERTIES CUDA_ARCHITECTURES "native")
    else()
        include(FindCUDA/select_compute_arch)
        CUDA_DETECT_INSTALLED_GPUS(CUDA_ARCH_LIST)
        string(STRIP "${CUDA_ARCH_LIST}" CUDA_ARCH_LIST)
        string(REPLACE " " ";" CUDA_ARCH_LIST "${CUDA_ARCH_LIST}")
        string(REPLACE "." "" CUDA_ARCH_LIST "${CUDA_ARCH_LIST}")
        set(CMAKE_CUDA_ARCHITECTURES ${CUDA_ARCH_LIST})
        set_target_properties(ipc_toolkit PROPERTIES CUDA_ARCHITECTURES "${CUDA_ARCH_LIST}")
    endif()
endif()

################################################################################
# Required libraries
################################################################################

if(WIN32)
    # Eigen
    include(eigen)
    target_link_libraries(polyfem PUBLIC Eigen3::Eigen)
endif()

# OpenVDB is not linked: FitForm samples a lazily filled brick-grid SDF
# (garment_forms/SdfGrid) through a Lean-emitted spline kernel
# (garment_forms/SdfSpline). The kernel's source of truth is
# V-Sekai-fire/interactor-dress-on lean/Fit/SdfSplineHessian.lean; its slangc
# cpp output is shipped in garment_forms/kernels (see the README there). FIT_KERNELS_PENDING=ON leaves it out
# and the sampler throws when called (the negative control).
option(FIT_KERNELS_PENDING "Leave the Lean SDF spline kernel out (the sampler throws)" OFF)
set(FIT_KERNELS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/polyfem/solver/forms/garment_forms/kernels" CACHE PATH "Directory holding sdf_spline_hessian_emit.cpp")
set(FIT_SLANG_PRELUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/polyfem/solver/forms/garment_forms/kernels/slang-rt" CACHE PATH "Directory holding slang-cpp-prelude.h")
if(FIT_KERNELS_PENDING)
    target_compile_definitions(polyfem PRIVATE FIT_KERNELS_PENDING)
else()
    if(NOT EXISTS "${FIT_KERNELS_DIR}/sdf_spline_hessian_emit.cpp")
        message(FATAL_ERROR "FIT_KERNELS_PENDING=OFF needs FIT_KERNELS_DIR with sdf_spline_hessian_emit.cpp")
    endif()
    if(NOT EXISTS "${FIT_SLANG_PRELUDE_DIR}/slang-cpp-prelude.h")
        message(FATAL_ERROR "FIT_KERNELS_PENDING=OFF needs FIT_SLANG_PRELUDE_DIR with slang-cpp-prelude.h")
    endif()
    # SYSTEM: slangc's cpp prelude is third-party code; its interfaces trip
    # this project's -Werror=non-virtual-dtor.
    target_include_directories(polyfem SYSTEM PRIVATE "${FIT_KERNELS_DIR}" "${FIT_SLANG_PRELUDE_DIR}")
endif()

# polysolve
include(polysolve)
target_link_libraries(polyfem PUBLIC polysolve::polysolve)

# Stage every JSON spec the solve validates against into one directory beside the
# built libraries, so a shipped binary carries its own specs. The paths compiled
# in above name the machine that built it and exist nowhere else; SpecPaths.cpp
# looks here first and falls back to them.
set(POLYFEM_SPEC_STAGE "${CMAKE_BINARY_DIR}/json-specs")
file(GLOB POLYFEM_SPEC_FILES "${POLYFEM_JSON_SPEC_DIR}/*.json")
if(DEFINED polysolve_SOURCE_DIR)
    file(GLOB POLYSOLVE_SPEC_FILES "${polysolve_SOURCE_DIR}/*-solver-spec.json")
    list(APPEND POLYFEM_SPEC_FILES ${POLYSOLVE_SPEC_FILES})
endif()
file(MAKE_DIRECTORY "${POLYFEM_SPEC_STAGE}")
file(COPY ${POLYFEM_SPEC_FILES} DESTINATION "${POLYFEM_SPEC_STAGE}")
list(LENGTH POLYFEM_SPEC_FILES POLYFEM_SPEC_COUNT)
message(STATUS "PolyFEM: staged ${POLYFEM_SPEC_COUNT} JSON spec(s) into ${POLYFEM_SPEC_STAGE}")

# libigl
include(libigl)
target_link_libraries(polyfem PUBLIC igl::core)

include(finite_diff)
target_link_libraries(polyfem PUBLIC finitediff::finitediff)

if(NOT WIN32)
    # Eigen
    include(eigen)
    target_link_libraries(polyfem PUBLIC Eigen3::Eigen)
endif()

# IPC Toolkit
include(ipc_toolkit)
target_link_libraries(polyfem PUBLIC ipc::toolkit)

# Threading
if(POLYFEM_THREADING STREQUAL "CPP")
    target_compile_definitions(polyfem PUBLIC POLYFEM_WITH_CPP_THREADS)
    # These definitions help avoid problems with GCC using TBB backend
    target_compile_definitions(polyfem PUBLIC PSTL_USE_PARALLEL_POLICIES=0)
    target_compile_definitions(polyfem PUBLIC _GLIBCXX_USE_TBB_PAR_BACKEND=0)
elseif(POLYFEM_THREADING STREQUAL "TBB")
    include(onetbb)
    target_link_libraries(polyfem PUBLIC TBB::tbb)
    target_compile_definitions(polyfem PUBLIC -DPOLYFEM_WITH_TBB)
endif()

# paraviewo (includes HDF5 and h5pp)
if(POLYFEM_WITH_PARAVIEWO)
    include(paraviewo)
    target_link_libraries(polyfem PUBLIC paraviewo::paraviewo)
    target_compile_definitions(polyfem PUBLIC -DPOLYFEM_WITH_PARAVIEWO)
endif()

# spdlog
include(spdlog)
target_link_libraries(polyfem PUBLIC spdlog::spdlog)

# JSON Specification Engine library
include(jse)
target_link_libraries(polyfem PUBLIC jse::jse)

# OpenUSD (optional): a C-ABI bridge dlopen'd at runtime, NOT linked into PolyFEM.
#

# The cloth_fit_usd bridge (SHARED) is the ONLY unit that links usd_ms; it exports
# only its extern "C" CFUSD_API surface (everything pxr/TBB hidden). generate_stubs
# turns cloth_fit_usd.sigs into a dlsym dispatch table; libpolyfem calls the weak
# cfusd_* symbols and consumers dlopen the bridge at runtime -> zero USD/TBB on the
# NIF's link line, no ABI coupling, no shared-TBB gymnastics (PolyFEM keeps static
# TBB; the bridge isolates USD's TBB in its own DLL).
if(POLYFEM_WITH_USD)
    if(NOT POLYFEM_USD_ROOT)
        message(FATAL_ERROR "POLYFEM_WITH_USD=ON requires -DPOLYFEM_USD_ROOT=<openusd tree>")
    endif()
    find_library(POLYFEM_USD_MS_LIB
        NAMES usd_ms libusd_ms
        PATHS "${POLYFEM_USD_ROOT}/lib" NO_DEFAULT_PATH REQUIRED)

    set(CFUSD_DIR "${PROJECT_SOURCE_DIR}/src/usd_bridge")

    # 1. The bridge — the only unit that links usd_ms. Exports only CFUSD_API.
    add_library(cloth_fit_usd SHARED
        "${CFUSD_DIR}/cloth_fit_usd.cpp"
        "${POLYFEM_SOURCE_DIR}/io/USDReader.cpp"
        "${POLYFEM_SOURCE_DIR}/io/USDWriter.cpp")
    target_compile_features(cloth_fit_usd PRIVATE cxx_std_17)
    set_target_properties(cloth_fit_usd PROPERTIES
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON
        POSITION_INDEPENDENT_CODE ON)
    target_include_directories(cloth_fit_usd PRIVATE
        "${POLYFEM_INCLUDE_DIR}" "${CFUSD_DIR}" "${POLYFEM_USD_ROOT}/include")
    # CFUSD_BUILDING_DLL is set inside cloth_fit_usd.cpp itself.
    target_compile_definitions(cloth_fit_usd PRIVATE
        POLYFEM_WITH_USD
        POLYFEM_USD_PLUGIN_PATH="${POLYFEM_USD_ROOT}/lib/usd")
    target_link_libraries(cloth_fit_usd PRIVATE "${POLYFEM_USD_MS_LIB}")
    polyfem_static_cxx_runtime(cloth_fit_usd)
    # Find usd_ms both at the build-time usd tree and next to the bridge once it is
    # bundled into priv/ (relocatable).
    if(APPLE)
        set(CFUSD_ORIGIN "@loader_path")
    else()
        set(CFUSD_ORIGIN "$ORIGIN")
    endif()
    set_property(TARGET cloth_fit_usd APPEND PROPERTY
        BUILD_RPATH "${POLYFEM_USD_ROOT}/lib;${CFUSD_ORIGIN}")

    # 2-3. Consumers bind cfusd_* + carry the loader. The mechanism differs by OS.
    if(WIN32)
        # llvm-mingw has no dlfcn, so the POSIX dlsym stub can't compile. Instead
        # consumers bind cfusd_* via the bridge's auto-generated import lib
        # (load-time), and the loader's _WIN32 branch LoadLibraryEx's the bridge so
        # its deps (usd_ms.dll, tbb) resolve. Still zero usd_ms on the NIF link line.
        add_library(cloth_fit_usd_stub STATIC "${CFUSD_DIR}/loader.cpp")
        target_include_directories(cloth_fit_usd_stub PUBLIC "${CFUSD_DIR}")
        set_target_properties(cloth_fit_usd_stub PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_link_libraries(polyfem PUBLIC cloth_fit_usd cloth_fit_usd_stub)
    else()
        # POSIX: generate the dlsym dispatch table from the .sigs; consumers link a
        # stub with weak cfusd_* forwarders + the loader and dlopen the bridge.
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        set(CFUSD_GEN "${CMAKE_BINARY_DIR}/usd_bridge_generated")
        add_custom_command(
            OUTPUT "${CFUSD_GEN}/cloth_fit_usd_stubs.cc" "${CFUSD_GEN}/cfusd/cloth_fit_usd_stubs.h"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CFUSD_GEN}/cfusd"
            COMMAND ${Python3_EXECUTABLE}
                    "${PROJECT_SOURCE_DIR}/thirdparty/generate_stubs/generate_stubs.py"
                    -o "${CFUSD_GEN}" -i "${CFUSD_GEN}" -t posix_stubs
                    -s cloth_fit_usd_stubs -p cfusd -l "std::cerr" -n iostream
                    --macro-include "${CFUSD_DIR}/stubgen_compat.h"
                    "${CFUSD_DIR}/cloth_fit_usd.sigs"
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${CFUSD_GEN}/cloth_fit_usd_stubs.h" "${CFUSD_GEN}/cfusd/cloth_fit_usd_stubs.h"
            DEPENDS "${CFUSD_DIR}/cloth_fit_usd.sigs" "${CFUSD_DIR}/stubgen_compat.h"
            COMMENT "Generating cloth_fit_usd dlopen stubs")

        add_library(cloth_fit_usd_stub STATIC
            "${CFUSD_GEN}/cloth_fit_usd_stubs.cc"
            "${CFUSD_DIR}/loader.cpp")
        target_include_directories(cloth_fit_usd_stub PUBLIC "${CFUSD_GEN}" "${CFUSD_DIR}")
        set_target_properties(cloth_fit_usd_stub PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_link_libraries(cloth_fit_usd_stub PUBLIC ${CMAKE_DL_LIBS})
        target_link_libraries(polyfem PUBLIC cloth_fit_usd_stub)
    endif()

    # 4. libpolyfem calls the C ABI (no usd_ms); consumers get the stub transitively.
    target_include_directories(polyfem PUBLIC "${CFUSD_DIR}")
    target_compile_definitions(polyfem PUBLIC POLYFEM_WITH_USD)
    message(STATUS "PolyFEM: OpenUSD via bridge (${POLYFEM_USD_MS_LIB})")
endif()

# The weftfit_retarget SHARED bridge — the only unit exposing the garment solve
# (garment core's run_retarget) via a narrow extern "C" WFRT_API surface, so
# consumers (the NIF/CLI, viewer, other bindings) dlopen it and link ZERO PolyFEM/
# TBB. Mirrors the cloth_fit_usd bridge; only built with the garment domain.
if(POLYFEM_WITH_GARMENT)
    set(WFRT_DIR "${PROJECT_SOURCE_DIR}/src/retarget_bridge")
    add_library(weftfit_retarget SHARED "${WFRT_DIR}/weftfit_retarget.cpp")
    target_compile_features(weftfit_retarget PRIVATE cxx_std_17)
    set_target_properties(weftfit_retarget PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
    target_include_directories(weftfit_retarget PRIVATE "${WFRT_DIR}")
    # libpolyfem carries run_retarget; only WFRT_API is exported (PolyFEM/TBB hidden).
    target_link_libraries(weftfit_retarget PRIVATE polyfem)
    polyfem_static_cxx_runtime(weftfit_retarget)
    # The bridge's own USD writing (inside run_retarget) goes through the
    # cloth_fit_usd bridge, which polyfem PUBLIC-links. On Windows delay-load it so
    # weftfit_retarget.dll loads without usd_ms on the search path; run_retarget's
    # cfusd_loader::load_from_env() LoadLibraryEx's it from priv/ before first use.
    if(WIN32 AND POLYFEM_WITH_USD)
        target_link_options(weftfit_retarget PRIVATE "-Wl,-delayload,libcloth_fit_usd.dll")
    endif()
    # Expose the bridge's C ABI + loader headers to consumers built outside this
    # tree (the NIF Makefile reuses polyfem's include set), mirroring usd_bridge.
    target_include_directories(polyfem PUBLIC "${WFRT_DIR}")
    # Find the bridge next to the consumer once bundled into priv/ (relocatable).
    if(APPLE)
        set(WFRT_ORIGIN "@loader_path")
    else()
        set(WFRT_ORIGIN "$ORIGIN")
    endif()
    set_property(TARGET weftfit_retarget APPEND PROPERTY BUILD_RPATH "${WFRT_ORIGIN}")
    message(STATUS "PolyFEM: weftfit_retarget solve bridge")

    # Consumers (the NIF/CLI) bind wf_retarget_* + carry the loader, and link ZERO
    # PolyFEM/TBB. As with cloth_fit_usd, the mechanism differs by OS.
    if(WIN32)
        # llvm-mingw has no dlfcn, so the POSIX dlsym stub can't compile. The
        # consumer binds wf_retarget_* via the bridge's auto-generated import lib
        # (delay-loaded on the consumer link line), and the loader LoadLibraryEx's
        # the bridge from next-to-self. Still zero PolyFEM on the consumer link line.
        add_library(weftfit_retarget_stub STATIC "${WFRT_DIR}/loader.cpp")
        target_include_directories(weftfit_retarget_stub PUBLIC "${WFRT_DIR}")
        set_target_properties(weftfit_retarget_stub PROPERTIES POSITION_INDEPENDENT_CODE ON)
    else()
        # POSIX: generate the dlsym dispatch table from the .sigs; the consumer links
        # a stub with weak wf_retarget_* forwarders + the loader and dlopen's the
        # bridge (RTLD_GLOBAL) so the exported symbols satisfy the forwarders.
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        set(WFRT_GEN "${CMAKE_BINARY_DIR}/retarget_bridge_generated")
        add_custom_command(
            OUTPUT "${WFRT_GEN}/weftfit_retarget_stubs.cc" "${WFRT_GEN}/wfrt/weftfit_retarget_stubs.h"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${WFRT_GEN}/wfrt"
            COMMAND ${Python3_EXECUTABLE}
                    "${PROJECT_SOURCE_DIR}/thirdparty/generate_stubs/generate_stubs.py"
                    -o "${WFRT_GEN}" -i "${WFRT_GEN}" -t posix_stubs
                    -s weftfit_retarget_stubs -p wfrt -l "std::cerr" -n iostream
                    --macro-include "${WFRT_DIR}/stubgen_compat.h"
                    "${WFRT_DIR}/weftfit_retarget.sigs"
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${WFRT_GEN}/weftfit_retarget_stubs.h" "${WFRT_GEN}/wfrt/weftfit_retarget_stubs.h"
            DEPENDS "${WFRT_DIR}/weftfit_retarget.sigs" "${WFRT_DIR}/stubgen_compat.h"
            COMMENT "Generating weftfit_retarget dlopen stubs")

        add_library(weftfit_retarget_stub STATIC
            "${WFRT_GEN}/weftfit_retarget_stubs.cc"
            "${WFRT_DIR}/loader.cpp")
        target_include_directories(weftfit_retarget_stub PUBLIC "${WFRT_GEN}" "${WFRT_DIR}")
        set_target_properties(weftfit_retarget_stub PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_link_libraries(weftfit_retarget_stub PUBLIC ${CMAKE_DL_LIBS})
    endif()
endif()

################################################################################
# Optional libraries
################################################################################

# Extra warnings (link this here so it has top priority)
include(polyfem_warnings)
target_link_libraries(polyfem PRIVATE polyfem::warnings)

################################################################################
# Polyfem binary
################################################################################

# Main executable — the garment retargeting CLI (main.cpp constructs GarmentSolver
# + the garment forms), so it belongs to the garment domain layer.
if(POLYFEM_TOPLEVEL_PROJECT AND POLYFEM_WITH_GARMENT)
    add_executable(${PROJECT_NAME}_bin src/polyfem/main.cpp)

    target_compile_features(${PROJECT_NAME}_bin PUBLIC cxx_std_14)

    target_link_libraries(${PROJECT_NAME}_bin PUBLIC polyfem::polyfem polyfem::warnings)

    include(cli11)
    target_link_libraries(${PROJECT_NAME}_bin PUBLIC CLI11::CLI11)
    polyfem_static_cxx_runtime(${PROJECT_NAME}_bin)

    if(NOT (${CMAKE_VERSION} VERSION_LESS "3.6.0"))
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}_bin)
    endif()
endif()

################################################################################
# Tests
################################################################################

# Compile extras only if this is a top-level project
if(POLYFEM_TOPLEVEL_PROJECT AND POLYFEM_WITH_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
