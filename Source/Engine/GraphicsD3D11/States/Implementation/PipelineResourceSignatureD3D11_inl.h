
XII_ALWAYS_INLINE bool xiiGALPipelineResourceSignatureD3D11::IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const
{
  const xiiGALPipelineResourceSignatureD3D11* pPipelineResourceSignatureD3D11 = static_cast<const xiiGALPipelineResourceSignatureD3D11*>(pPipelineResourceSignature);

  XII_ASSERT_DEV(m_pPipelineResourceSignature != nullptr && pPipelineResourceSignatureD3D11 != nullptr, "");

  return (m_pPipelineResourceSignature == pPipelineResourceSignatureD3D11->GetPipelineResourceSignature()) || m_pPipelineResourceSignature->IsCompatibleWith(pPipelineResourceSignatureD3D11->GetPipelineResourceSignature());
}

XII_ALWAYS_INLINE Diligent::IPipelineResourceSignature* xiiGALPipelineResourceSignatureD3D11::GetPipelineResourceSignature() const
{
  return m_pPipelineResourceSignature;
}
