set(SOURCES
	optimize.cpp
	optimize.hpp
	GarmentNLProblem.cpp
	GarmentNLProblem.hpp
	run_retarget.cpp
	run_retarget.hpp
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})
target_sources(polyfem PUBLIC ${SOURCES})
