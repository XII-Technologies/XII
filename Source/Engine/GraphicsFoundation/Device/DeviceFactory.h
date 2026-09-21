/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Types/Delegate.h>

#include <GraphicsFoundation/Device/Device.h>

/// This describes the graphics device implementation.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceImplementationDescription
{
  xiiEnum<xiiGALGraphicsDeviceType> m_APIType = xiiGALGraphicsDeviceType::Null; ///< The graphics API type of the implementation.
  xiiString                         m_sName;                                    ///< The name of the implementation, e.g. "Vulkan", "Direct3D 12", etc.
  xiiString                         m_sShaderModel;                             ///< The shader model supported by the implementation, e.g. "VK_SM67", etc.
  xiiString                         m_sShaderCompiler;                          ///< The shader compiler to use for this implementation, e.g. "xiiShaderCompilerSPIRV", "xiiShaderCompilerDXIL".
};

struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceFactory
{
  using CreatorFunc = xiiDelegate<xiiInternal::NewInstance<xiiGALDevice>(xiiAllocator*, const xiiGALDeviceCreationDescription&)>;

  static xiiSharedPtr<xiiGALDevice> CreateDevice(xiiStringView sImplementationName, xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& description);

  static void RegisterImplementation(xiiStringView sImplementationName, const CreatorFunc& func, const xiiGALDeviceImplementationDescription& description);

  static void UnregisterImplementation(xiiStringView sImplementationName);

  static void GetShaderModelAndCompiler(xiiStringView sRendererName, xiiStringView& ref_sShaderModel, xiiStringView& ref_sShaderCompiler);
};
