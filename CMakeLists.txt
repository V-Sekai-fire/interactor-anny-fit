# solve/ — the body-solve cluster: FK phenotype (Lean->Slang, CPU) + LBS skinning
# (Vulkan).  core/ holds the engine; adapters/ holds the bake tooling.  Links the
# gpu cluster's vkc_core.  Self-skips if the baked header is absent.

# Standalone-build entry (cloned side-by-side in ~/); a no-op under the superbuild.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.19)
    project(solve C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    option(SINEW_TESTS "Build the cluster's unit/property tests" OFF)
    if(SINEW_TESTS)
        enable_testing()
    endif()
endif()

# Sibling: the vkc Vulkan compute host lives in mount_drift (ports/sibling-repos.txt).
if(NOT TARGET vkc_core)
    add_subdirectory(../mount_drift ${CMAKE_BINARY_DIR}/_mount_drift)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core/soma_pheno.h")
    add_library(solve_core STATIC
        core/soma_pheno_slang.cpp
        core/vk_lbs_host.cpp)
    target_include_directories(solve_core PUBLIC core ports)
    # Links the gpu cluster's vkc_core for the Vulkan loader.  NOTE (DRY follow-up):
    # vk_lbs_host reimplements the same Vulkan boilerplate as gpu/core/vkc — it
    # should be rebuilt on vkc_pipeline/vkc_run instead of duplicating it.
    target_link_libraries(solve_core PUBLIC vkc_core)
else()
    message(STATUS "solve_core skipped (no baked core/soma_pheno.h)")
endif()
