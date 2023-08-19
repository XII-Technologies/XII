#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GRAPHICSD3D12_LIB
#    define XII_GRAPHICSD3D12_DLL XII_DECL_EXPORT
#  else
#    define XII_GRAPHICSD3D12_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_GRAPHICSD3D12_DLL
#endif

////////// Forward Declarations //////////

class xiiGALBlendStateD3D12;
class xiiGALDepthStencilStateD3D12;
class xiiGALRasterizerStateD3D12;
