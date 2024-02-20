
XII_ALWAYS_INLINE bool xiiGALPipelineResourceSignatureD3D12::IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const
{
  const xiiGALPipelineResourceSignatureD3D12* pPipelineResourceSignatureD3D12 = static_cast<const xiiGALPipelineResourceSignatureD3D12*>(pPipelineResourceSignature);

  XII_ASSERT_DEV(m_pPipelineResourceSignature != nullptr && pPipelineResourceSignatureD3D12 != nullptr, "");

  return m_pPipelineResourceSignature->IsCompatibleWith(pPipelineResourceSignatureD3D12->GetPipelineResourceSignature());
}

XII_ALWAYS_INLINE Diligent::IPipelineResourceSignature* xiiGALPipelineResourceSignatureD3D12::GetPipelineResourceSignature() const
{
  return m_pPipelineResourceSignature;
}
