set (XII_BUILD_UNITTESTS ON CACHE BOOL "Whether unit-test projects should be added to the solution")

xii_requires(XII_BUILD_UNITTESTS)

xii_add_all_subdirs()