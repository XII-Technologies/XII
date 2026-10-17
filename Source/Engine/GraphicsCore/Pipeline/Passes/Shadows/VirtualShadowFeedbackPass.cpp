/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowFeedbackPass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowFeedbackTypes.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>
#include <Shaders/Pipeline/Passes/VirtualShadowMap/VirtualShadowMapConstants.h>

namespace
{
  bool ValidFeedbackSettings(const xiiMat4& inverse, xiiUInt32 resolution, xiiUInt32 pageSize, xiiUInt32 light, float nearPlane)
  {
    return inverse.IsValid() && xiiMath::IsPowerOf2(resolution) && xiiMath::IsPowerOf2(pageSize) && resolution >= pageSize &&
      resolution / pageSize <= (1U << 17) && light < (1U << 24) && xiiMath::IsFinite(nearPlane) && nearPlane > 0;
  }
} // namespace
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVirtualShadowFeedbackPass, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowFeedbackPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneDepth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("CascadeConstants", m_Cascades)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitializedRequests", m_Initial)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Requests", m_Requests)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InverseViewProjection", m_InverseViewProjection),
    XII_MEMBER_PROPERTY("VirtualResolution", m_uiVirtualResolution)->AddAttributes(new xiiDefaultValueAttribute(16384U), new xiiClampValueAttribute(1U, xiiVariant())),
    XII_MEMBER_PROPERTY("PageSize", m_uiPageSize)->AddAttributes(new xiiDefaultValueAttribute(128U), new xiiClampValueAttribute(1U, xiiVariant())),
    XII_MEMBER_PROPERTY("LightId", m_uiLightId)->AddAttributes(new xiiDefaultValueAttribute(1U), new xiiClampValueAttribute(0U, 0xFFFFFFU)),
    XII_MEMBER_PROPERTY("NearPlane", m_fNearPlane)->AddAttributes(new xiiDefaultValueAttribute(0.1f), new xiiClampValueAttribute(0.0001f, xiiVariant())),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Shadows") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiVirtualShadowFeedbackPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return ValidFeedbackSettings(m_InverseViewProjection, m_uiVirtualResolution, m_uiPageSize, m_uiLightId, m_fNearPlane) ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiVirtualShadowFeedbackPass::Build(xiiRenderPipelinePassContext& context)
{
  auto inverse    = m_InverseViewProjection;
  auto resolution = m_uiVirtualResolution, pageSize = m_uiPageSize, light = m_uiLightId;
  auto nearPlane = m_fNearPlane;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ShadowFeedbackInverseViewProjection", inverse));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ShadowFeedbackVirtualResolution", resolution));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ShadowFeedbackPageSize", pageSize));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ShadowFeedbackLightId", light));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ShadowFeedbackNearPlane", nearPlane));
  if (!ValidFeedbackSettings(inverse, resolution, pageSize, light, nearPlane))
    return context.ReportFailure("Invalid virtual shadow feedback projection, page dimensions, light identity or near plane.");
  const auto  depth    = context.GetInputTexture(m_Depth);
  const auto  cascades = context.GetInputBuffer(m_Cascades), initial = context.GetInputBuffer(m_Initial);
  const auto* image     = context.m_Graph.GetTextureDescription(depth);
  const auto* constants = context.m_Graph.GetBufferDescription(cascades);
  const auto* requests  = context.m_Graph.GetBufferDescription(initial);
  if (!image || image->m_Format != xiiGALResourceFormat::D32Float || image->m_Type != xiiGALResourceDimension::Texture2D || image->m_uiSampleCount != 1 ||
      !image->m_Size.HasNonZeroArea() || image->m_Size.width > 65536 || image->m_Size.height > 65536 || !constants || constants->m_uiSize < sizeof(xiiShadowCascadeConstants) ||
      !requests || requests->m_Mode != xiiGALBufferMode::Structured || requests->m_uiElementByteStride != sizeof(xiiGpuVirtualShadowFeedback) ||
      requests->m_uiSize % sizeof(xiiGpuVirtualShadowFeedback) || requests->m_uiSize < 2 * sizeof(xiiGpuVirtualShadowFeedback) || requests->m_uiSize > 1048577ULL * sizeof(xiiGpuVirtualShadowFeedback))
    return context.ReportFailure("Virtual shadow feedback requires single-sample D32 depth, cascade constants and a bounded initialized structured append stream.");
  if (!xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto& limits  = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  const auto  groupsX = (image->m_Size.width + 63) / 64, groupsY = (image->m_Size.height + 63) / 64;
  if (limits.m_uiMaxThreadGroupInvocations < 64 || limits.m_uiMaxThreadGroupSizeX < 8 || limits.m_uiMaxThreadGroupSizeY < 8 || groupsX > limits.m_uiMaxThreadGroupCountX || groupsY > limits.m_uiMaxThreadGroupCountY)
    return context.ReportFailure("Virtual shadow feedback exceeds compute dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Shadows/VirtualShadowFeedback.xiiShader"));
  const auto                 capacity = static_cast<xiiUInt32>(requests->m_uiSize / sizeof(xiiGpuVirtualShadowFeedback) - 1);
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle       m_Depth;
    xiiRenderGraphBufferHandle        m_Cascades, m_Requests, m_Constants;
    xiiVirtualShadowFeedbackConstants m_Snapshot;
    xiiVec3U32                        m_Groups;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Cascades = builder.ReadBuffer(cascades, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Requests = builder.ReadWriteBuffer(initial, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "Shadow Feedback Parameters", sizeof(xiiVirtualShadowFeedbackConstants));
    data.m_Snapshot.VirtualShadowInverseViewProjection = inverse;
    data.m_Snapshot.VirtualResolution = resolution;
    data.m_Snapshot.PageSize = pageSize;
    data.m_Snapshot.MaxFeedbackRequests = capacity;
    data.m_Snapshot.DirectionalLightId = light;
    data.m_Snapshot.VirtualShadowNearPlane = nearPlane;
    data.m_Snapshot._VirtualShadowFeedbackPadding = xiiVec3::MakeZero();
    data.m_Groups = {groupsX, groupsY, 1};
    builder.SetPassAllowMerge(false); }, [pipeline = m_Pipeline](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& cmd = execute.GetCommandList();
    { xiiGALMapHelper<xiiVirtualShadowFeedbackConstants> mapped(cmd, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard); *mapped = data.m_Snapshot; }
    cmd.SetPipelineState(pipeline.Borrow());
    cmd.ResolveAndSetConstantBuffer("xiiVirtualShadowFeedbackConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiShadowCascadeConstants", execute.GetBuffer(data.m_Cascades), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_VirtualShadowFeedback", execute.GetBuffer(data.m_Requests)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
    cmd.DispatchCompute({data.m_Groups.x, data.m_Groups.y, data.m_Groups.z}); });
  return context.SetOutput(m_Requests, pass.first->m_Requests);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_VirtualShadowFeedbackPass);
