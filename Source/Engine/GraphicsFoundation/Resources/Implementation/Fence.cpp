#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Fence.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALFenceType, 1)
  XII_ENUM_CONSTANT(xiiGALFenceType::CpuWaitOnly),
  XII_ENUM_CONSTANT(xiiGALFenceType::General),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALFence, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALFence::xiiGALFence(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALDeviceObject(std::move(pDevice)), m_Description(creationDescription), m_LastCompletedFenceValue(0)
{
}

xiiGALFence::~xiiGALFence() = default;

void xiiGALFence::UpdateLastCompletedFenceValue(xiiUInt64 uiValue)
{
  m_LastCompletedFenceValue.Max(uiValue);
}

void xiiGALFence::ValidateFenceSignal(xiiUInt64 uiValue)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)

  XII_ASSERT_DEV(uiValue >= m_EnqueuedFenceValue, "Fence '{}' is being signaled or enqueued for signal with value {}, but the previous value ({}) is greater than the new value. Signal operation will have no effect.", GetDebugName(), uiValue, m_EnqueuedFenceValue);

  m_EnqueuedFenceValue.Max(uiValue);
#else
  XII_IGNORE_UNUSED(uiValue);
#endif
}

void xiiGALFence::ValidateDeviceWaitForFence(xiiUInt64 uiValue)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (m_pDevice->GetFeatures().m_NativeFence != xiiGALDeviceFeatureState::Enabled)
  {
    XII_ASSERT_DEV(uiValue <= m_EnqueuedFenceValue, "Can not wait for value {} that is greater than the last enqueued for signal value ({}). This is not supported when NativeFence feature is disabled.", uiValue, m_EnqueuedFenceValue);
  }
#else
  XII_IGNORE_UNUSED(uiValue);
#endif
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Fence);
