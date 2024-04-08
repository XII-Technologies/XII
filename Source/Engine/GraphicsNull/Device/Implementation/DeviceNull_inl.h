
XII_ALWAYS_INLINE xiiGALCommandQueue* xiiGALDeviceNull::GetGraphicsQueue() const
{
  return m_pDefaultQueue.Borrow();
}

XII_ALWAYS_INLINE xiiGALCommandQueue* xiiGALDeviceNull::GetComputeQueue() const
{
  return nullptr;
}

XII_ALWAYS_INLINE xiiGALCommandQueue* xiiGALDeviceNull::GetTransferQueue() const
{
  return nullptr;
}

XII_ALWAYS_INLINE xiiGALCommandQueue* xiiGALDeviceNull::GetSparseBindingQueue() const
{
  return nullptr;
}
