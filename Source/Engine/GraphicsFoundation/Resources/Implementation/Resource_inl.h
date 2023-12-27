
XII_ALWAYS_INLINE xiiGALResourceBase::xiiGALResourceBase(xiiGALDevice* pDevice, xiiStringView sDebugName) :
  m_pDevice(pDevice)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (!sDebugName.IsEmpty())
  {
    m_sDebugName.Assign(sDebugName);
  }
#endif
}

XII_NODISCARD XII_ALWAYS_INLINE xiiGALResourceBase* xiiGALResourceBase::GetParentResource()
{
  return this;
}

XII_NODISCARD XII_ALWAYS_INLINE xiiGALDevice* xiiGALResourceBase::GetDevice() const
{
  return m_pDevice;
}

XII_NODISCARD XII_ALWAYS_INLINE xiiStringView xiiGALResourceBase::GetDebugName() const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  return m_sDebugName.GetView();
#else
  return {};
#endif
}

XII_NODISCARD XII_ALWAYS_INLINE void xiiGALResourceBase::SetDebugName(xiiStringView sDebugName)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(sDebugName);
#endif
}
