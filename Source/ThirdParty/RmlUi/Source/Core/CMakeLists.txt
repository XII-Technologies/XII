xii_cmake_init()

xii_requires_rmlui()

get_property(XII_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY XII_SUBMODULE_PREFIX_PATH)

set (PROJECT_NAME "RmlCore")

xii_create_target(LIBRARY ${PROJECT_NAME} NO_XII_PREFIX)

# override the default IDE folder
xii_set_project_ide_folder(${PROJECT_NAME} "Source/ThirdParty/RmlCore")

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_RMLUI_SUPPORT)

# Don't use standard c++ rtti since it will be disabled in the project
target_compile_definitions(${PROJECT_NAME} PUBLIC RMLUI_USE_CUSTOM_RTTI)

# Enable SVG support
target_compile_definitions(${PROJECT_NAME} PRIVATE RMLUI_ENABLE_SVG_PLUGIN)
target_sources(${PROJECT_NAME} PRIVATE
  ../SVG/ElementSVG.cpp
  ../SVG/SVGPlugin.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../../Include")

# Freetype

if(XII_CMAKE_PLATFORM_WINDOWS)
  set(RMLUI_FREETYPE_PATH ${CMAKE_SOURCE_DIR}/${XII_SUBMODULE_PREFIX_PATH}/Source/ThirdParty/RmlUi/Dependencies/freetype)
  target_include_directories(${PROJECT_NAME} PRIVATE "${RMLUI_FREETYPE_PATH}/Include")

  target_compile_definitions(${PROJECT_NAME} PRIVATE RmlCore_EXPORTS)
  
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(RMLUI_FREETYPE_LIB "${RMLUI_FREETYPE_PATH}/win64")
  else()
    set(RMLUI_FREETYPE_LIB "${RMLUI_FREETYPE_PATH}/win32")
  endif()
  
  target_link_libraries(${PROJECT_NAME} PRIVATE "${RMLUI_FREETYPE_LIB}/freetype.lib")

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RMLUI_FREETYPE_LIB}/freetype.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )
endif()

if(XII_CMAKE_PLATFORM_LINUX)
  find_package(Freetype)
  if(NOT Freetype_FOUND)
    message(FATAL_ERROR "Couldn't find freetype. Check your cmake prefix path or install your systems freetype package (e.g. libfreetype-dev).")
  endif()
  target_link_libraries(${PROJECT_NAME} PRIVATE Freetype::Freetype)
endif()

#lunasvg
target_link_libraries(${PROJECT_NAME} PRIVATE lunasvg)
