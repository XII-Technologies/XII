# #####################################
# ## Qt support
# #####################################

set(XII_ENABLE_QT_SUPPORT ON CACHE BOOL "Whether to add Qt support.")
set(XII_QT_DIR $ENV{QTDIR} CACHE PATH "Directory of the Qt installation")

# #####################################
# ## xii_requires_qt()
# #####################################
macro(xii_requires_qt)
  xii_requires(XII_ENABLE_QT_SUPPORT)
endmacro()

# #####################################
# ## xii_find_qt()
# ## invoked once in the root cmake script to locate all requierd QT components.
# #####################################
macro(xii_find_qt)
  xii_prepare_find_qt()

  set(XII_QT_COMPONENTS 
    Widgets
    Core
    Gui
    Widgets
    Network
    Svg
  )

  # XII requires at least Qt 6.3 because earlier versions have a bug which prevents the 3d viewport in the 
  # Editor from working correctly.
  SET(XII_REQUIRED_QT_VERSION "6.3")

  if(XII_ENABLE_QT_SUPPORT)
    if(XII_QT_DIR)
      find_package(Qt6 ${XII_REQUIRED_QT_VERSION} COMPONENTS ${XII_QT_COMPONENTS} REQUIRED PATHS ${XII_QT_DIR})
    else()
      find_package(Qt6 ${XII_REQUIRED_QT_VERSION} COMPONENTS ${XII_QT_COMPONENTS} REQUIRED)
    endif()
  endif()

  message(STATUS "Found Qt6 Version ${Qt6_VERSION} in ${Qt6_DIR}")
  
  mark_as_advanced(FORCE Qt6_DIR)
  mark_as_advanced(FORCE Qt6Core_DIR)
  mark_as_advanced(FORCE Qt6CoreTools_DIR)
  mark_as_advanced(FORCE Qt6Gui_DIR)
  mark_as_advanced(FORCE Qt6GuiTools_DIR)
  mark_as_advanced(FORCE Qt6Widgets_DIR)
  mark_as_advanced(FORCE Qt6WidgetsTools_DIR)
  mark_as_advanced(FORCE Qt6Network_DIR)
  mark_as_advanced(FORCE Qt6Svg_DIR)
  mark_as_advanced(FORCE Qt6EntryPointPrivate_DIR)
  mark_as_advanced(FORCE Qt6ZlibPrivate_DIR)
  mark_as_advanced(FORCE WINDEPLOYQT_EXECUTABLE)
  mark_as_advanced(FORCE QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH)
  mark_as_advanced(FORCE QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
  
  if(XII_CMAKE_PLATFORM_WINDOWS AND XII_CMAKE_COMPILER_CLANG)
    # The qt6 interface compile options contain msvc specific flags which don't exist for clang.
    set_target_properties(Qt6::Platform PROPERTIES INTERFACE_COMPILE_OPTIONS "")
    
    # Qt6 link options include '-NXCOMPAT' which does not exist on clang.
    get_target_property(QtLinkOptions Qt6::PlatformCommonInternal INTERFACE_LINK_OPTIONS)
    string(REPLACE "-NXCOMPAT;" "" QtLinkOptions "${QtLinkOptions}")
    set_target_properties(Qt6::PlatformCommonInternal PROPERTIES INTERFACE_LINK_OPTIONS ${QtLinkOptions})
  endif()
endmacro()

# #####################################
# ## xii_prepare_find_qt()
# #####################################
function(xii_prepare_find_qt)
  set(XII_CACHED_QT_DIR "XII_CACHED_QT_DIR-NOTFOUND" CACHE STRING "")
  mark_as_advanced(XII_CACHED_QT_DIR FORCE)

  # #####################
  # # Download Qt package
  xii_pull_compiler_and_architecture_vars()
  xii_pull_platform_vars()
  xii_pull_config_vars()

  # Currently only implemented for x64
  if(XII_CMAKE_PLATFORM_WINDOWS_DESKTOP AND XII_CMAKE_ARCHITECTURE_64BIT)
    # Upgrade from Qt5 to Qt6 if the XII_QT_DIR points to a previously automatically downloaded Qt5 package.
    if("${XII_QT_DIR}" MATCHES ".*Qt-5\\.13\\.0-vs141-x64")
      set(XII_QT_DIR "XII_QT_DIR-NOTFOUND" CACHE PATH "Directory of the Qt installation" FORCE)
    endif()
  
    if(XII_CMAKE_ARCHITECTURE_64BIT)
      set(XII_SDK_VERSION "${XII_CONFIG_QT_WINX64_VERSION}")
      set(XII_SDK_URL "${XII_CONFIG_QT_WINX64_URL}")
    endif()

    if((XII_QT_DIR STREQUAL "XII_QT_DIR-NOTFOUND") OR(XII_QT_DIR STREQUAL ""))
      xii_download_and_extract("${XII_SDK_URL}" "${CMAKE_BINARY_DIR}" "${XII_SDK_VERSION}")

      set(XII_QT_DIR "${CMAKE_BINARY_DIR}/${XII_SDK_VERSION}" CACHE PATH "Directory of the Qt installation" FORCE)
    endif()
  endif()

  # # Download Qt package
  # #####################
  if(NOT "${XII_QT_DIR}" STREQUAL "${XII_CACHED_QT_DIR}")
    # Need to reset qt vars now so that 'find_package' is re-executed
    set(XII_CACHED_QT_DIR ${XII_QT_DIR} CACHE STRING "" FORCE)

    message(STATUS "Qt-dir has changed, clearing cached Qt paths")

    # Clear cached qt dirs
    set(Qt6_DIR "Qt6_DIR-NOTFOUND" CACHE PATH "" FORCE)
    set(Qt6Core_DIR "Qt6Core_DIR-NOTFOUND" CACHE PATH "" FORCE)
    set(Qt6CoreTools_DIR "Qt6CoreTools_DIR-NOTFOUND" CACHE PATH "" FORCE)
    set(Qt6Gui_DIR "Qt6Gui_DIR-NOTFOUND" CACHE PATH "" FORCE)
    set(Qt6GuiTools_DIR "Qt6GuiTools_DIR-NOTFOUND" CACHE PATH "" FORCE)
    set(Qt6Widgets_DIR "Qt6Widgets_DIR-NOTFOUND" CACHE PATH "" FORCE)
    set(Qt6WidgetsTools_DIR "Qt6WidgetTools_DIR-NOTFOUND" CACHE PATH "" FORCE)
    set(Qt6Network_DIR "Qt6Network_DIR-NOTFOUND" CACHE PATH "" FORCE)
    set(Qt6Svg_DIR "Qt6Svg_DIR-NOTFOUND" CACHE PATH "" FORCE)
    set(Qt6ZlibPrivate_DIR "Qt6ZlibPrivate_DIR-NOTFOUND" CACHE PATH "" FORCE)
    set(Qt6EntryPointPrivate_DIR "Qt6EntryPointPrivate_DIR-NOTFOUND" CACHE PATH "" FORCE)
    set(WINDEPLOYQT_EXECUTABLE "WINDEPLOYQT_EXECUTABLE-NOTFOUND" CACHE FILEPATH "" FORCE)
  endif()

  # force find_package to search for Qt in the correct folder
  if(XII_QT_DIR)
    set(CMAKE_PREFIX_PATH ${XII_QT_DIR} PARENT_SCOPE)
  endif()
endfunction()

# #####################################
# ## xii_link_target_qt(TARGET <target> COMPONENTS <qt components> [COPY_DLLS])
# #####################################
function(xii_link_target_qt)
  xii_pull_all_vars()

  xii_requires_qt()

  set(FN_OPTIONS COPY_DLLS)
  set(FN_ONEVALUEARGS TARGET)
  set(FN_MULTIVALUEARGS COMPONENTS)
  cmake_parse_arguments(FN_ARG "${FN_OPTIONS}" "${FN_ONEVALUEARGS}" "${FN_MULTIVALUEARGS}" ${ARGN})

  if(FN_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "xii_link_target_qt: Invalid arguments '${FN_ARG_UNPARSED_ARGUMENTS}'")
  endif()

  get_property(XII_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY XII_SUBMODULE_PREFIX_PATH)

  file(RELATIVE_PATH SUB_FOLDER "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")

  target_include_directories(${FN_ARG_TARGET} PUBLIC ${CMAKE_BINARY_DIR}/${SUB_FOLDER})

  target_compile_definitions(${FN_ARG_TARGET} PUBLIC XII_USE_QT)

  if(XII_CMAKE_COMPILER_MSVC)
    # Qt6 requires runtime type information (RTTI) in debug builds.
    target_compile_options(${FN_ARG_TARGET} PRIVATE "$<$<CONFIG:Debug>:/GR>")
  endif()

  foreach(module ${FN_ARG_COMPONENTS})
    target_link_libraries(${FN_ARG_TARGET} PUBLIC "Qt6::${module}")

    if(FN_ARG_COPY_DLLS)
      # copy the dll into the binary folder for each configuration using generator expressions
      # as a post-build step for every qt-enabled target:
      add_custom_command(TARGET ${FN_ARG_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Qt6::${module}>
        $<TARGET_FILE_DIR:${FN_ARG_TARGET}>)
    endif()
  endforeach()

  if(XII_QT_DIR AND FN_ARG_COPY_DLLS)
    # Copy 'imageformats' into the binary folder.
    add_custom_command(TARGET ${FN_ARG_TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${XII_QT_DIR}/plugins/imageformats"
      "$<TARGET_FILE_DIR:${FN_ARG_TARGET}>/imageformats")

    # Copy 'platforms' into the binary folder.
    add_custom_command(TARGET ${FN_ARG_TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${XII_QT_DIR}/plugins/platforms"
      "$<TARGET_FILE_DIR:${FN_ARG_TARGET}>/platforms")

    # Copy 'iconengines' into the binary folder.
    add_custom_command(TARGET ${FN_ARG_TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${XII_QT_DIR}/plugins/iconengines"
      "$<TARGET_FILE_DIR:${FN_ARG_TARGET}>/iconengines")
  endif()
endfunction()

# #####################################
# ## xii_qt_wrap_target_ui_files(<target> <files>)
# #####################################
function(xii_qt_wrap_target_ui_files TARGET_NAME FILES_TO_WRAP)
  xii_requires_qt()

  list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.ui$")

  if(NOT FILES_TO_WRAP)
    return()
  endif()

  if(NOT TARGET Qt6::uic)
    message(FATAL_ERROR "UIC.exe not found")
  else()
    # due to the stupid way CMake variable scopes work, and a stupid way Qt sets up the Qt6Widgets_UIC_EXECUTABLE variable,
    # its value gets 'lost' (reset to empty) if find_package has been called in a different function before
    # since it is used internally by Qt6_wrap_ui, we must ensure to restore it here, otherwise the custom build tool command on UI files
    # will miss the uic.exe part and just be "-o stuff" instead of "uic.exe -o stuff"
    get_target_property(Qt6Widgets_UIC_EXECUTABLE Qt6::uic IMPORTED_LOCATION)
  endif()

  Qt6_wrap_ui(UIC_FILES ${FILES_TO_WRAP})

  target_sources(${TARGET_NAME} PRIVATE ${UIC_FILES})

  source_group("Qt\\UI Files" FILES ${UIC_FILES})
endfunction()

# #####################################
# ## xii_qt_wrap_target_moc_files(<target> <files>)
# #####################################
function(xii_qt_wrap_target_moc_files TARGET_NAME FILES_TO_WRAP)
  xii_requires_qt()

  list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.h$")

  if(NOT FILES_TO_WRAP)
    return()
  endif()

  set(Qt6Core_MOC_EXECUTABLE Qt6::moc)
  xii_retrieve_target_pch(${TARGET_NAME} PCH_H)

  if(PCH_H)
    Qt6_wrap_cpp(MOC_FILES TARGET ${TARGET_NAME} ${FILES_TO_WRAP} OPTIONS -b "${PCH_H}.h")
    xii_pch_use("${PCH_H}.h" "${MOC_FILES}")
  else()
    Qt6_wrap_cpp(MOC_FILES TARGET ${TARGET_NAME} ${FILES_TO_WRAP})
  endif()

  target_sources(${TARGET_NAME} PRIVATE ${MOC_FILES})

  source_group("Qt\\MOC Files" FILES ${MOC_FILES})
endfunction()

# #####################################
# ## xii_qt_wrap_target_qrc_files(<target> <files>)
# #####################################
function(xii_qt_wrap_target_qrc_files TARGET_NAME FILES_TO_WRAP)
  xii_requires_qt()

  list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.qrc$")

  if(NOT FILES_TO_WRAP)
    return()
  endif()

  set(Qt6Core_RCC_EXECUTABLE Qt6::rcc)
  Qt6_add_resources(QRC_FILES ${FILES_TO_WRAP})

  target_sources(${TARGET_NAME} PRIVATE ${QRC_FILES})

  source_group("Qt\\QRC Files" FILES ${QRC_FILES})
endfunction()

# #####################################
# ## xii_qt_wrap_target_files(<target> <files>)
# #####################################
function(xii_qt_wrap_target_files TARGET_NAME FILES_TO_WRAP)
  xii_requires_qt()

  xii_qt_wrap_target_qrc_files(${TARGET_NAME} "${FILES_TO_WRAP}")

  xii_qt_wrap_target_ui_files(${TARGET_NAME} "${FILES_TO_WRAP}")

  # in this automated method, we only MOC files that end with ".moc.h"
  list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.moc\.h")
  xii_qt_wrap_target_moc_files(${TARGET_NAME} "${FILES_TO_WRAP}")
endfunction()
