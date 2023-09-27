#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>

struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceFactory
{
  using CreatorFunc = xiiDelegate<xiiInternal::NewInstance<xiiGALDevice>(xiiAllocatorBase*, const xiiGALDeviceCreationDescription&)>;

  static xiiInternal::NewInstance<xiiGALDevice> CreateDevice(xiiStringView sRendererName, xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  static void GetShaderModelAndCompiler(xiiStringView sRendererName, const char*& ref_szShaderModel, const char*& ref_szShaderCompiler);

  static void RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler);
  static void UnregisterCreatorFunc(const char* szRendererName);
};

#include <GraphicsFoundation/Device/Implementation/DeviceFactory_inl.h>
