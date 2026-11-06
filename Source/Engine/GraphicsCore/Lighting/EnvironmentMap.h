/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Status.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// Immutable GGX radiance and Lambert irradiance derived from one resident cubemap.
/// Creation runs on the thread owning the active xiiRenderGraphFrame and submits initialization once.
/// Every consuming queue must wait for m_Ready (value 1).
class XII_GRAPHICSCORE_DLL xiiEnvironmentMap final : public xiiRefCounted
{
public:
  static xiiStatus Create(const xiiSharedPtr<xiiGALTexture>& source, xiiSharedPtr<const xiiEnvironmentMap>& out_map);

  xiiEnvironmentMap(xiiSharedPtr<xiiGALTexture> source, xiiSharedPtr<xiiGALTexture> specular, xiiSharedPtr<xiiGALTexture> irradiance, xiiSharedPtr<xiiGALFence> ready, xiiUInt64 memory);

  const xiiSharedPtr<xiiGALTexture> m_Source, m_Specular, m_Irradiance;
  const xiiSharedPtr<xiiGALFence>   m_Ready;
  const xiiUInt64                   m_uiMemoryGPU;
};
