
[[nodiscard]] XII_ALWAYS_INLINE xiiStringView xiiGALObject::GetDebugName() const
{
  return m_sDebugName.GetView();
}

[[nodiscard]] XII_ALWAYS_INLINE void xiiGALObject::SetDebugName(xiiStringView sDebugName)
{
  m_sDebugName.Assign(sDebugName);

  SetDebugNamePlatform(m_sDebugName.GetView());
}
