/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/Pipeline/RenderPipelineSettings.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Resources/Texture.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiRenderPipelineDeviceFeature, 1)
  XII_BITFLAGS_CONSTANTS(xiiRenderPipelineDeviceFeature::None, xiiRenderPipelineDeviceFeature::Compute,
    xiiRenderPipelineDeviceFeature::MeshShaders, xiiRenderPipelineDeviceFeature::RayTracing, xiiRenderPipelineDeviceFeature::BindlessResources)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineSettings, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_MEMBER_PROPERTY("RequiredDeviceFeatures", xiiRenderPipelineDeviceFeature, m_RequiredDeviceFeatures),
    XII_ENUM_MEMBER_PROPERTY("OutputFormat", xiiGALResourceFormat, m_OutputFormat),
    XII_MEMBER_PROPERTY("OutputSamples", m_uiOutputSamples)->AddAttributes(new xiiClampValueAttribute(0U, 64U)),
  } XII_END_PROPERTIES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  xiiResult Fail(xiiStringBuilder* error, xiiStringView message)
  {
    if (error)
      error->Set(message);
    return XII_FAILURE;
  }
} // namespace

xiiResult xiiRenderPipelineSettings::Validate(xiiStringBuilder* out_error) const
{
  if (xiiReflectionUtils::MakeEnumerationValid(xiiGetStaticRTTI<xiiRenderPipelineDeviceFeature>(), m_RequiredDeviceFeatures.GetValue()) != m_RequiredDeviceFeatures.GetValue())
    return Fail(out_error, "Pipeline RequiredDeviceFeatures contains unknown flags.");
  if (m_OutputFormat.GetValue() >= xiiGALResourceFormat::ENUM_COUNT)
    return Fail(out_error, "Pipeline OutputFormat is invalid.");
  if (m_uiOutputSamples && (m_uiOutputSamples > 64 || !xiiMath::IsPowerOf2(m_uiOutputSamples)))
    return Fail(out_error, "Pipeline OutputSamples must be zero or a supported power of two.");
  return XII_SUCCESS;
}

xiiResult xiiRenderPipelineSettings::ValidateDevice(const xiiGALDeviceFeatures* features, xiiStringBuilder* out_error) const
{
  if (!m_RequiredDeviceFeatures.IsAnyFlagSet())
    return XII_SUCCESS;
  if (!features)
    return Fail(out_error, "Pipeline requires device features but no graphics device is available.");
  const auto require = [&](xiiRenderPipelineDeviceFeature::Enum flag, xiiGALDeviceFeatureState::Enum state, xiiStringView name) {
    if (!m_RequiredDeviceFeatures.IsSet(flag) || state == xiiGALDeviceFeatureState::Enabled)
      return XII_SUCCESS;
    if (out_error)
      out_error->SetFormat("Pipeline RequiredDeviceFeatures: '{}' is unavailable on the selected device.", name);
    return XII_FAILURE;
  };
  XII_SUCCEED_OR_RETURN(require(xiiRenderPipelineDeviceFeature::Compute, features->m_ComputeShaders, "Compute"));
  XII_SUCCEED_OR_RETURN(require(xiiRenderPipelineDeviceFeature::MeshShaders, features->m_MeshShaders, "MeshShaders"));
  XII_SUCCEED_OR_RETURN(require(xiiRenderPipelineDeviceFeature::RayTracing, features->m_RayTracing, "RayTracing"));
  return require(xiiRenderPipelineDeviceFeature::BindlessResources, features->m_BindlessResources, "BindlessResources");
}

xiiResult xiiRenderPipelineSettings::ValidateOutput(const xiiGALTextureCreationDescription* output, xiiStringBuilder* out_error) const
{
  if (m_OutputFormat == xiiGALResourceFormat::Unknown && !m_uiOutputSamples)
    return XII_SUCCESS;
  if (!output)
    return Fail(out_error, "Pipeline output contract requires a selected output texture.");
  if (m_OutputFormat != xiiGALResourceFormat::Unknown && m_OutputFormat != output->m_Format)
    return Fail(out_error, "Selected output texture conflicts with pipeline OutputFormat.");
  if (m_uiOutputSamples && m_uiOutputSamples != output->m_uiSampleCount)
    return Fail(out_error, "Selected output texture conflicts with pipeline OutputSamples.");
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelineSettings);
