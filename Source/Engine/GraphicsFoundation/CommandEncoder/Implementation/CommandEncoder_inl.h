
XII_ALWAYS_INLINE xiiGALDevice& xiiGALCommandEncoder::GetDevice()
{
  return m_Device;
}

XII_ALWAYS_INLINE void xiiGALCommandEncoder::AssertRenderingThread() const
{
  XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "This function may only be executed on the main thread.");
}

XII_ALWAYS_INLINE void xiiGALCommandEncoder::CountStateChange()
{
  ++m_uiStateChanges;
}

XII_ALWAYS_INLINE void xiiGALCommandEncoder::CountRedundantStateChange()
{
  ++m_uiRedundantStateChanges;
}
