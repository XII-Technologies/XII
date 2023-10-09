
XII_ALWAYS_INLINE xiiUInt64 xiiGALFenceD3D12::GetCompletedValue()
{
  return m_pFence->GetCompletedValue();
}

XII_ALWAYS_INLINE void xiiGALFenceD3D12::Signal(xiiUInt64 uiValue)
{
  m_pFence->Signal(uiValue);
}

XII_ALWAYS_INLINE void xiiGALFenceD3D12::Wait(xiiUInt64 uiValue)
{
  m_pFence->Wait(uiValue);
}

XII_ALWAYS_INLINE const Diligent::IFence* xiiGALFenceD3D12::GetFence() const
{
  return m_pFence;
}
