
XII_ALWAYS_INLINE xiiGALCommandQueue* xiiGALDeviceNull::GetDefaultCommandQueue(xiiBitflags<xiiGALCommandQueueType> queueType, bool bAllowGraphicsCommandQueueFallback) const
{
  return m_pDefaultQueue.Borrow();
}
