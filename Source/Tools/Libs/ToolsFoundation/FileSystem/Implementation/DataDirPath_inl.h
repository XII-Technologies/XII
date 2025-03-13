

inline xiiDataDirPath::xiiDataDirPath() = default;

inline xiiDataDirPath::xiiDataDirPath(xiiStringView sAbsPath, xiiArrayPtr<xiiString> dataDirRoots, xiiUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  XII_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  m_sAbsolutePath = sAbsPath;
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline xiiDataDirPath::xiiDataDirPath(const xiiStringBuilder& sAbsPath, xiiArrayPtr<xiiString> dataDirRoots, xiiUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  XII_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  m_sAbsolutePath = sAbsPath;
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline xiiDataDirPath::xiiDataDirPath(xiiString&& sAbsPath, xiiArrayPtr<xiiString> dataDirRoots, xiiUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  XII_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  m_sAbsolutePath = std::move(sAbsPath);
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline xiiDataDirPath::operator xiiStringView() const
{
  return m_sAbsolutePath;
}

inline bool xiiDataDirPath::operator==(xiiStringView rhs) const
{
  return m_sAbsolutePath == rhs;
}

inline bool xiiDataDirPath::operator!=(xiiStringView rhs) const
{
  return m_sAbsolutePath != rhs;
}

inline bool xiiDataDirPath::IsValid() const
{
  return m_uiDataDirParent != 0;
}

inline void xiiDataDirPath::Clear()
{
  m_sAbsolutePath.Clear();
  m_uiDataDirParent = 0;
  m_uiDataDirLength = 0;
  m_uiDataDirIndex  = 0;
}

inline const xiiString& xiiDataDirPath::GetAbsolutePath() const
{
  return m_sAbsolutePath;
}

inline xiiStringView xiiDataDirPath::GetDataDirParentRelativePath() const
{
  XII_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  const xiiUInt32 uiOffset = m_uiDataDirParent + 1;
  return xiiStringView(m_sAbsolutePath.GetData() + uiOffset, m_sAbsolutePath.GetElementCount() - uiOffset);
}

inline xiiStringView xiiDataDirPath::GetDataDirRelativePath() const
{
  XII_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  const xiiUInt32 uiOffset = xiiMath::Min(m_sAbsolutePath.GetElementCount(), m_uiDataDirParent + m_uiDataDirLength + 1u);
  return xiiStringView(m_sAbsolutePath.GetData() + uiOffset, m_sAbsolutePath.GetElementCount() - uiOffset);
}

inline xiiStringView xiiDataDirPath::GetDataDir() const
{
  XII_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  return xiiStringView(m_sAbsolutePath.GetData(), m_uiDataDirParent + m_uiDataDirLength);
}

inline xiiUInt8 xiiDataDirPath::GetDataDirIndex() const
{
  XII_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  return m_uiDataDirIndex;
}

inline xiiStreamWriter& xiiDataDirPath::Write(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_sAbsolutePath;
  inout_stream << m_uiDataDirParent;
  inout_stream << m_uiDataDirLength;
  inout_stream << m_uiDataDirIndex;
  return inout_stream;
}

inline xiiStreamReader& xiiDataDirPath::Read(xiiStreamReader& inout_stream)
{
  inout_stream >> m_sAbsolutePath;
  inout_stream >> m_uiDataDirParent;
  inout_stream >> m_uiDataDirLength;
  inout_stream >> m_uiDataDirIndex;
  return inout_stream;
}

bool xiiCompareDataDirPath::Less(xiiStringView lhs, xiiStringView rhs)
{
  xiiInt32 res = lhs.Compare_NoCase(rhs);
  if (res == 0)
  {
    return lhs.Compare(rhs) < 0;
  }

  return res < 0;
}

bool xiiCompareDataDirPath::Equal(xiiStringView lhs, xiiStringView rhs)
{
  return lhs.IsEqual(rhs);
}

inline xiiStreamWriter& operator<<(xiiStreamWriter& inout_stream, const xiiDataDirPath& value)
{
  return value.Write(inout_stream);
}

inline xiiStreamReader& operator>>(xiiStreamReader& inout_stream, xiiDataDirPath& out_value)
{
  return out_value.Read(inout_stream);
}
