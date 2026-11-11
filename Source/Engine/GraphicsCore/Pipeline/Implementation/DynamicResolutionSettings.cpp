/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/DynamicResolutionSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicResolutionSettings, 1, xiiRTTIDefaultAllocator<xiiDynamicResolutionSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled),
    XII_MEMBER_PROPERTY("RenderScale", m_fRenderScale)->AddAttributes(new xiiClampValueAttribute(0.1, 1)),
    XII_MEMBER_PROPERTY("TargetFrameTimeMs", m_fTargetFrameTimeMs)->AddAttributes(new xiiClampValueAttribute(0.1, 1000)),
    XII_MEMBER_PROPERTY("MinimumRenderScale", m_fMinimumRenderScale)->AddAttributes(new xiiClampValueAttribute(0.25, 1)),
    XII_MEMBER_PROPERTY("MaximumRenderScale", m_fMaximumRenderScale)->AddAttributes(new xiiClampValueAttribute(0.25, 1)),
    XII_MEMBER_PROPERTY("ProportionalGain", m_fProportionalGain)->AddAttributes(new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("IntegralGain", m_fIntegralGain)->AddAttributes(new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("DerivativeGain", m_fDerivativeGain)->AddAttributes(new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("IntegralLimit", m_fIntegralLimit)->AddAttributes(new xiiClampValueAttribute(0, 100)),
    XII_MEMBER_PROPERTY("MaximumScaleStep", m_fMaximumScaleStep)->AddAttributes(new xiiClampValueAttribute(0.001, 1)),
    XII_MEMBER_PROPERTY("Smoothing", m_fSmoothing)->AddAttributes(new xiiClampValueAttribute(0, 1)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Resolution"),
    new xiiViewRenderSettingsAttribute(),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiDynamicResolutionSettings::Validate() const
{
  XII_SUCCEED_OR_RETURN(SUPER::Validate());
  if (m_fMinimumRenderScale > m_fMaximumRenderScale)
    return XII_FAILURE;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_DynamicResolutionSettings);
