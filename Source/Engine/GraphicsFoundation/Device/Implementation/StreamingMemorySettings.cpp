/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Device/StreamingMemorySettings.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALStreamingMemorySettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGALStreamingMemorySettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("UseNativeBudget", m_bUseNativeBudget),
    XII_MEMBER_PROPERTY("BudgetMiB", m_uiBudgetMiB)->AddAttributes(new xiiClampValueAttribute(1, 1048576)),
    XII_MEMBER_PROPERTY("HeadroomMiB", m_uiHeadroomMiB)->AddAttributes(new xiiClampValueAttribute(0, 1048576)),
    XII_MEMBER_PROPERTY("PressureHighPercent", m_uiPressureHighPercent)->AddAttributes(new xiiClampValueAttribute(2, 100)),
    XII_MEMBER_PROPERTY("PressureLowPercent", m_uiPressureLowPercent)->AddAttributes(new xiiClampValueAttribute(1, 99)),
  } XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiGALStreamingMemorySettings::Validate() const
{
  return m_uiBudgetMiB > 0 && m_uiBudgetMiB <= 1048576U && m_uiHeadroomMiB <= 1048576U && m_uiPressureHighPercent <= 100U && m_uiPressureHighPercent > m_uiPressureLowPercent && m_uiPressureLowPercent > 0 ? XII_SUCCESS : XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_StreamingMemorySettings);
