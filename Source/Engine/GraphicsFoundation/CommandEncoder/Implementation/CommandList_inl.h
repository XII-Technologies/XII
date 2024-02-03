
XII_ALWAYS_INLINE void xiiGALCommandList::AssertRenderingThread() const
{
  XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "This function may only be executed on the main thread.");
}

XII_ALWAYS_INLINE void xiiGALCommandList::CountDispatchCall()
{
  ++m_uiDispatchCalls;
}

XII_ALWAYS_INLINE void xiiGALCommandList::CountDrawCall()
{
  ++m_uiDrawCalls;
}
