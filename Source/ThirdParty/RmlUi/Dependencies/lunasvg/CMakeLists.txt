xii_cmake_init()

xii_requires_rmlui()

set (PROJECT_NAME "lunasvg")

xii_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_UNITY NO_XII_PREFIX)

# override the default IDE folder
xii_set_project_ide_folder(${PROJECT_NAME} "Source/ThirdParty/lunasvg")

target_include_directories(${PROJECT_NAME}
  PRIVATE
  "3rdparty/plutovg"
  "3rdparty/software"

  PUBLIC
  "include"
)

if(XII_CMAKE_COMPILER_MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /wd4456 /wd4458 /wd4701)
endif()