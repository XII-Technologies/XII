# Copyright (c) Theophilus Eriata. All Rights Reserved.

# #####################################
# ## xii_ci_add_to_targets_list(<target> [C++])
# #####################################

function(xii_ci_add_to_targets_list TARGET_NAME LANGUAGE)
  if(XII_NO_TXT_FILES)
    return()
  endif()

  file(RELATIVE_PATH REL_PATH_TO_FOLDER ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

  get_target_property(LINK_LIBS ${TARGET_NAME} LINK_LIBRARIES)

  file(APPEND ${CMAKE_BINARY_DIR}/Targets.txt "${TARGET_NAME}|${REL_PATH_TO_FOLDER}|0|${LANGUAGE}|${LINK_LIBS}\n")
endfunction()

# #####################################
# ## xii_ci_add_test(<target> [HEADLESS | NEEDS_HW_ACCESS])
# #####################################
function(xii_ci_add_test TARGET_NAME)
  set(ARG_OPTIONS HEADLESS NEEDS_HW_ACCESS)
  set(ARG_ONEVALUEARGS "")
  set(ARG_MULTIVALUEARGS "")
  cmake_parse_arguments(ARG "${ARG_OPTIONS}" "${ARG_ONEVALUEARGS}" "${ARG_MULTIVALUEARGS}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "xii_ci_add_test: Invalid arguments '${ARG_UNPARSED_ARGUMENTS}'")
  endif()

  if(ARG_HEADLESS AND ARG_NEEDS_HW_ACCESS)
    message(FATAL_ERROR "A test cannot be both HEADLESS and NEEDS_HW_ACCESS")
  endif()

  if(XII_NO_TXT_FILES)
    return()
  endif()

  if(${ARG_NEEDS_HW_ACCESS})
    set(HWA_VALUE 1)
  else()
    set(HWA_VALUE 0)
  endif()

  file(APPEND ${CMAKE_BINARY_DIR}/Tests.txt "${TARGET_NAME}|${HWA_VALUE}|0\n")

  # Add CTest integration.
  get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
  if(TARGET_TYPE STREQUAL "EXECUTABLE")
    add_test(
      NAME ${TARGET_NAME}
      COMMAND ${TARGET_NAME} -noGui -run -close -noSave -all -console false -html false
        -sdkRoot "${XII_ROOT}"
        -outputDir "${CMAKE_BINARY_DIR}/TestResults/$<CONFIG>/${TARGET_NAME}"
        -json "${CMAKE_BINARY_DIR}/TestResults/$<CONFIG>/${TARGET_NAME}/results.json"
    )
    # Set test properties.
    set_tests_properties(${TARGET_NAME} PROPERTIES
      TIMEOUT 600
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      RUN_SERIAL TRUE # Suites share filesystem/process fixtures.
    )
    if(ARG_HEADLESS)
      set_tests_properties(${TARGET_NAME} PROPERTIES LABELS "headless")
    elseif(ARG_NEEDS_HW_ACCESS)
      set_tests_properties(${TARGET_NAME} PROPERTIES LABELS "hardware")
    else()
      set_tests_properties(${TARGET_NAME} PROPERTIES LABELS "interactive")
    endif()
  endif()
endfunction()
