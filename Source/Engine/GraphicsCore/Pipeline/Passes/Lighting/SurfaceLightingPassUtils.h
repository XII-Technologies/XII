/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <Shaders/Pipeline/Passes/Lighting/SurfaceLightingConstants.h>
namespace xiiSurfaceLightingPassUtils
{
  inline bool FloatReadable(xiiGALResourceFormat::Enum format)
  {
    const auto& description = xiiGALTextureUtilities::GetResourceFormatProperties(format);
    const auto  type        = description.m_ComponentType;
    return !description.IsTypeless() && (type == xiiGALResourceFormatComponentType::Float || description.IsNormalized() || type == xiiGALResourceFormatComponentType::UnsignedNormalizedSRGB || type == xiiGALResourceFormatComponentType::Depth || type == xiiGALResourceFormatComponentType::DepthStencil);
  }
  struct Inputs
  {
    xiiRenderGraphTextureHandle m_Depth, m_Normal, m_Albedo, m_Material, m_Emissive;
    xiiRenderGraphBufferHandle  m_Camera;
    xiiSizeU32                  m_Size;
  };
  inline xiiResult Validate(xiiRenderPipelinePassContext& context, Inputs& data, bool requireEmission = true)
  {
    const auto* depth  = context.m_Graph.GetTextureDescription(data.m_Depth);
    const auto* camera = context.m_Graph.GetBufferDescription(data.m_Camera);
    if (!depth || !depth->m_Size.HasNonZeroArea() || !camera || camera->m_uiSize != sizeof(xiiFrameCameraConstants))
      return context.ReportFailure("Surface lighting requires nonempty depth and exact camera constants.");
    data.m_Size           = depth->m_Size;
    const auto compatible = [&](xiiRenderGraphTextureHandle handle, xiiUInt32 channels) {
      const auto* d = context.m_Graph.GetTextureDescription(handle);
      return d && d->m_Size == data.m_Size && d->m_Type == xiiGALResourceDimension::Texture2D && d->m_uiArraySizeOrDepth == 1 && d->m_uiSampleCount == 1 && d->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) && FloatReadable(d->m_Format) && xiiGALTextureUtilities::GetComponentCount(d->m_Format) >= channels;
    };
    if (!compatible(data.m_Depth, 1) || !compatible(data.m_Normal, 2) || !compatible(data.m_Albedo, 4) || !compatible(data.m_Material, 4) || (requireEmission && !compatible(data.m_Emissive, 4)))
      return context.ReportFailure("Surface images must have matching extents, sample count and shader-readable channel layouts.");
    return XII_SUCCESS;
  }
  inline Inputs Read(const Inputs& source, xiiRenderGraphBuilder& builder)
  {
    auto data       = source;
    data.m_Depth    = builder.ReadTexture(source.m_Depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal   = builder.ReadTexture(source.m_Normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Albedo   = builder.ReadTexture(source.m_Albedo, xiiGALResourceStateFlags::ShaderResource);
    data.m_Material = builder.ReadTexture(source.m_Material, xiiGALResourceStateFlags::ShaderResource);
    if (source.m_Emissive.IsValid())
      data.m_Emissive = builder.ReadTexture(source.m_Emissive, xiiGALResourceStateFlags::ShaderResource);
    data.m_Camera = builder.ReadBuffer(source.m_Camera, xiiGALResourceStateFlags::ConstantBuffer);
    return data;
  }
  inline void Bind(const Inputs& data, xiiRenderGraphPassContext& execute)
  {
    auto& commands = execute.GetCommandList();
    commands.ResolveAndSetConstantBuffer("xiiSurfaceCameraConstants", execute.GetBuffer(data.m_Camera), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SurfaceDepth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SurfaceNormal", execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SurfaceAlbedo", execute.GetTexture(data.m_Albedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SurfaceMaterial", execute.GetTexture(data.m_Material)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    if (data.m_Emissive.IsValid())
      commands.ResolveAndSetShaderResourceTextureView("g_SurfaceEmissive", execute.GetTexture(data.m_Emissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  }
  inline xiiRenderGraphTextureHandle Output(xiiRenderGraphBuilder& builder, xiiSizeU32 size)
  {
    xiiGALTextureCreationDescription d;
    d.m_Type      = xiiGALResourceDimension::Texture2D;
    d.m_Size      = size;
    d.m_Format    = xiiGALResourceFormat::RGBA16Float;
    d.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    return builder.WriteTexture("Lighting", d, xiiGALResourceStateFlags::UnorderedAccess);
  }
  inline void BindConstants(xiiRenderGraphPassContext& execute, xiiRenderGraphBufferHandle buffer, const xiiSurfaceLightingConstants& snapshot)
  {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiSurfaceLightingConstants> mapped(commands, execute.GetBuffer(buffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = snapshot;
    }
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = execute.GetBuffer(buffer);
    transition.m_NewState        = xiiGALResourceStateFlags::ConstantBuffer;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    commands.ResolveAndSetConstantBuffer("xiiSurfaceLightingConstants", execute.GetBuffer(buffer), xiiGALShaderType::Compute);
  }
} // namespace xiiSurfaceLightingPassUtils
