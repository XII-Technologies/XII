
XII_ALWAYS_INLINE xiiGALDevice& xiiGALCommandEncoder::GetDevice()
{
  return m_Device;
}

void xiiGALCommandEncoder::AssertRenderingThread() const
{
  XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "This function may only be executed on the main thread.");
}

void xiiGALCommandEncoder::CountStateChange()
{
  ++m_uiStateChanges;
}

void xiiGALCommandEncoder::CountRedundantStateChange()
{
  ++m_uiRedundantStateChanges;
}
