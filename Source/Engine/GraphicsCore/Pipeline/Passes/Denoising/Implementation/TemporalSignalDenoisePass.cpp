/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/Denoising/TemporalSignalDenoisePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionReprojectionConstants.h>
#include <Shaders/Pipeline/Passes/Denoising/TemporalDenoiseConstants.h>

struct xiiTemporalSignalDenoisePass::State : public xiiRefCounted
{
  struct Slot : xiiRefCounted
  {
    xiiSharedPtr<xiiGALTexture>   m_Signal, m_Surface, m_Identity;
    xiiSharedPtr<xiiGALFence>     m_ReadyFence;
    xiiUInt64                     m_uiReadyValue    = 0;
    xiiUInt64                     m_uiLastUsedFrame = 0;
    xiiUInt64                     m_uiFrame         = 0;
    xiiFrameCameraConstants       m_Camera;
    xiiEnum<xiiGALResourceFormat> m_InputFormat = xiiGALResourceFormat::Unknown;
  };
  static constexpr xiiUInt32               s_uiMaximumHistorySlots = 8;
  xiiDynamicArray<xiiSharedPtr<Slot>>      m_Slots;
  xiiSharedPtr<Slot>                       m_Last;
  xiiMutex                                 m_Mutex;
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_SpatialPipeline;
  bool                                     m_bHistoryValid     = false;
  xiiUInt64                                m_uiHistoryRevision = 0;
};
xiiTemporalSignalDenoisePass::xiiTemporalSignalDenoisePass(SignalMode mode) :
  m_Raw(mode == SignalMode::Occlusion ? xiiGALResourceFormat::R8UNormalized : xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1),
  m_Output(mode == SignalMode::Occlusion ? xiiGALResourceFormat::R8UNormalized : xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1),
  m_Previous(mode == SignalMode::Occlusion ? xiiGALResourceFormat::R8UNormalized : xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read, true),
  m_Surface(mode == SignalMode::Occlusion ? xiiGALResourceFormat::RGBA16Float : xiiGALResourceFormat::RGBA32Float, true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1),
  m_Mode(mode),
  m_Format(mode == SignalMode::Occlusion ? xiiGALResourceFormat::R8UNormalized : xiiGALResourceFormat::RGBA32Float)
{
  m_Output.SetResolution({xiiRenderPipelineTextureResolutionMode::Input});
}
xiiTemporalSignalDenoisePass::~xiiTemporalSignalDenoisePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTemporalSignalDenoisePass, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneDepth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferNormal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DilatedVelocityBuffer", m_Velocity)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PerFrameCamera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PerFrameGlobal", m_Global)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Surface", m_Surface)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("TemporalCamera", m_TemporalCamera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("UseHistory", m_bUseHistory)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("HistoryWeight", m_fHistoryWeight)->AddAttributes(new xiiDefaultValueAttribute(0.92f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("DepthThreshold", m_fDepthThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.08f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("NormalThreshold", m_fNormalThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.82f), new xiiClampValueAttribute(-1.0f, 1.0f)),
    XII_MEMBER_PROPERTY("SpatialWeight", m_fSpatialWeight)->AddAttributes(new xiiDefaultValueAttribute(0.35f), new xiiClampValueAttribute(0.0f, 1.0f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Denoising") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiTemporalSignalDenoisePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if (!xiiMath::IsFinite(m_fHistoryWeight) || m_fHistoryWeight < 0 || m_fHistoryWeight > 1 || !xiiMath::IsFinite(m_fDepthThreshold) || m_fDepthThreshold < 0 || m_fDepthThreshold > 1 || !xiiMath::IsFinite(m_fNormalThreshold) || m_fNormalThreshold < -1 || m_fNormalThreshold > 1 || !xiiMath::IsFinite(m_fSpatialWeight) || m_fSpatialWeight < 0 || m_fSpatialWeight > 1)
    return XII_FAILURE;
  InvalidateTemporalHistory();
  return XII_SUCCESS;
}
void xiiTemporalSignalDenoisePass::InvalidateTemporalHistory()
{
  if (m_pState)
  {
    XII_LOCK(m_pState->m_Mutex);
    m_pState->m_bHistoryValid = false;
    ++m_pState->m_uiHistoryRevision;
  }
}
xiiResult xiiTemporalSignalDenoisePass::BuildTemporalHistory(xiiRenderPipelinePassContext& context)
{
  if (!m_bEnabled || !m_bUseHistory || !m_pState || (m_Mode == SignalMode::Irradiance && xiiGALDevice::GetDefaultDevice()->GetFeatures().m_TextureUAVExtendedFormats != xiiGALDeviceFeatureState::Enabled))
    return XII_SUCCESS;
  XII_LOCK(m_pState->m_Mutex);
  if (!m_pState->m_bHistoryValid)
    return XII_SUCCESS;
  xiiUInt64 completed;
  if (xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("Signal history requires an active shared GPU frame.");
  const auto last         = m_pState->m_Last;
  last->m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiSharedPtr<State::Slot>   m_Slot;
    xiiRenderGraphTextureHandle m_Output;
  };
  const auto setup = [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Slot = last;
    builder.WaitForFence(last->m_ReadyFence, last->m_uiReadyValue).AssertSuccess();
    data.m_Output = builder.ReadWriteTexture(builder.ImportTexture("Previous signal", last->m_Signal, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource);
  };
  const auto emitted = context.m_Graph.AddPass<Data>("Publish Signal History", xiiGALCommandQueueFlags::Compute, setup, [](const Data&, xiiRenderGraphPassContext&) {});
  return context.SetOutput(m_Previous, emitted.first->m_Output);
}
xiiResult xiiTemporalSignalDenoisePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto raw = context.GetInputTexture(m_Raw), depth = context.GetInputTexture(m_Depth), normal = context.GetInputTexture(m_Normal), velocity = context.GetInputTexture(m_Velocity);
  const auto camera = context.GetInputBuffer(m_Camera), global = context.GetInputBuffer(m_Global);
  const bool occlusion     = m_Mode == SignalMode::Occlusion;
  const bool localLighting = m_Mode == SignalMode::LocalLighting;
  const auto domain        = localLighting ? context.GetInputTexture(m_SampleDomain) : xiiRenderGraphTextureHandle();
  if (!m_bEnabled || !raw.IsValid() || (localLighting && !domain.IsValid()))
  {
    InvalidateTemporalHistory();
    return raw.IsValid() ? context.SetOutput(m_Output, raw) : XII_SUCCESS;
  }
  const auto device = xiiGALDevice::GetDefaultDevice();
  if (!device || (occlusion && device->GetFeatures().m_TextureUAVExtendedFormats != xiiGALDeviceFeatureState::Enabled))
    return context.ReportFailure("Signal filtering requires R8 storage-image support (TextureUAVExtendedFormats).");
  const bool useHistory = m_bUseHistory && (m_Mode != SignalMode::Irradiance || device->GetFeatures().m_TextureUAVExtendedFormats == xiiGALDeviceFeatureState::Enabled);
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_Output, raw, size));
  const auto* sizingInput = context.m_Graph.GetTextureDescription(raw);
  if (!sizingInput || sizingInput->m_Size != size || (!occlusion && sizingInput->m_Format != xiiGALResourceFormat::RGBA16Float && sizingInput->m_Format != xiiGALResourceFormat::RGBA32Float))
    return context.ReportFailure("This operation requires input-sized output; resample its inputs before processing at another resolution.");
  if (!xiiScreenSpacePassUtils::Image(context, depth, 1, &size) || !xiiScreenSpacePassUtils::Image(context, normal, 2, &size) || (useHistory && !xiiScreenSpacePassUtils::Image(context, velocity, 2, &size)))
    return context.ReportFailure("Signal filtering requires matching readable depth, normal and, with history, velocity images.");
  xiiRenderGraphTextureHandle material, objectId;
  if (!occlusion)
  {
    material             = context.GetInputTexture(m_Material);
    objectId             = context.GetInputTexture(m_ObjectId);
    const auto* identity = context.m_Graph.GetTextureDescription(objectId);
    if (!xiiScreenSpacePassUtils::Image(context, material, 4, &size) || !identity || identity->m_Format != xiiGALResourceFormat::R32UInt || identity->m_Size != size || identity->m_uiSampleCount != 1 || !identity->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
      return context.ReportFailure("RGB signal denoising requires matching material and object identity images.");
  }
  if (localLighting)
  {
    const auto* sampleDomain = context.m_Graph.GetTextureDescription(domain);
    if (!sampleDomain || sampleDomain->m_Format != xiiGALResourceFormat::R32UInt || sampleDomain->m_Size != size || sampleDomain->m_uiSampleCount != 1 || !sampleDomain->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
      return context.ReportFailure("Local light denoising requires a matching readable R32UInt reservoir domain.");
  }
  const auto* cameraDescription = context.m_Graph.GetBufferDescription(camera);
  const auto* globalDescription = context.m_Graph.GetBufferDescription(global);
  if (!cameraDescription || cameraDescription->m_uiSize != sizeof(xiiFrameCameraConstants) || !globalDescription || globalDescription->m_uiSize != sizeof(xiiFrameGlobalConstants))
    return context.ReportFailure("Temporal signal frame constant layouts are invalid.");
  xiiUInt64 completed = 0;
  if (useHistory && xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("Temporal signal filtering requires an active shared GPU frame.");
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  auto& state = *m_pState;
  XII_LOCK(state.m_Mutex);
  auto&       pipeline = useHistory ? state.m_Pipeline : state.m_SpatialPipeline;
  const char* shader;
  if (occlusion)
    shader = useHistory ? "Shaders/Pipeline/AmbientOcclusionTemporal.xiiShader" : "Shaders/Pipeline/SpatialAmbientOcclusion.xiiShader";
  else if (localLighting)
    shader = useHistory ? "Shaders/Pipeline/LocalLightTemporalDenoise.xiiShader" : "Shaders/Pipeline/LocalLightSpatialDenoise.xiiShader";
  else
    shader = useHistory ? "Shaders/Pipeline/IrradianceTemporalDenoise.xiiShader" : "Shaders/Pipeline/IrradianceSpatialDenoise.xiiShader";
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(pipeline, shader));
  const auto temporalCamera = context.GetInputData(m_TemporalCamera).Downcast<const xiiTemporalCameraSnapshot>();
  if (useHistory && (!temporalCamera || temporalCamera->m_Sample.m_Size != size))
    return context.ReportFailure("Temporal signal filtering requires a matching explicit temporal camera snapshot.");
  const bool historyValid = useHistory && state.m_bHistoryValid && state.m_Last && state.m_Last->m_Signal->GetDescription().m_Size == size &&
    state.m_Last->m_InputFormat == sizingInput->m_Format && temporalCamera->m_Sample.m_bHistoryValid && state.m_Last->m_uiFrame == temporalCamera->m_Sample.m_uiPreviousFrame && state.m_Last->m_uiFrame < context.m_Graph.GetFrameIndex();
  xiiSharedPtr<State::Slot> next;
  if (useHistory)
  {
    for (xiiUInt32 i = state.m_Slots.GetCount(); i-- > 0;)
    {
      const auto& slot = state.m_Slots[i];
      if (slot != state.m_Last && slot->GetRefCount() == 1 && slot->m_uiLastUsedFrame <= completed)
      {
        if (slot->m_Signal->GetDescription().m_Size != size)
          state.m_Slots.RemoveAtAndSwap(i);
        else if (!next)
          next = slot;
      }
    }
    if (!next)
    {
      if (state.m_Slots.GetCount() >= State::s_uiMaximumHistorySlots)
        return context.ReportFailure("Temporal signal history capacity is exhausted by in-flight or retained recordings.");
      next = XII_DEFAULT_NEW(State::Slot);
      xiiGALTextureCreationDescription description;
      description.m_Type      = xiiGALResourceDimension::Texture2D;
      description.m_Size      = size;
      description.m_Format    = m_Format;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      next->m_Signal          = device->CreateTexture(description);
      description.m_Format    = occlusion ? xiiGALResourceFormat::RGBA16Float : xiiGALResourceFormat::RGBA32Float;
      next->m_Surface         = device->CreateTexture(description);
      if (!occlusion)
      {
        description.m_Format = localLighting ? xiiGALResourceFormat::RGBA32UInt : xiiGALResourceFormat::RG32UInt;
        next->m_Identity     = device->CreateTexture(description);
      }
      next->m_ReadyFence = device->CreateFence({.m_Type = xiiGALFenceType::General});
      if (!next->m_Signal || !next->m_Surface || !next->m_ReadyFence || (!occlusion && !next->m_Identity))
        return context.ReportFailure("Could not allocate signal history images or readiness fence.");
      state.m_Slots.PushBack(next);
    }
    ++next->m_uiReadyValue;
    next->m_uiFrame = next->m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
    next->m_Camera                            = temporalCamera->m_Sample.m_Camera;
    next->m_InputFormat                       = sizingInput->m_Format;
    if (historyValid)
      state.m_Last->m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  }
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Raw, m_Depth, m_Normal, m_Velocity, m_Previous, m_PreviousSurface, m_Output, m_Surface;
    xiiRenderGraphTextureHandle              m_Material, m_ObjectId, m_PreviousIdentity, m_Identity, m_Domain;
    xiiRenderGraphBufferHandle               m_Camera, m_Global, m_Constants, m_Reprojection;
    xiiSharedPtr<State::Slot>                m_Next, m_PreviousSlot;
    xiiAmbientOcclusionReprojectionConstants m_ReprojectionParameters;
    xiiSharedPtr<xiiGALFence>                m_ReadyFence;
    xiiUInt64                                m_uiReadyValue;
    xiiTemporalDenoiseConstants              m_Parameters;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<State>                      m_State;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiUInt64                                m_uiHistoryRevision;
  };
  const auto setup = [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size              = size;
    data.m_State             = m_pState;
    data.m_Pipeline          = pipeline;
    data.m_uiHistoryRevision = state.m_uiHistoryRevision;
    data.m_Next              = next;
    data.m_ReadyFence        = next ? next->m_ReadyFence : nullptr;
    data.m_uiReadyValue      = next ? next->m_uiReadyValue : 0;
    data.m_PreviousSlot      = historyValid ? state.m_Last : nullptr;
    if (historyValid)
      builder.WaitForFence(state.m_Last->m_ReadyFence, state.m_Last->m_uiReadyValue).AssertSuccess();
    data.m_Raw      = builder.ReadTexture(raw, xiiGALResourceStateFlags::ShaderResource);
    data.m_Depth    = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal   = builder.ReadTexture(normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Velocity = useHistory ? builder.ReadTexture(velocity, xiiGALResourceStateFlags::ShaderResource) : data.m_Normal;
    // HistoryValid == 0 exits the shader before history reads. Bind initialized
    // current inputs for the first frame and after camera cuts or size changes.
    data.m_Previous        = historyValid ? builder.ReadTexture(builder.ImportTexture("Previous Signal", state.m_Last->m_Signal, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource) : data.m_Raw;
    data.m_PreviousSurface = historyValid ? builder.ReadTexture(builder.ImportTexture("Previous Surface", state.m_Last->m_Surface, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource) : data.m_Normal;
    if (localLighting)
      data.m_Domain = builder.ReadTexture(domain, xiiGALResourceStateFlags::ShaderResource);
    if (!occlusion)
    {
      data.m_Material = builder.ReadTexture(material, xiiGALResourceStateFlags::ShaderResource);
      data.m_ObjectId = builder.ReadTexture(objectId, xiiGALResourceStateFlags::ShaderResource);
      if (useHistory)
      {
        data.m_PreviousIdentity = historyValid ? builder.ReadTexture(builder.ImportTexture("Previous signal identity", state.m_Last->m_Identity, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource) : data.m_ObjectId;
        data.m_Identity         = builder.WriteTexture(builder.ImportTexture("Signal identity", next->m_Identity, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::UnorderedAccess);
      }
    }
    if (useHistory)
    {
      data.m_Output  = builder.WriteTexture(builder.ImportTexture("Filtered signal", next->m_Signal, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::UnorderedAccess);
      data.m_Surface = builder.WriteTexture(builder.ImportTexture("Signal surface", next->m_Surface, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::UnorderedAccess);
    }
    else
    {
      xiiGALTextureCreationDescription description;
      description.m_Type      = xiiGALResourceDimension::Texture2D;
      description.m_Size      = size;
      description.m_Format    = m_Format;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      data.m_Output           = builder.WriteTexture("Spatial signal", description, xiiGALResourceStateFlags::UnorderedAccess);
    }
    if (useHistory)
    {
      const auto& previous                                 = historyValid ? state.m_Last->m_Camera : next->m_Camera;
      data.m_ReprojectionParameters.PreviousCameraPosition = previous.m_CameraPositionAndNearPlane;
      data.m_ReprojectionParameters.PreviousCameraForward  = previous.m_CameraForwardAndFarPlane;
      data.m_ReprojectionParameters.JitterDelta            = temporalCamera->m_Sample.m_PreviousJitter - temporalCamera->m_Sample.m_Jitter;
      data.m_ReprojectionParameters._ReprojectionPadding   = xiiVec2::MakeZero();
      data.m_Reprojection                                  = xiiRenderPassUtils::Constants(builder, "Signal Reprojection", sizeof(data.m_ReprojectionParameters));
    }
    data.m_Camera                     = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Global                     = builder.ReadBuffer(global, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Constants                  = xiiRenderPassUtils::Constants(builder, "Signal Temporal Constants", sizeof(data.m_Parameters));
    data.m_Parameters.HistoryWeight   = m_fHistoryWeight;
    data.m_Parameters.DepthThreshold  = m_fDepthThreshold;
    data.m_Parameters.NormalThreshold = m_fNormalThreshold;
    data.m_Parameters.SpatialWeight   = m_fSpatialWeight;
    data.m_Parameters.HistoryValid    = historyValid ? 1U : 0U;
    data.m_Parameters.SignalMode      = occlusion ? 0U : 1U;
    data.m_Parameters._Padding        = xiiVec2::MakeZero();
  };
  const auto record = [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiTemporalDenoiseConstants> constants(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *constants = data.m_Parameters;
    }
    if (data.m_Reprojection.IsValid())
    {
      xiiGALMapHelper<xiiAmbientOcclusionReprojectionConstants> mapped(commands, execute.GetBuffer(data.m_Reprojection), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = data.m_ReprojectionParameters;
    }
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    if (data.m_Reprojection.IsValid())
      commands.ResolveAndSetConstantBuffer("xiiAmbientOcclusionReprojectionConstants", execute.GetBuffer(data.m_Reprojection), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("PerFrameCamera", execute.GetBuffer(data.m_Camera), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("PerFrameGlobal", execute.GetBuffer(data.m_Global), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiTemporalDenoiseConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_CurrentSignal", execute.GetTexture(data.m_Raw)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_PreviousSignal", execute.GetTexture(data.m_Previous)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SceneDepth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Velocity", execute.GetTexture(data.m_Velocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", execute.GetTexture(data.m_PreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    if (data.m_Domain.IsValid())
      commands.ResolveAndSetShaderResourceTextureView("g_SampleDomain", execute.GetTexture(data.m_Domain)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    if (data.m_Material.IsValid())
    {
      commands.ResolveAndSetShaderResourceTextureView("g_GBufferMaterial", execute.GetTexture(data.m_Material)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceTextureView("g_ObjectId", execute.GetTexture(data.m_ObjectId)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      if (data.m_Identity.IsValid())
      {
        commands.ResolveAndSetShaderResourceTextureView("g_PreviousIdentity", execute.GetTexture(data.m_PreviousIdentity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
        commands.ResolveAndSetUnorderedAccessTextureView("g_CurrentIdentity", execute.GetTexture(data.m_Identity)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      }
    }
    commands.ResolveAndSetUnorderedAccessTextureView("g_DenoisedOutput", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (data.m_Surface.IsValid())
      commands.ResolveAndSetUnorderedAccessTextureView("g_CurrentSurface", execute.GetTexture(data.m_Surface)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      execute.ReportFailure("Could not bind temporal signal filtering resources.");
      return;
    }
    commands.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1});
    if (data.m_ReadyFence)
    {
      commands.EnqueueSignal(data.m_ReadyFence.Borrow(), data.m_uiReadyValue);
      XII_LOCK(data.m_State->m_Mutex);
      if (data.m_uiHistoryRevision == data.m_State->m_uiHistoryRevision && (!data.m_State->m_bHistoryValid || data.m_State->m_Last->m_uiFrame < data.m_Next->m_uiFrame))
      {
        data.m_State->m_Last          = data.m_Next;
        data.m_State->m_bHistoryValid = true;
      }
    }
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, setup, record);
  if (emitted.first->m_Surface.IsValid())
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Surface, emitted.first->m_Surface));
  return context.SetOutput(m_Output, emitted.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Denoising_Implementation_TemporalSignalDenoisePass);
