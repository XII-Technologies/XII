#pragma once

#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct GPUTimingScope;

/// Sets profiling marker and GPU timings for the current scope.
class XII_RENDERERFOUNDATION_DLL xiiProfilingScopeAndMarker : public xiiProfilingScope
{
public:
  static GPUTimingScope* Start(xiiGALCommandEncoder* pCommandEncoder, const char* szName);
  static void            Stop(xiiGALCommandEncoder* pCommandEncoder, GPUTimingScope*& ref_pTimingScope);

  xiiProfilingScopeAndMarker(xiiGALCommandEncoder* pCommandEncoder, const char* szName);

  ~xiiProfilingScopeAndMarker();

protected:
  xiiGALCommandEncoder* m_pCommandEncoder;
  GPUTimingScope*       m_pTimingScope;
};

#if XII_ENABLED(XII_USE_PROFILING) || defined(XII_DOCS)

/// \brief Profiles the current scope using the given name and also inserts a marker with the given GALContext.
#  define XII_PROFILE_AND_MARKER(GALContext, szName) xiiProfilingScopeAndMarker XII_CONCAT(_xiiProfilingScope, XII_SOURCE_LINE)(GALContext, szName)

#else

#  define XII_PROFILE_AND_MARKER(GALContext, szName) /*empty*/

#endif
