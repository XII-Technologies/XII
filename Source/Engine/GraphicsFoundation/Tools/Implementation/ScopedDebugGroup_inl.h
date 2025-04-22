
XII_ALWAYS_INLINE xiiGALScopedDebugGroup::xiiGALScopedDebugGroup() noexcept :
  m_pCommandList(nullptr)
{
}

XII_ALWAYS_INLINE xiiGALScopedDebugGroup::xiiGALScopedDebugGroup(xiiGALCommandList* pCommandList, xiiStringView sName, xiiColor color) :
  m_pCommandList(pCommandList)
{
  if (m_pCommandList)
  {
    m_pCommandList->BeginDebugGroup(sName, color);
  }
}

XII_ALWAYS_INLINE xiiGALScopedDebugGroup::~xiiGALScopedDebugGroup()
{
  if (m_pCommandList)
  {
    m_pCommandList->EndDebugGroup();
  }
}
