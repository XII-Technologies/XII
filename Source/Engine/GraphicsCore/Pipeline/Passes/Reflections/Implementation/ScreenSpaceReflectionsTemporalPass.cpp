/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Reflections/ScreenSpaceReflectionsTemporalPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Reflections/SSRTemporalConstants.h>

struct xiiScreenSpaceReflectionsTemporalPass::State : xiiRefCounted
{
  static constexpr xiiUInt32 s_uiMaximumHistorySlots = 8;
  struct Slot : xiiRefCounted
  {
    xiiSharedPtr<xiiGALTexture> m_Radiance, m_Surface, m_Identity;
    xiiSharedPtr<xiiGALFence>   m_Ready;
    xiiUInt64                   m_uiReadyValue = 0, m_uiLastUsedFrame = 0, m_uiFrame = 0;
    xiiGALResourceFormat::Enum  m_InputFormat = xiiGALResourceFormat::Unknown;
  };
  xiiMutex                                 m_Mutex;
  xiiDynamicArray<xiiSharedPtr<Slot>>      m_Slots;
  xiiSharedPtr<Slot>                       m_Last;
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiUInt64                                m_uiRevision = 0;
  bool                                     m_bValid     = false;
};

xiiScreenSpaceReflectionsTemporalPass::xiiScreenSpaceReflectionsTemporalPass() : m_pState(XII_DEFAULT_NEW(State))
{
  m_OutputSSRTerm.SetResolution({xiiRenderPipelineTextureResolutionMode::Input});
}
xiiScreenSpaceReflectionsTemporalPass::~xiiScreenSpaceReflectionsTemporalPass() = default;

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenSpaceReflectionsTemporalPass, 2, xiiRTTIDefaultAllocator<xiiScreenSpaceReflectionsTemporalPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("TemporalCamera", m_TemporalCamera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("SSRRawTerm", m_InputSSRRawTerm)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SceneDepth", m_InputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GBufferNormal", m_InputGBufferNormal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GBufferMaterial", m_InputGBufferMaterial)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GBufferObjectId", m_InputGBufferObjectId)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DilatedVelocityBuffer", m_InputDilatedVelocityBuffer)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SSRTerm", m_OutputSSRTerm)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Identity", m_OutputIdentity)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Constants", m_OutputConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("xiiSSRTemporalConstants")),
    XII_MEMBER_PROPERTY("PreviousReflection", m_OutputPreviousReflection)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PreviousIdentity", m_OutputPreviousIdentity)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("HistoryWeight", m_fHistoryWeight)->AddAttributes(new xiiDefaultValueAttribute(0.92f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("ConfidenceDecay", m_fConfidenceDecay)->AddAttributes(new xiiDefaultValueAttribute(0.96f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("DepthThreshold", m_fDepthThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.02f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("NormalThreshold", m_fNormalThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.95f), new xiiClampValueAttribute(0.0f, 1.0f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Reflections") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiScreenSpaceReflectionsTemporalPass::InvalidateTemporalHistory()
{
  XII_LOCK(m_pState->m_Mutex);
  m_pState->m_bValid = false;
  ++m_pState->m_uiRevision;
}
xiiResult xiiScreenSpaceReflectionsTemporalPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  for (float value : {m_fHistoryWeight, m_fConfidenceDecay, m_fDepthThreshold, m_fNormalThreshold})
    if (!xiiMath::IsFinite(value) || value < 0 || value > 1)
      return XII_FAILURE;
  InvalidateTemporalHistory();
  return XII_SUCCESS;
}

xiiResult xiiScreenSpaceReflectionsTemporalPass::BuildTemporalHistory(xiiRenderPipelinePassContext& context)
{
  auto& state = *m_pState;
  XII_LOCK(state.m_Mutex);
  if (!m_bEnabled || !state.m_bValid || !state.m_Last || state.m_Last->m_uiFrame >= context.m_Graph.GetFrameIndex())
    return XII_SUCCESS;
  xiiRenderPassResourceScope scope(context);
  struct Previous
  {
    xiiSharedPtr<State::Slot>   m_Slot;
    xiiRenderGraphTextureHandle m_Radiance, m_Identity;
  };
  const auto imported = context.m_Graph.AddPass<Previous>("Publish SSR history", xiiGALCommandQueueFlags::Compute, [&](Previous& data, xiiRenderGraphBuilder& builder) {
    data.m_Slot = state.m_Last;
    data.m_Slot->m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
    builder.WaitForFence(data.m_Slot->m_Ready, data.m_Slot->m_uiReadyValue).AssertSuccess();
    data.m_Radiance = builder.ReadTexture(builder.ImportTexture("SSR Previous Radiance", data.m_Slot->m_Radiance, data.m_Slot->m_Radiance->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
    data.m_Identity = builder.ReadTexture(builder.ImportTexture("SSR Previous Identity", data.m_Slot->m_Identity, data.m_Slot->m_Identity->GetResourceState()), xiiGALResourceStateFlags::ShaderResource); }, [](const Previous&, xiiRenderGraphPassContext&) {});
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputPreviousReflection, imported.first->m_Radiance));
  return context.SetOutput(m_OutputPreviousIdentity, imported.first->m_Identity);
}

xiiResult xiiScreenSpaceReflectionsTemporalPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto radiance = context.GetInputTexture(m_InputSSRRawTerm);
  if (!m_bEnabled || !radiance.IsValid())
  {
    InvalidateTemporalHistory();
    XII_LOCK(m_pState->m_Mutex);
    m_pState->m_Last.Clear();
    m_pState->m_Slots.Clear();
    return radiance.IsValid() ? context.SetOutput(m_OutputSSRTerm, radiance) : XII_SUCCESS;
  }
  if (xiiGALDevice::GetDefaultDevice()->GetFeatures().m_TextureUAVExtendedFormats != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("Temporal SSR requires RG32UInt storage-image support (TextureUAVExtendedFormats). Disable temporal filtering on unsupported devices.");
  const auto* source = xiiScreenSpacePassUtils::Image(context, radiance, 4);
  if (!source || (source->m_Format != xiiGALResourceFormat::RGBA16Float && source->m_Format != xiiGALResourceFormat::RGBA32Float))
    return context.ReportFailure("Temporal SSR requires readable RGBA16Float or RGBA32Float radiance.");
  const auto size = source->m_Size;
  xiiSizeU32 outputSize;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_OutputSSRTerm, radiance, outputSize));
  const auto  depth = context.GetInputTexture(m_InputSceneDepth), normal = context.GetInputTexture(m_InputGBufferNormal), material = context.GetInputTexture(m_InputGBufferMaterial);
  const auto  ids = context.GetInputTexture(m_InputGBufferObjectId), velocity = context.GetInputTexture(m_InputDilatedVelocityBuffer);
  const auto  cameraData = context.GetInputData(m_TemporalCamera);
  const auto* camera     = xiiDynamicCast<const xiiTemporalCameraSnapshot*>(cameraData.Borrow());
  const auto* identities = context.m_Graph.GetTextureDescription(ids);
  if (outputSize != size || !xiiScreenSpacePassUtils::Image(context, depth, 1, &size) || !xiiScreenSpacePassUtils::Image(context, normal, 2, &size) ||
      !xiiScreenSpacePassUtils::Image(context, material, 4, &size) || !xiiScreenSpacePassUtils::Image(context, velocity, 2, &size) || !identities ||
      identities->m_Type != xiiGALResourceDimension::Texture2D || identities->m_Format != xiiGALResourceFormat::R32UInt || identities->m_Size != size || identities->m_uiSampleCount != 1 ||
      !identities->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || !camera || camera->m_Sample.m_Size != size)
    return context.ReportFailure("Temporal SSR requires matching surface, object identity, velocity and explicit temporal camera inputs.");
  xiiUInt64 completed = 0;
  if (xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("Temporal SSR requires an active shared GPU frame.");
  auto& state = *m_pState;
  XII_LOCK(state.m_Mutex);
  auto& pipeline = state.m_Pipeline;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(pipeline, "Shaders/Pipeline/SSRTemporal.xiiShader"));
  const bool valid = state.m_bValid && state.m_Last && state.m_Last->m_Radiance->GetDescription().m_Size == size &&
    state.m_Last->m_InputFormat == source->m_Format && camera->m_Sample.m_bHistoryValid &&
    state.m_Last->m_uiFrame == camera->m_Sample.m_uiPreviousFrame && state.m_Last->m_uiFrame < context.m_Graph.GetFrameIndex();
  xiiSharedPtr<State::Slot> next;
  for (xiiUInt32 i = state.m_Slots.GetCount(); i-- > 0;)
  {
    const auto& slot = state.m_Slots[i];
    if (slot != state.m_Last && slot->GetRefCount() == 1 && slot->m_uiLastUsedFrame <= completed)
    {
      if (slot->m_Radiance->GetDescription().m_Size != size)
        state.m_Slots.RemoveAtAndSwap(i);
      else if (!next)
        next = slot;
    }
  }
  if (!next)
  {
    // Retained recordings may pin slots beyond GPU completion. Fail explicitly
    // instead of overwriting their storage or growing history without a bound.
    if (state.m_Slots.GetCount() >= State::s_uiMaximumHistorySlots)
      return context.ReportFailure("Temporal SSR history capacity is exhausted by in-flight or retained recordings.");
    next = XII_DEFAULT_NEW(State::Slot);
    xiiGALTextureCreationDescription allocation;
    allocation.m_Type      = xiiGALResourceDimension::Texture2D;
    allocation.m_Size      = size;
    allocation.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    allocation.m_Format    = xiiGALResourceFormat::RGBA16Float;
    next->m_Radiance       = xiiGALDevice::GetDefaultDevice()->CreateTexture(allocation);
    allocation.m_Format    = xiiGALResourceFormat::RG32UInt;
    next->m_Surface        = xiiGALDevice::GetDefaultDevice()->CreateTexture(allocation);
    allocation.m_Format    = xiiGALResourceFormat::RG32UInt;
    next->m_Identity       = xiiGALDevice::GetDefaultDevice()->CreateTexture(allocation);
    xiiGALFenceCreationDescription fence;
    fence.m_Type  = xiiGALFenceType::General;
    next->m_Ready = xiiGALDevice::GetDefaultDevice()->CreateFence(fence);
    if (!next->m_Radiance || !next->m_Surface || !next->m_Identity || !next->m_Ready)
      return context.ReportFailure("Could not allocate temporal SSR radiance, surface, identity and readiness storage.");
    state.m_Slots.PushBack(next);
  }
  next->m_InputFormat = source->m_Format;
  ++next->m_uiReadyValue;
  next->m_uiFrame = next->m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  if (valid)
    state.m_Last->m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Radiance, m_Depth, m_Normal, m_Material, m_Ids, m_Velocity, m_PreviousRadiance, m_PreviousSurface, m_PreviousIdentity, m_Output, m_Surface, m_Identity;
    xiiRenderGraphBufferHandle               m_Constants;
    xiiSSRTemporalConstants                  m_Parameters;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<State>                      m_State;
    xiiSharedPtr<State::Slot>                m_Next, m_Previous;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiUInt64                                m_uiRevision, m_uiReadyValue, m_uiFrame;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_State = m_pState; data.m_Next = next; data.m_Pipeline = pipeline; data.m_Size = size;
    data.m_uiRevision = state.m_uiRevision; data.m_uiReadyValue = next->m_uiReadyValue; data.m_uiFrame = next->m_uiFrame;
    data.m_Radiance = builder.ReadTexture(radiance, xiiGALResourceStateFlags::ShaderResource);
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal = builder.ReadTexture(normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Material = builder.ReadTexture(material, xiiGALResourceStateFlags::ShaderResource);
    data.m_Ids = builder.ReadTexture(ids, xiiGALResourceStateFlags::ShaderResource);
    data.m_Velocity = builder.ReadTexture(velocity, xiiGALResourceStateFlags::ShaderResource);
    const auto import = [&](xiiStringView name, const xiiSharedPtr<xiiGALTexture>& image) { return builder.ImportTexture(name, image, image->GetResourceState()); };
    data.m_Previous = valid ? state.m_Last : next;
    if (valid) builder.WaitForFence(data.m_Previous->m_Ready, data.m_Previous->m_uiReadyValue).AssertSuccess();
    // Invalid history binds current inputs of compatible types and is never sampled.
    data.m_PreviousRadiance = valid ? builder.ReadTexture(import("SSR Previous Radiance", data.m_Previous->m_Radiance), xiiGALResourceStateFlags::ShaderResource) : data.m_Radiance;
    data.m_PreviousSurface = valid ? builder.ReadTexture(import("SSR Previous Surface", data.m_Previous->m_Surface), xiiGALResourceStateFlags::ShaderResource) : data.m_Ids;
    data.m_PreviousIdentity = valid ? builder.ReadTexture(import("SSR Previous Identity", data.m_Previous->m_Identity), xiiGALResourceStateFlags::ShaderResource) : data.m_Ids;
    data.m_Output = builder.WriteTexture(import("SSR Resolved", next->m_Radiance), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Surface = builder.WriteTexture(import("SSR Surface", next->m_Surface), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Identity = builder.WriteTexture(import("SSR Identity", next->m_Identity), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Parameters = {};
    data.m_Parameters.HistoryWeight = m_fHistoryWeight; data.m_Parameters.ConfidenceDecay = m_fConfidenceDecay;
    data.m_Parameters.DepthThreshold = m_fDepthThreshold; data.m_Parameters.NormalThreshold = m_fNormalThreshold;
    data.m_Parameters.HistoryValid = valid ? 1U : 0U;
    data.m_Parameters.JitterDelta = camera->m_Sample.m_PreviousJitter - camera->m_Sample.m_Jitter;
    data.m_Parameters.InverseViewProjection = camera->m_Sample.m_Camera.m_InverseViewProjectionMatrix;
    data.m_Parameters.PreviousViewProjection = camera->m_Sample.m_PreviousViewProjection;
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "SSR Temporal Constants", sizeof(data.m_Parameters)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    { xiiGALMapHelper<xiiSSRTemporalConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard); *mapped = data.m_Parameters; }
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiSSRTemporalConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    const auto read = [&](const xiiTempHashedString& name, xiiRenderGraphTextureHandle image) { commands.ResolveAndSetShaderResourceTextureView(name, execute.GetTexture(image)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute); };
    read("g_CurrentReflection", data.m_Radiance); read("g_PreviousReflection", data.m_PreviousRadiance);
    read("g_SceneDepth", data.m_Depth); read("g_GBufferNormal", data.m_Normal); read("g_GBufferMaterial", data.m_Material);
    read("g_GBufferObjectId", data.m_Ids); read("g_Velocity", data.m_Velocity); read("g_PreviousSurface", data.m_PreviousSurface); read("g_PreviousIdentitySignature", data.m_PreviousIdentity);
    const auto write = [&](const xiiTempHashedString& name, xiiRenderGraphTextureHandle image) { commands.ResolveAndSetUnorderedAccessTextureView(name, execute.GetTexture(image)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute); };
    write("g_ResolvedReflection", data.m_Output); write("g_CurrentSurface", data.m_Surface); write("g_CurrentIdentitySignature", data.m_Identity);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()) { execute.ReportFailure("Could not bind temporal SSR reconstruction resources."); return; }
    commands.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1U});
    commands.EnqueueSignal(data.m_Next->m_Ready.Borrow(), data.m_uiReadyValue);
    XII_LOCK(data.m_State->m_Mutex);
    if (data.m_State->m_uiRevision == data.m_uiRevision && (!data.m_State->m_bValid || data.m_State->m_Last->m_uiFrame < data.m_uiFrame))
    { data.m_State->m_Last = data.m_Next; data.m_State->m_bValid = true; } });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputSSRTerm, emitted.first->m_Output));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputIdentity, emitted.first->m_Identity));
  return context.SetOutput(m_OutputConstants, emitted.first->m_Constants);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Reflections_Implementation_ScreenSpaceReflectionsTemporalPass);
