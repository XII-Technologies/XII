
XII_ALWAYS_INLINE D3D11_CPU_DESCRIPTOR_HANDLE xiiGALSamplerD3D11::GetCPUDescriptorHandle() const
{
  return m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}
