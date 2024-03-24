
XII_ALWAYS_INLINE xiiUInt64 xiiGALFenceVulkan::GetCompletedValue()
{
  return m_pFence->GetCompletedValue();
}

XII_ALWAYS_INLINE void xiiGALFenceVulkan::Signal(xiiUInt64 uiValue)
{
  m_pFence->Signal(uiValue);
}

XII_ALWAYS_INLINE void xiiGALFenceVulkan::Wait(xiiUInt64 uiValue)
{
  m_pFence->Wait(uiValue);
}

XII_ALWAYS_INLINE Diligent::IFence* xiiGALFenceVulkan::GetFence() const
{
  return m_pFence;
}
