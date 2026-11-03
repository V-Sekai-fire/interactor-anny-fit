set(SOURCES
	Form.hpp
	ContactForm.cpp
	ContactForm.hpp
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})
target_sources(polyfem PRIVATE ${SOURCES})

if(POLYFEM_WITH_GARMENT)
	add_subdirectory(garment_forms)
endif()
