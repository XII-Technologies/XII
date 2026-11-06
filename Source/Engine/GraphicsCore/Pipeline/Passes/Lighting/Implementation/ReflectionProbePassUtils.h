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
    xiiRenderGraphTextureHandle                     m_Textures[XII_MAX_REFLECTION_PROBES];
  };

  inline Bindings Read(const xiiSharedPtr<const xiiReflectionProbeFrameData>& snapshot, xiiRenderGraphBuilder& builder)
  {
    Bindings result;
    result.m_Snapshot  = snapshot;
    result.m_Records   = builder.ReadBuffer(snapshot->m_Records, xiiGALResourceStateFlags::ShaderResource);
    result.m_Constants = builder.ReadBuffer(snapshot->m_Constants, xiiGALResourceStateFlags::ConstantBuffer);
    for (xiiUInt32 i = 0; i < snapshot->m_Textures.GetCount(); ++i)
    {
      xiiStringBuilder name;
      name.SetFormat("Local Reflection Cubemap {}", i);
      result.m_Textures[i] = builder.ReadTexture(builder.ImportTexture(name, snapshot->m_Textures[i], xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource);
    }
    return result;
  }

  inline void Bind(const Bindings& data, xiiRenderGraphPassContext& execute, xiiBitflags<xiiGALShaderType> stages)
  {
    auto& commands = execute.GetCommandList();
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
  }
} // namespace xiiReflectionProbePassUtils
