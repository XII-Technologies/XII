set (XII_BUILD_SAMPLES ON CACHE BOOL "Whether sample projects should be added to the solution")

xii_requires(XII_BUILD_SAMPLES)

xii_add_all_subdirs()