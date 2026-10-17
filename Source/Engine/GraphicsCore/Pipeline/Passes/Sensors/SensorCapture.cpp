/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/Threading/AtomicInteger.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/SensorCapture.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Sensors/SensorOutputConstants.h>

struct xiiSensorCaptureResult : xiiRefCounted
{
  xiiSharedPtr<xiiGALTexture> m_Output;
  xiiSensorProfileHandle      m_Profile;
  xiiAtomicBool               m_Recorded;
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSensorCapture, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneDepth", m_InputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinOptionalAttribute("UseViewProfile")),
    XII_MEMBER_PROPERTY("Velocity", m_InputVelocity)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PerFrameCamera", m_InputPerFrameCamera)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinOptionalAttribute("UseViewProfile")),
    XII_MEMBER_PROPERTY("PerFrameGlobal", m_InputPerFrameGlobal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SensorOutput", m_OutputSensor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinOptionalAttribute("UseViewProfile")),
    XII_MEMBER_PROPERTY("Constants", m_OutputConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinOptionalAttribute("UseViewProfile")),
    XII_MEMBER_PROPERTY("UseViewProfile", m_bUseViewProfile)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Calibration", m_Calibration),
  } XII_END_PROPERTIES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiSensorCapture::xiiSensorCapture(xiiSensorType::Enum type) : m_Type(type) {}
xiiSensorCapture::~xiiSensorCapture() = default;
xiiResult xiiSensorCapture::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  xiiSensorProfile profile;
  static_cast<xiiSensorCalibration&>(profile) = m_Calibration;
  profile.m_Type                              = m_Type;
  return xiiSensorRenderingManager::IsValidProfile(profile) ? XII_SUCCESS : XII_FAILURE;
}
void                        xiiSensorCapture::InvalidateTemporalHistory() { m_Capture = nullptr; }
xiiSharedPtr<xiiGALTexture> xiiSensorCapture::GetViewOutput(xiiSensorProfileHandle profile) const
{
  return m_bUseViewProfile && !profile.IsInvalidated() && m_Capture && m_Capture->m_Profile == profile && m_Capture->m_Recorded ? m_Capture->m_Output : xiiSharedPtr<xiiGALTexture>();
}
xiiResult xiiSensorCapture::Prepare(xiiRenderPipelinePassContext& context, Capture& capture)
{
  m_Capture                                             = nullptr;
  static_cast<xiiSensorCalibration&>(capture.m_Profile) = m_Calibration;
  capture.m_Profile.m_Type                              = m_Type;
  if (m_bUseViewProfile)
  {
    if (!context.m_pView)
      return context.ReportFailure("A view-driven sensor requires an explicit view.");
    capture.m_ViewProfile = context.m_pView->GetSensorProfile();
    if (capture.m_ViewProfile.IsInvalidated())
    {
      m_Output = nullptr;
      return XII_SUCCESS;
    }
    if (xiiSensorRenderingManager::GetProfile(capture.m_ViewProfile, capture.m_Profile).Failed())
      return context.ReportFailure("The selected sensor profile has expired.");
  }
  if (capture.m_Profile.m_Type != m_Type || !xiiSensorRenderingManager::IsValidProfile(capture.m_Profile))
    return context.ReportFailure("The selected calibration is invalid or belongs to another sensor algorithm.");
  capture.m_bActive = true;
  return XII_SUCCESS;
}
bool xiiSensorCapture::IsColorInput(const xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle image, xiiUInt32 channels)
{
  const auto* desc = context.m_Graph.GetTextureDescription(image);
  if (!desc || desc->m_Type != xiiGALResourceDimension::Texture2D || desc->m_uiArraySizeOrDepth != 1 || desc->m_uiSampleCount != 1 || !desc->m_Size.width || !desc->m_Size.height || !desc->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    return false;
  return channels == 2 ? desc->m_Format == xiiGALResourceFormat::RG16Float || desc->m_Format == xiiGALResourceFormat::RG32Float :
                         desc->m_Format == xiiGALResourceFormat::RGBA16Float || desc->m_Format == xiiGALResourceFormat::RGBA32Float || desc->m_Format == xiiGALResourceFormat::RGBA8UNormalized || desc->m_Format == xiiGALResourceFormat::RGBA8UNormalizedSRGB;
}
xiiResult xiiSensorCapture::Record(xiiRenderPipelinePassContext& context, const Capture& capture, const Inputs& inputs, xiiStringView shaderPath)
{
  const auto  depth = context.GetInputTexture(m_InputSceneDepth), velocity = context.GetInputTexture(m_InputVelocity);
  const auto  camera = context.GetInputBuffer(m_InputPerFrameCamera), global = context.GetInputBuffer(m_InputPerFrameGlobal);
  const auto* depthDesc  = context.m_Graph.GetTextureDescription(depth);
  const auto* cameraDesc = context.m_Graph.GetBufferDescription(camera);
  const auto* globalDesc = context.m_Graph.GetBufferDescription(global);
  const bool  rolling    = capture.m_Profile.m_Shutter == xiiSensorShutterType::Rolling && capture.m_Profile.m_fRollingShutterSeconds > 0;
  if (!depthDesc || depthDesc->m_Type != xiiGALResourceDimension::Texture2D || depthDesc->m_uiArraySizeOrDepth != 1 || depthDesc->m_uiSampleCount != 1 ||
      (depthDesc->m_Format != xiiGALResourceFormat::D32Float && depthDesc->m_Format != xiiGALResourceFormat::R32Float) || !depthDesc->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) ||
      !cameraDesc || cameraDesc->m_uiSize != sizeof(xiiFrameCameraConstants))
    return context.ReportFailure("Sensor capture requires canonical depth and camera constants.");
  const auto matchingSize = [&](xiiRenderGraphTextureHandle image) {
    const auto* desc = context.m_Graph.GetTextureDescription(image);
    return !image.IsValid() || (desc && desc->m_Size == depthDesc->m_Size);
  };
  if (!matchingSize(inputs.m_Radiance) || !matchingSize(inputs.m_Normal) || !matchingSize(inputs.m_Albedo) ||
      (rolling && (!IsColorInput(context, velocity, 2) || !matchingSize(velocity) || !globalDesc || globalDesc->m_uiSize != sizeof(xiiFrameGlobalConstants))))
    return context.ReportFailure("Sensor input sizes disagree, or rolling shutter lacks matching velocity/frame-time inputs.");
  if (!xiiGALDevice::HasDefaultDevice())
    return context.ReportFailure("Sensor output allocation requires an active graphics device.");
  xiiGALTextureCreationDescription output;
  output.m_Type      = xiiGALResourceDimension::Texture2D;
  output.m_Format    = xiiGALResourceFormat::RGBA32Float;
  output.m_Size      = {capture.m_Profile.m_uiResolutionX, capture.m_Profile.m_uiResolutionY};
  output.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  if (!m_Output || m_Output->GetDescription() != output)
    m_Output = xiiGALDevice::GetDefaultDevice()->CreateTexture(output);
  if (!m_Output)
    return context.ReportFailure("Failed to allocate calibrated sensor output.");
  xiiHashTable<xiiHashedString, xiiHashedString> variables;
  xiiHashedString                                name, value;
  name.Assign("SENSOR_ROLLING_SHUTTER");
  value.Assign(rolling ? "TRUE" : "FALSE");
  variables.Insert(name, value);
  const auto                                    handle = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>(shaderPath), variables, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(handle, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("The selected native sensor shader is unavailable.");
  xiiGALComputePipelineStateCreationDescription pipelineDesc;
  pipelineDesc.m_pPipelineResourceSignature = shader->GetPipelineResourceSignature();
  pipelineDesc.m_pComputeShader             = shader->GetGALShader(xiiGALShaderType::Compute);
  const auto pipeline                       = xiiGALPipelineCache::GetPipeline(pipelineDesc);
  if (!pipeline)
    return context.ReportFailure("Could not create the native sensor compute pipeline.");
  m_Capture            = XII_DEFAULT_NEW(xiiSensorCaptureResult);
  m_Capture->m_Output  = m_Output;
  m_Capture->m_Profile = capture.m_ViewProfile;
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Depth, m_Velocity, m_Output;
    Inputs                                   m_Inputs;
    xiiRenderGraphBufferHandle               m_Camera, m_Global, m_Constants;
    xiiSensorOutputConstants                 m_Snapshot;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiSharedPtr<xiiSensorCaptureResult>     m_Capture;
    xiiSizeU32                               m_Size;
    bool                                     m_Rolling;
  };
  xiiRenderPassResourceScope scope(context);
  const auto                 emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Camera = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Rolling = rolling;
    if (rolling) { data.m_Velocity = builder.ReadTexture(velocity, xiiGALResourceStateFlags::ShaderResource); data.m_Global = builder.ReadBuffer(global, xiiGALResourceStateFlags::ConstantBuffer); }
    if (inputs.m_Radiance.IsValid()) data.m_Inputs.m_Radiance = builder.ReadTexture(inputs.m_Radiance, xiiGALResourceStateFlags::ShaderResource);
    if (inputs.m_Normal.IsValid()) data.m_Inputs.m_Normal = builder.ReadTexture(inputs.m_Normal, xiiGALResourceStateFlags::ShaderResource);
    if (inputs.m_Albedo.IsValid()) data.m_Inputs.m_Albedo = builder.ReadTexture(inputs.m_Albedo, xiiGALResourceStateFlags::ShaderResource);
    data.m_Output = builder.WriteTexture(builder.ImportTexture("SensorOutput", m_Output, m_Output->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
    builder.ExportTexture(data.m_Output, xiiGALResourceStateFlags::ShaderResource);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "Sensor Calibration", sizeof(data.m_Snapshot));
    const auto& p = capture.m_Profile;
    data.m_Snapshot.Intrinsics = xiiVec4(p.m_fFocalLengthXPixels,p.m_fFocalLengthYPixels,p.m_fPrincipalPointXPixels,p.m_fPrincipalPointYPixels);
    data.m_Snapshot.RadialDistortion = xiiVec4(p.m_vRadialDistortion.x,p.m_vRadialDistortion.y,p.m_vRadialDistortion.z,0);
    data.m_Snapshot.TangentialDistortion = xiiVec4(p.m_vTangentialDistortion.x,p.m_vTangentialDistortion.y,0,0);
    data.m_Snapshot.RangeAndExposure = xiiVec4(p.m_fNearPlaneMeters,p.m_fFarPlaneMeters,p.m_fExposureSeconds,p.m_fRollingShutterSeconds);
    data.m_Snapshot.SpectralSensitivityAndQE = xiiVec4(p.m_vSpectralSensitivity.x,p.m_vSpectralSensitivity.y,p.m_vSpectralSensitivity.z,p.m_fQuantumEfficiency);
    data.m_Snapshot.SignalConversion = xiiVec4(p.m_fRadianceToElectrons,p.m_fAnalogGain,p.m_fSaturationElectrons,p.m_fWavelengthNanometers);
    data.m_Snapshot.NoiseParameters = xiiVec4(p.m_fReadNoiseElectrons,p.m_fShotNoiseScale,p.m_fDepthNoiseStandardDeviationMeters,p.m_fDepthNoiseScalePerMeter);
    data.m_Snapshot.OutputDescription = xiiVec4U32(p.m_Type.GetValue(),p.m_NoiseModel.GetValue(),p.m_uiOutputBitDepth,p.m_uiNoiseSeed);
    data.m_Snapshot.FrameAndResolution = xiiVec4U32(xiiUInt32(context.m_Graph.GetFrameIndex()),p.m_uiResolutionX,p.m_uiResolutionY,p.m_Shutter.GetValue());
    data.m_Pipeline = pipeline; data.m_Capture = m_Capture; data.m_Size = output.m_Size; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    { xiiGALMapHelper<xiiSensorOutputConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard); *mapped = data.m_Snapshot; }
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("PerFrameCamera", execute.GetBuffer(data.m_Camera), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiSensorOutputConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SceneDepth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    if (data.m_Rolling) {
      commands.ResolveAndSetConstantBuffer("PerFrameGlobal", execute.GetBuffer(data.m_Global), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceTextureView("g_Velocity", execute.GetTexture(data.m_Velocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    }
    if (data.m_Inputs.m_Radiance.IsValid()) commands.ResolveAndSetShaderResourceTextureView("g_SceneRadiance", execute.GetTexture(data.m_Inputs.m_Radiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    if (data.m_Inputs.m_Normal.IsValid()) commands.ResolveAndSetShaderResourceTextureView("g_SceneNormal", execute.GetTexture(data.m_Inputs.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    if (data.m_Inputs.m_Albedo.IsValid()) commands.ResolveAndSetShaderResourceTextureView("g_BaseColor", execute.GetTexture(data.m_Inputs.m_Albedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_SensorOutput", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
    commands.DispatchCompute({(data.m_Size.width+7)/8,(data.m_Size.height+7)/8,1});
    data.m_Capture->m_Recorded = true; }, true);
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputSensor, emitted.first->m_Output));
  return context.SetOutput(m_OutputConstants, emitted.first->m_Constants);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Sensors_SensorCapture);
