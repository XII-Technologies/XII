#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERDILIGENTD3D11_LIB
#    define XII_RENDERERDILIGENTD3D11_DLL XII_DECL_EXPORT
#  else
#    define XII_RENDERERDILIGENTD3D11_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_RENDERERDILIGENTD3D11_DLL
#endif


#include <Common/interface/RefCntAutoPtr.hpp>

#include <Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <Graphics/GraphicsEngine/interface/SwapChain.h>
