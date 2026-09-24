/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename T>
XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Set(xiiStringView sKey, const T& value)
{
  m_FrameEntries.Insert(xiiMakeHashedString(sKey), xiiVariant(value));
}

template <typename T>
XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Set(xiiStringView sKey, T&& value)
{
  m_FrameEntries.Insert(xiiMakeHashedString(sKey), xiiVariant(std::forward<T>(value)));
}

template <typename T>
XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::SetGraph(xiiStringView sKey, const T& value)
{
  m_GraphEntries.Insert(xiiMakeHashedString(sKey), xiiVariant(value));
}

template <typename T>
XII_ALWAYS_INLINE bool xiiRenderGraphBlackboard::TryGet(xiiStringView sKey, T& out_value) const
{
  XII_LOCK(m_ReadMutex);

  xiiVariant value;
  if (!m_FrameEntries.TryGetValue(xiiTempHashedString(sKey), value))
    return false;

  if (!value.IsA<T>())
    return false;

  out_value = value.Get<T>();
  return true;
}

template <typename T>
XII_ALWAYS_INLINE bool xiiRenderGraphBlackboard::TryGetScoped(xiiStringView sKey, T& out_value) const
{
  XII_LOCK(m_ReadMutex);

  const xiiVariant* pValue = m_FrameEntries.GetValue(xiiTempHashedString(sKey));
  if (pValue == nullptr)
    pValue = m_GraphEntries.GetValue(xiiTempHashedString(sKey));
  if (pValue == nullptr || !pValue->IsA<T>())
    return false;

  out_value = pValue->Get<T>();
  return true;
}

template <typename T>
XII_ALWAYS_INLINE const T& xiiRenderGraphBlackboard::GetRef(xiiStringView sKey) const
{
  XII_LOCK(m_ReadMutex);

  const xiiVariant* pValue = m_FrameEntries.GetValue(xiiTempHashedString(sKey));
  if (pValue == nullptr)
    pValue = m_GraphEntries.GetValue(xiiTempHashedString(sKey));
  XII_ASSERT_DEV(pValue != nullptr, "Blackboard key '{}' does not exist.", sKey);
  XII_ASSERT_DEV(pValue->IsA<T>(), "Blackboard key '{}' exists but type does not match.", sKey);
  return pValue->Get<T>();
}

XII_ALWAYS_INLINE bool xiiRenderGraphBlackboard::Contains(xiiStringView sKey) const
{
  XII_LOCK(m_ReadMutex);

  return m_FrameEntries.Contains(xiiTempHashedString(sKey)) || m_GraphEntries.Contains(xiiTempHashedString(sKey));
}

XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Remove(xiiStringView sKey)
{
  m_FrameEntries.Remove(xiiTempHashedString(sKey));
  m_GraphEntries.Remove(xiiTempHashedString(sKey));
}

XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::Clear()
{
  m_FrameEntries.Clear();
  m_GraphEntries.Clear();
}

XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::ClearFrame()
{
  m_FrameEntries.Clear();
}

XII_ALWAYS_INLINE void xiiRenderGraphBlackboard::ClearGraph()
{
  m_GraphEntries.Clear();
}
