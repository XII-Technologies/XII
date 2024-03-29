
XII_ALWAYS_INLINE xiiUInt64 xiiGALFenceD3D11::GetCompletedValue()
{
  xiiUInt64 uiValue = m_pFence->GetCompletedValue();
  XII_ASSERT_DEV(uiValue != xiiMath::MaxValue<xiiUInt64>(), "If the device has been removed, the return value will be UINT64_MAX.");
  return uiValue;
}

XII_ALWAYS_INLINE void xiiGALFenceD3D11::Signal(xiiUInt64 uiValue)
{
  XII_ASSERT_DEV(m_Description.m_Type == xiiGALFenceType::General, "The fence must be created with xiiGALFenceType::General.");
  XII_ASSERT_DEV(m_pDevice->GetDescription().m_DeviceFeatures.m_NativeFence == xiiGALDeviceFeatureState::Enabled, "CPU fence signal requires the device Native Fence feature.");

  m_pFence->Signal(uiValue);
}

XII_ALWAYS_INLINE void xiiGALFenceD3D11::Wait(xiiUInt64 uiValue)
{
  if (GetCompletedValue() >= uiValue)
    return;

  if (m_pFenceCompleteEvent != nullptr)
  {
    m_pFence->SetEventOnCompletion(uiValue, m_pFenceCompleteEvent);

    WaitForSingleObject(m_pFenceCompleteEvent, INFINITE);
  }
  else
  {
    while (GetCompletedValue() < uiValue)
    {
      xiiThreadUtils::Sleep(xiiTime::Microseconds(1));
    }
  }
}

XII_ALWAYS_INLINE ID3D11Fence* xiiGALFenceD3D11::GetFence() const
{
  return m_pFence;
}
