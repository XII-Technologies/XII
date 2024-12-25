
set (XII_3RDPARTY_BC7ENC_RDO_SUPPORT ON CACHE BOOL "Whether to add support for the bc7 compression.")
mark_as_advanced(FORCE XII_3RDPARTY_BC7ENC_RDO_SUPPORT)

macro(xii_requires_bc7enc_rdo)
  xii_requires(XII_3RDPARTY_BC7ENC_RDO_SUPPORT)
endmacro()
