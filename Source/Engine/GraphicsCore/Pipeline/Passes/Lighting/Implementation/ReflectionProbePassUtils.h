/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/StringBuilder.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/ReflectionProbeFrameData.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>

namespace xiiReflectionProbePassUtils
{
  struct Bindings
  {
    xiiSharedPtr<const xiiReflectionProbeFrameData> m_Snapshot;
    xiiRenderGraphBufferHandle                      m_Records, m_Constants;
    xiiRenderGraphBufferHandle                      m_SkyConstants;
    xiiRenderGraphTextureHandle                     m_SkyRadiance, m_SkyIrradiance;
    xiiRenderGraphTextureHandle                     m_Textures[XII_MAX_REFLECTION_PROBES];
    xiiRenderGraphTextureHandle                     m_Irradiance[XII_MAX_REFLECTION_PROBES];
  };

  inline Bindings Read(const xiiSharedPtr<const xiiReflectionProbeFrameData>& snapshot, xiiRenderGraphBuilder& builder)
  {
    Bindings result;
    result.m_Snapshot = snapshot;
    if (!snapshot->m_Textures.IsEmpty())
    {
      result.m_Records   = builder.ReadBuffer(snapshot->m_Records, xiiGALResourceStateFlags::ShaderResource);
      result.m_Constants = builder.ReadBuffer(snapshot->m_Constants, xiiGALResourceStateFlags::ConstantBuffer);
    }
    result.m_SkyConstants = builder.ReadBuffer(snapshot->m_SkyConstants, xiiGALResourceStateFlags::ConstantBuffer);
    // Bind an already retained local image when the shader's sky branch is disabled.
    const auto& sky = snapshot->m_Sky ? snapshot->m_Sky : snapshot->m_Environments[0];
    builder.WaitForFence(sky->m_Ready, 1).AssertSuccess();
    result.m_SkyRadiance   = builder.ReadTexture(builder.ImportTexture("Sky Specular Cubemap", sky->m_Specular, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource);
    result.m_SkyIrradiance = builder.ReadTexture(builder.ImportTexture("Sky Irradiance Cubemap", sky->m_Irradiance, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource);
    for (xiiUInt32 i = 0; i < snapshot->m_Textures.GetCount(); ++i)
    {
      xiiStringBuilder name;
      name.SetFormat("Local Reflection Cubemap {}", i);
      result.m_Textures[i]    = builder.ReadTexture(builder.ImportTexture(name, snapshot->m_Textures[i], xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource);
      const auto& environment = snapshot->m_Environments[i];
      builder.WaitForFence(environment->m_Ready, 1).AssertSuccess();
      name.SetFormat("Local Irradiance Cubemap {}", i);
      result.m_Irradiance[i] = builder.ReadTexture(builder.ImportTexture(name, environment->m_Irradiance, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource);
    }
    return result;
  }

  inline void Bind(const Bindings& data, xiiRenderGraphPassContext& execute, xiiBitflags<xiiGALShaderType> stages)
  {
    auto& commands = execute.GetCommandList();
    commands.ResolveAndSetConstantBuffer("xiiSkyLightConstants", execute.GetBuffer(data.m_SkyConstants), stages);
    commands.ResolveAndSetShaderResourceTextureView("g_SkyRadiance", execute.GetTexture(data.m_SkyRadiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), stages);
    commands.ResolveAndSetShaderResourceTextureView("g_SkyIrradiance", execute.GetTexture(data.m_SkyIrradiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), stages);
    if (data.m_Snapshot->m_Textures.IsEmpty())
      return;
    commands.ResolveAndSetConstantBuffer("xiiReflectionProbeConstants", execute.GetBuffer(data.m_Constants), stages);
    commands.ResolveAndSetShaderResourceBufferView("g_ReflectionProbeData", execute.GetBuffer(data.m_Records)->GetDefaultView(xiiGALBufferViewType::ShaderResource), stages);
    xiiSharedPtr<xiiGALTextureView> views[XII_MAX_REFLECTION_PROBES];
    xiiGALTextureView*              images[XII_MAX_REFLECTION_PROBES];
    for (xiiUInt32 i = 0; i < XII_MAX_REFLECTION_PROBES; ++i)
    {
      views[i]  = execute.GetTexture(data.m_Textures[i < data.m_Snapshot->m_Textures.GetCount() ? i : 0])->GetDefaultView(xiiGALTextureViewType::ShaderResource);
      images[i] = views[i].Borrow();
    }
    commands.ResolveAndSetShaderResourceTextureViews("g_ReflectionProbeTextures", 0, images, stages);
    for (xiiUInt32 i = 0; i < XII_MAX_REFLECTION_PROBES; ++i)
    {
      views[i]  = execute.GetTexture(data.m_Irradiance[i < data.m_Snapshot->m_Environments.GetCount() ? i : 0])->GetDefaultView(xiiGALTextureViewType::ShaderResource);
      images[i] = views[i].Borrow();
    }
    commands.ResolveAndSetShaderResourceTextureViews("g_ReflectionProbeIrradiance", 0, images, stages);
  }
} // namespace xiiReflectionProbePassUtils
