#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GRAPHICSFOUNDATION_LIB
#    define XII_GRAPHICSFOUNDATION_DLL XII_DECL_EXPORT
#  else
#    define XII_GRAPHICSFOUNDATION_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_GRAPHICSFOUNDATION_DLL
#endif

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>

////////// Forward Declarations //////////

struct xiiGALBLASTriangleDescription;
struct xiiGALBLASBoundingBoxDescription;
struct xiiGALBottomLevelASCreationDescription;
struct xiiGALScratchBufferSizeDescription;

class xiiGALDevice;
class xiiGALBottomLevelAS;
