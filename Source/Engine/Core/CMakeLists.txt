xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  Texture

  PRIVATE
  mikktspace
)

if (XII_3RDPARTY_LUA_SUPPORT)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Lua
  )
endif()

if (XII_3RDPARTY_DUKTAPE_SUPPORT)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Duktape
  )
endif()

if (XII_3RDPARTY_SDL_SUPPORT)
  if(XII_COMPILE_ENGINE_AS_DLL)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2)
  else()
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2-static)
  endif()
  if (XII_CMAKE_PLATFORM_LINUX)
    target_link_libraries(${PROJECT_NAME}
      PRIVATE
      X11-xcb
    )
  endif()
endif()
