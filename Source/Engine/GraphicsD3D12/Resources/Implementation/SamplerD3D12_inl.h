
XII_ALWAYS_INLINE D3D12_CPU_DESCRIPTOR_HANDLE xiiGALSamplerD3D12::GetCPUDescriptorHandle() const
{
  return m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}
