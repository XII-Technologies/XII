
template <typename T>
XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Set(xiiHashedString sKey, const T& value)
{
  m_Entries.Insert(sKey, xiiVariant(value));
}

template <typename T>
XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Set(xiiHashedString sKey, T&& value)
{
  m_Entries.Insert(sKey, xiiVariant(std::forward<T>(value)));
}

template <typename T>
XII_ALWAYS_INLINE bool xiiRenderGraphBlackboard::TryGet(xiiHashedString sKey, T& out_value) const
{
  XII_LOCK(m_ReadMutex);

  xiiVariant value;
  if (!m_Entries.TryGetValue(sKey, value))
    return false;

  if (!value.IsA<T>())
    return false;

  out_value = value.Get<T>();
  return true;
}

template <typename T>
XII_ALWAYS_INLINE const T& xiiRenderGraphBlackboard::GetRef(xiiHashedString sKey) const
{
  XII_LOCK(m_ReadMutex);

  xiiVariant value;
  XII_VERIFY(m_Entries.TryGetValue(sKey, value), "Blackboard key '{}' does not exist.", sKey.GetView());
  XII_ASSERT_DEV(value.IsA<T>(), "Blackboard key '{}' exists but type does not match.", sKey.GetView());
  return value.Get<T>();
}

XII_ALWAYS_INLINE bool xiiRenderGraphBlackboard::Contains(xiiHashedString sKey) const
{
  XII_LOCK(m_ReadMutex);

  return m_Entries.Contains(sKey);
}

XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Remove(xiiHashedString sKey)
{
  m_Entries.Remove(sKey);
}

XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Clear()
{
  m_Entries.Clear();
}
