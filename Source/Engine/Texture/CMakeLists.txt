xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME} EXCLUDE_FOLDER_FOR_UNITY DirectXTex)

xii_enable_strict_warnings(${PROJECT_NAME})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /wd4005)
endif()

if(XII_CMAKE_COMPILER_CLANG)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-pragmas -Wno-unused-const-variable)
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation

  PRIVATE
  stb
)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_HAS_TEXTURE)

if(XII_CMAKE_PLATFORM_LINUX)
  set(THIRDPARTY_DIR "${XII_ROOT}/Source/ThirdParty")
  target_include_directories(${PROJECT_NAME} PRIVATE "${THIRDPARTY_DIR}/DirectXMath/include" "${THIRDPARTY_DIR}/sal")

  if(XII_3RDPARTY_BC7ENC_RDO_SUPPORT)
    target_link_libraries(${PROJECT_NAME} PRIVATE bc7enc_rdo)
  endif()
endif()

if(XII_3RDPARTY_TINYEXR_SUPPORT)
  target_link_libraries(${PROJECT_NAME} PRIVATE tinyexr)
endif()
