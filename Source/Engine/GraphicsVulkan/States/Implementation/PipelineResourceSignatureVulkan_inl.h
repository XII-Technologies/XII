
XII_ALWAYS_INLINE bool xiiGALPipelineResourceSignatureVulkan::IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const
{
  const xiiGALPipelineResourceSignatureVulkan* pPipelineResourceSignatureVulkan = static_cast<const xiiGALPipelineResourceSignatureVulkan*>(pPipelineResourceSignature);

  XII_ASSERT_DEV(m_pPipelineResourceSignature != nullptr && pPipelineResourceSignatureVulkan != nullptr, "");

  return (m_pPipelineResourceSignature == pPipelineResourceSignatureVulkan->GetPipelineResourceSignature()) || m_pPipelineResourceSignature->IsCompatibleWith(pPipelineResourceSignatureVulkan->GetPipelineResourceSignature());
}

XII_ALWAYS_INLINE Diligent::IPipelineResourceSignature* xiiGALPipelineResourceSignatureVulkan::GetPipelineResourceSignature() const
{
  return m_pPipelineResourceSignature;
}
