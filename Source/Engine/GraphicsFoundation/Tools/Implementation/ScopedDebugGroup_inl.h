
XII_ALWAYS_INLINE xiiGALScopedDebugGroup::xiiGALScopedDebugGroup() noexcept :
  m_pCommandList(nullptr)
{
}

XII_ALWAYS_INLINE xiiGALScopedDebugGroup::xiiGALScopedDebugGroup(xiiSharedPtr<xiiGALCommandList> pCommandList, xiiStringView sName, xiiColor color) :
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

XII_ALWAYS_INLINE xiiGALScopedDebugGroup::xiiGALScopedDebugGroup(xiiGALScopedDebugGroup&& rhs) noexcept :
  m_pCommandList(rhs.m_pCommandList)
{
  rhs.m_pCommandList = nullptr;
}

XII_ALWAYS_INLINE xiiGALScopedDebugGroup& xiiGALScopedDebugGroup::operator=(xiiGALScopedDebugGroup&& rhs) noexcept
{
  if (this != &rhs)
  {
    m_pCommandList = std::move(rhs.m_pCommandList);
  }
  return *this;
}
