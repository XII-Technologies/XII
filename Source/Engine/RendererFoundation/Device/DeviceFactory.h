#pragma once

#include <Foundation/Types/Delegate.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct XII_RENDERERFOUNDATION_DLL xiiGALDeviceFactory
{
  using CreatorFunc = xiiDelegate<xiiInternal::NewInstance<xiiGALDevice>(xiiAllocatorBase*, const xiiGALDeviceCreationDescription&)>;

  static xiiInternal::NewInstance<xiiGALDevice> CreateDevice(const char* szRendererName, xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& desc);

  static void GetShaderModelAndCompiler(const char* szRendererName, const char*& szShaderModel, const char*& szShaderCompiler);

  static void RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler);
  static void UnregisterCreatorFunc(const char* szRendererName);

  static void ConfigureLibraryName(const char* szRendererName, const char* szLibraryName);
};
