xii_cmake_init()

xii_requires_renderer()

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})
xii_add_renderers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GraphicsCore
)

set(GPU_DRIVEN_SCENE_DATA_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Samples/GpuDrivenScene")
file(GLOB_RECURSE GPU_DRIVEN_SCENE_DATA "${GPU_DRIVEN_SCENE_DATA_FOLDER}/*")

source_group(TREE ${GPU_DRIVEN_SCENE_DATA_FOLDER} PREFIX "Data" FILES ${GPU_DRIVEN_SCENE_DATA})
target_sources(${PROJECT_NAME} PRIVATE "${GPU_DRIVEN_SCENE_DATA}")
target_include_directories(${PROJECT_NAME} PRIVATE "${GPU_DRIVEN_SCENE_DATA_FOLDER}")
set_source_files_properties(${GPU_DRIVEN_SCENE_DATA} PROPERTIES HEADER_FILE_ONLY TRUE)

