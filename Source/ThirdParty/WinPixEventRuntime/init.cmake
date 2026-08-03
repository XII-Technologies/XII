# Enable support for WinPixEventRuntime. This is a library that allows you to use PIX for Windows to profile your application.
set (XII_3RDPARTY_PIX_EVENT_RUNTIME_SUPPORT ON CACHE BOOL "Whether to add support for WinPixEventRuntime.")
mark_as_advanced(FORCE XII_3RDPARTY_PIX_EVENT_RUNTIME_SUPPORT)

macro(xii_requires_pix_event_runtime)
  xii_requires_development()
  xii_requires(XII_CMAKE_PLATFORM_WINDOWS)
  xii_requires(XII_3RDPARTY_PIX_EVENT_RUNTIME_SUPPORT)
endmacro()
