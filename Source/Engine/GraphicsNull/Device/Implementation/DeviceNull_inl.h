
XII_ALWAYS_INLINE xiiGALCommandQueue* xiiGALDeviceNull::GetDefaultCommandQueue(xiiBitflags<xiiGALCommandQueueType> queueType, bool bAllowGraphicsCommandQueueFallback) const
{
  XII_IGNORE_UNUSED(queueType);
  XII_IGNORE_UNUSED(bAllowGraphicsCommandQueueFallback);
  return m_pDefaultQueue.Borrow();
}
