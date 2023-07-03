#pragma once

#include <Foundation/Types/Delegate.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct XII_RENDERERFOUNDATION_DLL xiiGALDeviceFactory
{
  using CreatorFunc = xiiDelegate<xiiInternal::NewInstance<xiiGALDevice>(xiiAllocatorBase*, const xiiGALDeviceCreationDescription&)>;

  static xiiInternal::NewInstance<xiiGALDevice> CreateDevice(xiiStringView sRendererName, xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& desc);

  static void GetShaderModelAndCompiler(xiiStringView sRendererName, const char*& ref_szShaderModel, const char*& ref_szShaderCompiler);

  static void RegisterCreatorFunc(xiiStringView sRendererName, const CreatorFunc& func, xiiStringView sShaderModel, xiiStringView sShaderCompiler);
  static void UnregisterCreatorFunc(xiiStringView sRendererName);

  static void RegisterLibraryName(xiiStringView sRendererName, xiiStringView sLibraryName);
  static void UnregisterLibraryName(xiiStringView sRendererName);
};
