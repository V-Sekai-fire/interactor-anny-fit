set(SOURCES
	MatrixIO.cpp
	MatrixIO.hpp
	OBJReader.cpp
	OBJReader.hpp
	OBJWriter.cpp
	OBJWriter.hpp
)

# OpenUSD mesh I/O (USDReader/USDWriter) is built as a separate polyfem_usd
# library in the top-level CMakeLists (so it does not inherit PolyFEM's strict
# -Werror set, which USD's own headers do not satisfy).

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})
target_sources(polyfem PRIVATE ${SOURCES})
