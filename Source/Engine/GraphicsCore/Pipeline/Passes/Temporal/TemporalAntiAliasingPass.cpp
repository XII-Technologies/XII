/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalAntiAliasingPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Temporal/TAAConstants.h>

struct xiiTemporalAntiAliasingPass::State : xiiRefCounted
{
  struct Slot : xiiRefCounted
  {
    xiiSharedPtr<xiiGALTexture> m_Color, m_Depth;
    xiiSharedPtr<xiiGALFence>   m_Ready;
    xiiUInt64                   m_uiReadyValue = 0, m_uiLastUsedFrame = 0, m_uiFrame = 0;
  };
  xiiMutex                                 m_Mutex;
  xiiDynamicArray<xiiSharedPtr<Slot>>      m_Slots;
  xiiSharedPtr<Slot>                       m_Last;
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipelines[4];
  xiiSharedPtr<xiiGALSampler>              m_Point, m_Linear;
  xiiUInt64                                m_uiRevision = 0;
  bool                                     m_bValid     = false;
};

xiiTemporalAntiAliasingPass::xiiTemporalAntiAliasingPass() : m_pState(XII_DEFAULT_NEW(State))
{
  m_OutputTAAResolved.SetResolution({xiiRenderPipelineTextureResolutionMode::Input});
}
xiiTemporalAntiAliasingPass::~xiiTemporalAntiAliasingPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTemporalAntiAliasingPass, 2, xiiRTTIDefaultAllocator<xiiTemporalAntiAliasingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("TemporalCamera", m_TemporalCamera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("HistoryTrust",m_HistoryTrust)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ReactiveRadiance",m_ReactiveRadiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("DepthThreshold", m_fDepthThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.01f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("HDRSceneColor", m_InputHDRSceneColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DilatedVelocityBuffer", m_InputDilatedVelocityBuffer)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SceneDepth", m_InputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("TAAResolved", m_OutputTAAResolved)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PreviousColor", m_OutputPreviousColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("BaseBlendAlpha", m_fBaseBlendAlpha)->AddAttributes(new xiiDefaultValueAttribute(0.08f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("MotionBlendScale", m_fMotionBlendScale)->AddAttributes(new xiiDefaultValueAttribute(12.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Temporal")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiTemporalAntiAliasingPass::InvalidateTemporalHistory()
{
  XII_LOCK(m_pState->m_Mutex);
  m_pState->m_bValid = false;
  ++m_pState->m_uiRevision;
}

xiiResult xiiTemporalAntiAliasingPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if (!xiiMath::IsFinite(m_fBaseBlendAlpha) || m_fBaseBlendAlpha < 0 || m_fBaseBlendAlpha > 1 ||
      !xiiMath::IsFinite(m_fMotionBlendScale) || m_fMotionBlendScale < 0 ||
      !xiiMath::IsFinite(m_fDepthThreshold) || m_fDepthThreshold < 0 || m_fDepthThreshold > 1)
    return XII_FAILURE;
  InvalidateTemporalHistory();
  return XII_SUCCESS;
}

xiiResult xiiTemporalAntiAliasingPass::BuildTemporalHistory(xiiRenderPipelinePassContext& context)
{
  if (!m_bEnabled)
    return XII_SUCCESS;
  XII_LOCK(m_pState->m_Mutex);
  if (!m_pState->m_bValid || !m_pState->m_Last || m_pState->m_Last->m_uiFrame >= context.m_Graph.GetFrameIndex())
    return XII_SUCCESS;
  const auto last         = m_pState->m_Last;
  last->m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiSharedPtr<State::Slot>   m_Slot;
    xiiRenderGraphTextureHandle m_Output;
  };
  const auto emitted = context.m_Graph.AddPass<Data>("Publish TAA History", xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Slot = last;
    builder.WaitForFence(last->m_Ready, last->m_uiReadyValue).AssertSuccess();
    data.m_Output = builder.ReadWriteTexture(builder.ImportTexture("Previous Color", last->m_Color, last->m_Color->GetResourceState()), xiiGALResourceStateFlags::ShaderResource); }, [](const Data&, xiiRenderGraphPassContext&) {});
  return context.SetOutput(m_OutputPreviousColor, emitted.first->m_Output);
}

xiiResult xiiTemporalAntiAliasingPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  color      = context.GetInputTexture(m_InputHDRSceneColor);
  const auto  cameraData = context.GetInputData(m_TemporalCamera);
  const auto* camera     = xiiDynamicCast<const xiiTemporalCameraSnapshot*>(cameraData.Borrow());
  if (!m_bEnabled && camera && camera->m_Sample.m_bEnabled)
    return context.ReportFailure("Disable temporal camera jitter together with TAA reconstruction.");
  if (!m_bEnabled || (camera && !camera->m_Sample.m_bEnabled))
  {
    InvalidateTemporalHistory();
    XII_LOCK(m_pState->m_Mutex);
    m_pState->m_Last.Clear();
    m_pState->m_Slots.Clear();
    return context.SetOutput(m_OutputTAAResolved, color);
  }
  const auto  velocity = context.GetInputTexture(m_InputDilatedVelocityBuffer), depth = context.GetInputTexture(m_InputSceneDepth);
  const auto* description = xiiScreenSpacePassUtils::Image(context, color, 4);
  if (!description || (description->m_Format != xiiGALResourceFormat::RGBA16Float && description->m_Format != xiiGALResourceFormat::RGBA32Float))
    return context.ReportFailure("TAA requires a readable, single-sample RGBA16Float or RGBA32Float color image.");
  const auto size = description->m_Size;
  xiiSizeU32 outputSize;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_OutputTAAResolved, color, outputSize));
  if (outputSize != size || !xiiScreenSpacePassUtils::Image(context, velocity, 2, &size) || !xiiScreenSpacePassUtils::Image(context, depth, 1, &size) || (camera && camera->m_Sample.m_Size != size))
    return context.ReportFailure("TAA requires matching input-sized color, velocity, depth and camera samples.");
  const auto trust = context.GetInputTexture(m_HistoryTrust);
  const auto reactive = context.GetInputTexture(m_ReactiveRadiance);
  if (reactive.IsValid())
  {
    const auto* signal = xiiScreenSpacePassUtils::Image(context, reactive, 4, &size);
    if (!signal || (signal->m_Format != xiiGALResourceFormat::RGBA16Float && signal->m_Format != xiiGALResourceFormat::RGBA32Float))
      return context.ReportFailure("TAA reactive radiance requires matching readable RGBA16Float or RGBA32Float input.");
  }
  if (trust.IsValid())
  {
    const auto* mask = xiiScreenSpacePassUtils::Image(context, trust, 1, &size);
    if (!mask || (mask->m_Format != xiiGALResourceFormat::R8UNormalized && mask->m_Format != xiiGALResourceFormat::R16Float && mask->m_Format != xiiGALResourceFormat::R32Float))
      return context.ReportFailure("TAA history trust requires matching readable R8UNormalized, R16Float or R32Float coverage.");
  }
  xiiUInt64 completed = 0;
  if (xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("TAA requires an active shared GPU frame.");
  auto& state = *m_pState;
  XII_LOCK(state.m_Mutex);
  const bool            hasReactive = trust.IsValid() || reactive.IsValid();
  const xiiUInt32       variant     = (description->m_Format == xiiGALResourceFormat::RGBA32Float ? 1U : 0U) + (hasReactive ? 2U : 0U);
  auto&                 pipeline = state.m_Pipelines[variant];
  constexpr const char* paths[]  = {"Shaders/Pipeline/TAA.xiiShader", "Shaders/Pipeline/TAA32.xiiShader", "Shaders/Pipeline/TAAReactive.xiiShader", "Shaders/Pipeline/TAAReactive32.xiiShader"};
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(pipeline, paths[variant]));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(state.m_Linear));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::PointClampSampler(state.m_Point));
  const bool valid = state.m_bValid && state.m_Last && state.m_Last->m_Color->GetDescription().m_Size == size &&
    state.m_Last->m_Color->GetDescription().m_Format == description->m_Format && state.m_Last->m_uiFrame < context.m_Graph.GetFrameIndex() &&
    (!camera || (camera->m_Sample.m_bHistoryValid && state.m_Last->m_uiFrame == camera->m_Sample.m_uiPreviousFrame));
  // The slot itself is retained by every recording/publication. Completion alone
  // cannot authorize overwriting images referenced by a recording executed later.
  xiiSharedPtr<State::Slot> next;
  for (xiiUInt32 i = state.m_Slots.GetCount(); i-- > 0;)
  {
    const auto& slot = state.m_Slots[i];
    if (slot != state.m_Last && slot->GetRefCount() == 1 && slot->m_uiLastUsedFrame <= completed)
    {
      if (slot->m_Color->GetDescription().m_Size != size || slot->m_Color->GetDescription().m_Format != description->m_Format)
        state.m_Slots.RemoveAtAndSwap(i);
      else if (!next)
        next = slot;
    }
  }
  if (!next)
  {
    next = XII_DEFAULT_NEW(State::Slot);
    xiiGALTextureCreationDescription allocation;
    allocation.m_Type      = xiiGALResourceDimension::Texture2D;
    allocation.m_Size      = size;
    allocation.m_Format    = description->m_Format;
    allocation.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    next->m_Color          = xiiGALDevice::GetDefaultDevice()->CreateTexture(allocation);
    allocation.m_Format    = xiiGALResourceFormat::R32Float;
    next->m_Depth          = xiiGALDevice::GetDefaultDevice()->CreateTexture(allocation);
    xiiGALFenceCreationDescription fence;
    fence.m_Type  = xiiGALFenceType::General;
    next->m_Ready = xiiGALDevice::GetDefaultDevice()->CreateFence(fence);
    if (!next->m_Color || !next->m_Depth || !next->m_Ready)
      return context.ReportFailure("Could not allocate TAA color/depth history and readiness fence.");
    state.m_Slots.PushBack(next);
  }
  ++next->m_uiReadyValue;
  next->m_uiFrame = next->m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  if (valid)
    state.m_Last->m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Color, m_Velocity, m_Depth, m_Trust, m_Reactive, m_PreviousColor, m_PreviousDepth, m_Output, m_OutputDepth;
    xiiRenderGraphBufferHandle               m_Constants;
    xiiTAAConstants                          m_Parameters;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<State>                      m_State;
    xiiSharedPtr<State::Slot>                m_Next, m_Previous;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiSharedPtr<xiiGALSampler>              m_Linear, m_Point;
    xiiUInt64                                m_uiRevision, m_uiReadyValue, m_uiFrame;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_State = m_pState;
    data.m_Next = next;
    data.m_uiRevision = state.m_uiRevision;
    data.m_uiReadyValue = next->m_uiReadyValue;
    data.m_uiFrame = next->m_uiFrame;
    data.m_Pipeline = pipeline;
    data.m_Linear = state.m_Linear;
    data.m_Point = state.m_Point;
    data.m_Size = size;
    data.m_Color = builder.ReadTexture(color, xiiGALResourceStateFlags::ShaderResource);
    data.m_Velocity = builder.ReadTexture(velocity, xiiGALResourceStateFlags::ShaderResource);
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    if (hasReactive)
    {
      data.m_Trust    = trust.IsValid() ? builder.ReadTexture(trust, xiiGALResourceStateFlags::ShaderResource) : data.m_Depth;
      data.m_Reactive = reactive.IsValid() ? builder.ReadTexture(reactive, xiiGALResourceStateFlags::ShaderResource) : data.m_Color;
    }
    if (valid)
    {
      data.m_Previous = state.m_Last;
      builder.WaitForFence(state.m_Last->m_Ready, state.m_Last->m_uiReadyValue).AssertSuccess();
      data.m_PreviousColor = builder.ReadTexture(builder.ImportTexture("TAA Previous Color", state.m_Last->m_Color, state.m_Last->m_Color->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
      data.m_PreviousDepth = builder.ReadTexture(builder.ImportTexture("TAA Previous Depth", state.m_Last->m_Depth, state.m_Last->m_Depth->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
    }
    else
    {
      data.m_PreviousColor = data.m_Color;
      data.m_PreviousDepth = data.m_Depth;
    }
    data.m_Output = builder.WriteTexture(builder.ImportTexture("TAA Resolved", next->m_Color, next->m_Color->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_OutputDepth = builder.WriteTexture(builder.ImportTexture("TAA Retained Depth", next->m_Depth, next->m_Depth->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Parameters = {};
    data.m_Parameters.HistoryValid = valid ? 1U : 0U;
    data.m_Parameters.BaseBlendAlpha = m_fBaseBlendAlpha;
    data.m_Parameters.MotionBlendScale = m_fMotionBlendScale;
    data.m_Parameters.DepthThreshold = m_fDepthThreshold;
    data.m_Parameters.HasCamera = camera ? 1U : 0U;
    data.m_Parameters.ReactiveFlags          = (trust.IsValid() ? 1U : 0U) | (reactive.IsValid() ? 2U : 0U);
    data.m_Parameters.InverseViewProjection = camera ? camera->m_Sample.m_Camera.m_InverseViewProjectionMatrix : xiiMat4::MakeIdentity();
    data.m_Parameters.PreviousViewProjection = camera ? camera->m_Sample.m_PreviousViewProjection : xiiMat4::MakeIdentity();
    data.m_Parameters.CameraPosition = camera ? camera->m_Sample.m_Camera.m_CameraPositionAndNearPlane : xiiVec4::MakeZero();
    data.m_Parameters.JitterDelta = camera ? camera->m_Sample.m_PreviousJitter - camera->m_Sample.m_Jitter : xiiVec2::MakeZero();
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "TAA Constants", sizeof(data.m_Parameters)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiTAAConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = data.m_Parameters;
    }
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetSampler("LinearClampSampler", data.m_Linear.Borrow());
    commands.ResolveAndSetSampler("PointClampSampler", data.m_Point.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiTAAConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_CurrentFrame", execute.GetTexture(data.m_Color)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_VelocityBuffer", execute.GetTexture(data.m_Velocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SceneDepth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_HistoryFrame", execute.GetTexture(data.m_PreviousColor)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_HistoryDepth", execute.GetTexture(data.m_PreviousDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    if (data.m_Trust.IsValid())
    {
      commands.ResolveAndSetShaderResourceTextureView("g_HistoryTrust", execute.GetTexture(data.m_Trust)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceTextureView("g_ReactiveRadiance", execute.GetTexture(data.m_Reactive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    }
    commands.ResolveAndSetUnorderedAccessTextureView("g_Resolved", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_RetainedDepth", execute.GetTexture(data.m_OutputDepth)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      execute.ReportFailure("Could not bind TAA reconstruction resources.");
      return;
    }
    commands.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1U});
    commands.EnqueueSignal(data.m_Next->m_Ready.Borrow(), data.m_uiReadyValue);
    XII_LOCK(data.m_State->m_Mutex);
    if (data.m_State->m_uiRevision == data.m_uiRevision && (!data.m_State->m_bValid || data.m_State->m_Last->m_uiFrame < data.m_uiFrame))
    {
      data.m_State->m_Last = data.m_Next;
      data.m_State->m_bValid = true;
    } });
  return context.SetOutput(m_OutputTAAResolved, emitted.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Temporal_TemporalAntiAliasingPass);
