/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename T>
XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Set(xiiStringView sKey, const T& value)
{
  m_Entries.Insert(xiiMakeHashedString(sKey), xiiVariant(value));
}

template <typename T>
XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Set(xiiStringView sKey, T&& value)
{
  m_Entries.Insert(xiiMakeHashedString(sKey), xiiVariant(std::forward<T>(value)));
}

template <typename T>
XII_ALWAYS_INLINE bool xiiRenderGraphBlackboard::TryGet(xiiStringView sKey, T& out_value) const
{
  XII_LOCK(m_ReadMutex);

  xiiVariant value;
  if (!m_Entries.TryGetValue(xiiTempHashedString(sKey), value))
    return false;

  if (!value.IsA<T>())
    return false;

  out_value = value.Get<T>();
  return true;
}

template <typename T>
XII_ALWAYS_INLINE const T& xiiRenderGraphBlackboard::GetRef(xiiStringView sKey) const
{
  XII_LOCK(m_ReadMutex);

  xiiVariant value;
  XII_VERIFY(m_Entries.TryGetValue(xiiTempHashedString(sKey), value), "Blackboard key '{}' does not exist.", sKey);
  XII_ASSERT_DEV(value.IsA<T>(), "Blackboard key '{}' exists but type does not match.", sKey);
  return value.Get<T>();
}

XII_ALWAYS_INLINE bool xiiRenderGraphBlackboard::Contains(xiiStringView sKey) const
{
  XII_LOCK(m_ReadMutex);

  return m_Entries.Contains(xiiTempHashedString(sKey));
}

XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Remove(xiiStringView sKey)
{
  m_Entries.Remove(xiiTempHashedString(sKey));
}

XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Clear()
{
  m_Entries.Clear();
}
