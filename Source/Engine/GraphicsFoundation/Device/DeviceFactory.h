/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Types/Delegate.h>

#include <GraphicsFoundation/Device/Device.h>

struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceImplementationDescription
{
  xiiEnum<xiiGALGraphicsDeviceType> m_APIType = xiiGALGraphicsDeviceType::Undefined;
  xiiString                         m_sShaderModel;
  xiiString                         m_sShaderCompiler;
};

struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceFactory
{
  using CreatorFunc = xiiDelegate<xiiInternal::NewInstance<xiiGALDevice>(xiiAllocator*, const xiiGALDeviceCreationDescription&)>;

  static xiiSharedPtr<xiiGALDevice> CreateDevice(xiiStringView sImplementationName, xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& description);

  static void RegisterImplementation(xiiStringView sImplementationName, const CreatorFunc& func, const xiiGALDeviceImplementationDescription& description);

  static void UnregisterImplementation(xiiStringView sImplementationName);

  static void GetShaderModelAndCompiler(xiiStringView sRendererName, xiiStringView& ref_sShaderModel, xiiStringView& ref_sShaderCompiler);
};
