xii_cmake_init()

xii_requires(XII_CMAKE_PLATFORM_LINUX)

xii_requires(XII_3RDPARTY_TRACELOGGING_LTTNG_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_XII_PREFIX NO_WARNINGS_AS_ERRORS ALL_SYMBOLS_VISIBLE)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_TRACELOGGING_LTTNG_SUPPORT)

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} PRIVATE lttng-ust lttng-ust-common lttng-ust-tracepoint)
