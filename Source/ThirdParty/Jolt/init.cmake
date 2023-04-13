######################################
### Jolt support
######################################

set (XII_3RDPARTY_JOLT_SUPPORT ON CACHE BOOL "Whether to add support for the Jolt physics engine.")
mark_as_advanced(FORCE XII_3RDPARTY_JOLT_SUPPORT)

######################################
### xii_requires_jolt()
######################################

macro(xii_requires_jolt)
    xii_requires(XII_3RDPARTY_JOLT_SUPPORT)
endmacro()
