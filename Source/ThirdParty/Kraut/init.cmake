######################################
### Jolt support
######################################

set (XII_3RDPARTY_KRAUT_SUPPORT ON CACHE BOOL "Whether to add support for procedurally generated trees with Kraut.")
mark_as_advanced(FORCE XII_3RDPARTY_KRAUT_SUPPORT)

######################################
### xii_requires_kraut()
######################################

macro(xii_requires_kraut)

	xii_requires(XII_3RDPARTY_KRAUT_SUPPORT)

endmacro()
