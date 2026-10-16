/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Lighting/SensorRendering.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/SensorOutputPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Sensors/SensorOutputConstants.h>

using namespace xiiRenderPassUtils;
struct xiiSensorOutputPass::Data
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneRadiance;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hSceneNormal;
  xiiRenderGraphTextureHandle m_hBaseColor;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hSensorOutput;
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiSensorProfile            m_Profile;
  xiiUInt32                   m_uiFrameIndex = 0U;
  xiiSizeU32                  m_Size;
  xiiRenderGraphBufferHandle  m_hPerFrameCamera;
  xiiRenderGraphBufferHandle  m_hPerFrameGlobal;
};

struct xiiSensorOutputPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;

  xiiSharedPtr<xiiGALTexture> m_Output;
  xiiSensorProfileHandle      m_LastViewProfile;
  bool                        m_bOutputReady = false;
};

xiiSensorOutputPass::xiiSensorOutputPass()  = default;
xiiSensorOutputPass::~xiiSensorOutputPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSensorOutputPass, 1, xiiRTTIDefaultAllocator<xiiSensorOutputPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("UseViewProfile", m_bUseViewProfile)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Profile", m_Profile),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Sensors")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiSensorOutputPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiSensorRenderingManager::IsValidProfile(m_Profile) ? XII_SUCCESS : XII_FAILURE;
}
void xiiSensorOutputPass::InvalidateTemporalHistory()
{
  if (m_pState)
    m_pState->m_bOutputReady = false;
}
xiiSharedPtr<xiiGALTexture> xiiSensorOutputPass::GetViewOutput(xiiSensorProfileHandle profile) const
{
  return m_bUseViewProfile && !profile.IsInvalidated() && m_pState && m_pState->m_bOutputReady && profile == m_pState->m_LastViewProfile ? m_pState->m_Output : xiiSharedPtr<xiiGALTexture>();
}

void xiiSensorOutputPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  ResourcePins(pins);
  pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, m_bUseViewProfile, xiiGALResourceFormat::RGBA16Float});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, m_bUseViewProfile});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, true, m_bUseViewProfile});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"GBufferAlbedo", xiiRenderPipelinePin::Texture, true, m_bUseViewProfile});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, true, m_bUseViewProfile});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  pins.PushBack({"PerFrameCamera", xiiRenderPipelinePin::Buffer, true, m_bUseViewProfile});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UniformBuffer;
  pins.PushBack({"PerFrameGlobal", xiiRenderPipelinePin::Buffer, true, m_bUseViewProfile});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UniformBuffer;
  pins.PushBack({"SensorOutput", xiiRenderPipelinePin::Texture, false, m_bUseViewProfile, xiiGALResourceFormat::RGBA32Float});
  pins.PushBack({"Sensor Output Constants", xiiRenderPipelinePin::Buffer, false, m_bUseViewProfile});
  pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, false, m_bUseViewProfile});
  pins.PeekBack().m_Access = xiiRenderPipelinePinAccess::Read;
  for (auto& pin : pins)
    if (pin.m_Type == xiiRenderPipelinePin::Texture)
    {
      pin.m_Constraints.m_Dimension = xiiGALResourceDimension::Texture2D;
      pin.m_Constraints.m_uiSamples = 1;
    }
}

xiiResult xiiSensorOutputPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  m_pState->m_bOutputReady       = false;
  xiiSensorProfile       profile = m_Profile;
  xiiSensorProfileHandle viewProfile;
  if (m_bUseViewProfile)
  {
    if (!context.m_pView)
      return XII_FAILURE;
    viewProfile = context.m_pView->GetSensorProfile();
    if (viewProfile.IsInvalidated())
    {
      m_pState->m_Output          = nullptr;
      m_pState->m_LastViewProfile = {};
      return context.SetOutput("DilatedVelocityBuffer", context.GetInputTexture("DilatedVelocityBuffer"));
    }
    if (xiiSensorRenderingManager::GetProfile(viewProfile, profile).Failed())
      return XII_FAILURE;
  }
  xiiGALTextureCreationDescription description;
  description.m_Type      = xiiGALResourceDimension::Texture2D;
  description.m_Format    = xiiGALResourceFormat::RGBA32Float;
  description.m_Size      = {profile.m_uiResolutionX, profile.m_uiResolutionY};
  description.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  if (!m_pState->m_Output || m_pState->m_Output->GetDescription() != description)
  {
    m_pState->m_Output = nullptr;
    if (xiiGALDevice::HasDefaultDevice())
      m_pState->m_Output = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    if (!m_pState->m_Output)
    {
      xiiLog::Error("Failed to allocate calibrated sensor output ({} x {}).", profile.m_uiResolutionX, profile.m_uiResolutionY);
      return XII_FAILURE;
    }
  }
  m_pState->m_LastViewProfile = viewProfile;
  // View-driven sensors have optional pins while inactive. Once a profile is selected,
  // reject incomplete bindings before graph builder access rather than touching invalid handles.
  for (xiiStringView input : {"HDRSceneColor", "SceneDepth", "GBufferNormal", "GBufferAlbedo", "DilatedVelocityBuffer"})
    if (!context.GetInputTexture(input).IsValid())
      return XII_FAILURE;
  if (!context.GetInputBuffer("PerFrameCamera").IsValid() || !context.GetInputBuffer("PerFrameGlobal").IsValid())
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/SensorOutput.xiiShader"));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, context.GetInputTexture("HDRSceneColor"), 0, 0, size));
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_hSceneRadiance = context.GetInputTexture("HDRSceneColor");
    data.m_hSceneDepth = context.GetInputTexture("SceneDepth");
    data.m_hSceneNormal = context.GetInputTexture("GBufferNormal");
    data.m_hBaseColor = context.GetInputTexture("GBufferAlbedo");
    data.m_hVelocity = context.GetInputTexture("DilatedVelocityBuffer");
    data.m_hPerFrameCamera = context.GetInputBuffer("PerFrameCamera");
    data.m_hPerFrameGlobal = context.GetInputBuffer("PerFrameGlobal");
    data.m_Profile = profile;
    data.m_uiFrameIndex = static_cast<xiiUInt32>(context.m_Graph.GetFrameIndex());
    Setup(data, builder); }, xiiMakeDelegate(&xiiSensorOutputPass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput("SensorOutput", emitted.first->m_hSensorOutput));
  XII_SUCCEED_OR_RETURN(context.SetOutput("Sensor Output Constants", emitted.first->m_hConstants));
  XII_SUCCEED_OR_RETURN(context.SetOutput("DilatedVelocityBuffer", context.GetInputTexture("DilatedVelocityBuffer")));
  return XII_SUCCESS;
}

void xiiSensorOutputPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneRadiance = builder.ReadTexture(data.m_hSceneRadiance, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth    = builder.ReadTexture(data.m_hSceneDepth, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneNormal   = builder.ReadTexture(data.m_hSceneNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBaseColor     = builder.ReadTexture(data.m_hBaseColor, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity      = builder.ReadTexture(data.m_hVelocity, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSensorOutput  = builder.WriteTexture(builder.ImportTexture("SensorOutput", m_pState->m_Output, m_pState->m_Output->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiSensorOutputConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Sensor Output Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  builder.ExportTexture(data.m_hSensorOutput, xiiGALResourceStateFlags::ShaderResource);
  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);

  data.m_hPerFrameCamera = builder.ReadBuffer(data.m_hPerFrameCamera, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hPerFrameGlobal = builder.ReadBuffer(data.m_hPerFrameGlobal, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiSensorOutputPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  if (!data.m_hSceneRadiance.IsValid() || !data.m_hSceneDepth.IsValid() || !data.m_hSceneNormal.IsValid() || !data.m_hBaseColor.IsValid() || !data.m_hVelocity.IsValid() || !data.m_hSensorOutput.IsValid() || !data.m_hConstants.IsValid())
    return;

  xiiGALCommandList& cmd = context.GetCommandList();
  cmd.BeginDebugGroup("CalibratedSensorOutput");
  {
    {
      xiiGALMapHelper<xiiSensorOutputConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->Intrinsics               = xiiVec4(data.m_Profile.m_fFocalLengthXPixels, data.m_Profile.m_fFocalLengthYPixels, data.m_Profile.m_fPrincipalPointXPixels, data.m_Profile.m_fPrincipalPointYPixels);
      pConstants->RadialDistortion         = xiiVec4(data.m_Profile.m_vRadialDistortion.x, data.m_Profile.m_vRadialDistortion.y, data.m_Profile.m_vRadialDistortion.z, 0.0f);
      pConstants->TangentialDistortion     = xiiVec4(data.m_Profile.m_vTangentialDistortion.x, data.m_Profile.m_vTangentialDistortion.y, 0.0f, 0.0f);
      pConstants->RangeAndExposure         = xiiVec4(data.m_Profile.m_fNearPlaneMeters, data.m_Profile.m_fFarPlaneMeters, data.m_Profile.m_fExposureSeconds, data.m_Profile.m_fRollingShutterSeconds);
      pConstants->SpectralSensitivityAndQE = xiiVec4(data.m_Profile.m_vSpectralSensitivity.x, data.m_Profile.m_vSpectralSensitivity.y, data.m_Profile.m_vSpectralSensitivity.z, data.m_Profile.m_fQuantumEfficiency);
      pConstants->SignalConversion         = xiiVec4(data.m_Profile.m_fRadianceToElectrons, data.m_Profile.m_fAnalogGain, data.m_Profile.m_fSaturationElectrons, data.m_Profile.m_fWavelengthNanometers);
      pConstants->NoiseParameters          = xiiVec4(data.m_Profile.m_fReadNoiseElectrons, data.m_Profile.m_fShotNoiseScale, data.m_Profile.m_fDepthNoiseStandardDeviationMeters, data.m_Profile.m_fDepthNoiseScalePerMeter);
      pConstants->OutputDescription        = xiiVec4U32(data.m_Profile.m_Type.GetValue(), data.m_Profile.m_NoiseModel.GetValue(), data.m_Profile.m_uiOutputBitDepth, data.m_Profile.m_uiNoiseSeed);
      pConstants->FrameAndResolution       = xiiVec4U32(data.m_uiFrameIndex, data.m_Profile.m_uiResolutionX, data.m_Profile.m_uiResolutionY, data.m_Profile.m_Shutter.GetValue());
    }

    cmd.SetPipelineState(m_pState->m_Pipeline);
    cmd.ResolveAndSetConstantBuffer("PerFrameCamera", context.GetBuffer(data.m_hPerFrameCamera), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("PerFrameGlobal", context.GetBuffer(data.m_hPerFrameGlobal), xiiGALShaderType::Compute);


    cmd.ResolveAndSetConstantBuffer("xiiSensorOutputConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneRadiance", context.GetTexture(data.m_hSceneRadiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneNormal", context.GetTexture(data.m_hSceneNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_BaseColor", context.GetTexture(data.m_hBaseColor)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SensorOutput", context.GetTexture(data.m_hSensorOutput)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_Profile.m_uiResolutionX + 7U) / 8U, (data.m_Profile.m_uiResolutionY + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
  m_pState->m_bOutputReady = true;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Sensors_SensorOutputPass);
