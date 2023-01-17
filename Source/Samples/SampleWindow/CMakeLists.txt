xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

#TODO: Add actual packaging code. This is done in PRE_BUILD so that it happens before the
#apk gen steps that happen in POST_BUILD and which are already done via xii_create_target. 
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/${XII_SUBMODULE_PREFIX_PATH}/Data/Samples/SampleWindow/ ${CMAKE_CURRENT_BINARY_DIR}/package/Assets/Data/Samples/SampleWindow/)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Core
)
