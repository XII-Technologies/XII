# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

xii_create_target(APPLICATION FileserveTest)
target_link_libraries(FileserveTest PRIVATE FileservePlugin TestFramework)
xii_ci_add_test(FileserveTest HEADLESS)
