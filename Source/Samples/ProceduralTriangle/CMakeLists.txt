xii_cmake_init()

xii_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

xii_add_renderers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GraphicsCore
)

set(PROCEDURAL_TRIANGLE_DATA_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Samples/ProceduralTriangle")
file(GLOB_RECURSE PROCEDURAL_TRIANGLE_DATA "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Samples/ProceduralTriangle/*")

source_group(TREE ${PROCEDURAL_TRIANGLE_DATA_FOLDER} PREFIX "Data" FILES ${PROCEDURAL_TRIANGLE_DATA})
target_sources(${PROJECT_NAME} PRIVATE "${PROCEDURAL_TRIANGLE_DATA}")
target_include_directories(${PROJECT_NAME} PRIVATE "${PROCEDURAL_TRIANGLE_DATA_FOLDER}")

set_source_files_properties(${PROCEDURAL_TRIANGLE_DATA} PROPERTIES HEADER_FILE_ONLY TRUE)
