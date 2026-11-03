# anny_fit.elf, the godot-sandbox guest, from this checkout and one sibling checkout under WEFT_ROOT:
#   2-contract/guest-runtime  the guest API (vendor/sandbox-api) and rd_compute
# The solvers come from vendor/solve (a git subtree of sinew-mocap/solve): the in-guest L-BFGS-B, its CPU and RenderingDevice vector backends,
# and the baked-phenotype dimensions. Build:
#   cmake -S guest -B build/rv64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=<riscv64-sysroot>/toolchain.cmake -DWEFT_ROOT=<manifest root>
#   cmake --build build/rv64 --target anny_fit
# The viewer is a double-precision Godot, so the guest API is built with DOUBLE_PRECISION (a single-precision ELF is refused at load).
# The fit's sources are compiled uncontracted (-ffp-contract=off), as the host harness is.
cmake_minimum_required(VERSION 3.16)
project(anny_fit_guest C CXX)
if(CMAKE_C_COMPILER_TARGET AND NOT CMAKE_ASM_COMPILER_TARGET)
	set(CMAKE_ASM_COMPILER_TARGET ${CMAKE_C_COMPILER_TARGET})
endif()
enable_language(ASM)
set(WEFT_ROOT "" CACHE PATH "The goal manifest's root (holds 2-contract/)")
if(NOT WEFT_ROOT)
	message(FATAL_ERROR "set -DWEFT_ROOT=<manifest root>")
endif()
get_filename_component(AF_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(GR "${WEFT_ROOT}/2-contract/guest-runtime")
set(SOLVE "${AF_ROOT}/vendor/solve/core")
set(LB "${SOLVE}/lbfgsb")
foreach(f "${GR}/vendor/sandbox-api/cmake/CMakeLists.txt" "${GR}/guest/rd_compute.cpp" "${LB}/guest/drape/lbfgsb.cpp" "${LB}/guest/drape/vec_rd.cpp")
	if(NOT EXISTS "${f}")
		message(FATAL_ERROR "missing ${f}")
	endif()
endforeach()
add_compile_options("-ffile-prefix-map=${WEFT_ROOT}/=" "-ffile-prefix-map=${AF_ROOT}/=")
set(DOWNLOAD_RUNTIME_API OFF CACHE BOOL "" FORCE)
set(DOUBLE_PRECISION ON CACHE BOOL "" FORCE)
add_subdirectory("${GR}/vendor/sandbox-api/cmake" sandbox_api)
set(AF_SOURCES "${AF_ROOT}/guest/anny_fit/main.cpp" "${AF_ROOT}/guest/anny_fit/pheno_weights.cpp")
add_sandbox_program(anny_fit ${AF_SOURCES} "${LB}/guest/drape/lbfgsb.cpp" "${LB}/guest/drape/vec_cpu.cpp" "${LB}/guest/drape/vec_rd.cpp"
	"${GR}/guest/rd_compute.cpp")
set_source_files_properties(${AF_SOURCES} PROPERTIES COMPILE_OPTIONS "-ffp-contract=off")
target_include_directories(anny_fit PRIVATE "${AF_ROOT}/guest/anny_fit" "${AF_ROOT}/guest/drape_support" "${SOLVE}" "${LB}/guest/drape" "${LB}/guest" "${GR}/guest" "${GR}/guest/avbd/slang-rt")
target_link_libraries(anny_fit PRIVATE -lm -lstdc++)
