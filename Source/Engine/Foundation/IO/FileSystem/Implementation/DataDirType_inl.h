#pragma once

#include <Foundation/Strings/StringBuilder.h>

inline xiiDataDirectoryReaderWriterBase::xiiDataDirectoryReaderWriterBase(xiiInt32 iDataDirUserData, bool bIsReader)
{
  m_iDataDirUserData = iDataDirUserData;
  m_pDataDirectory   = nullptr;
  m_bIsReader        = bIsReader;
}

inline xiiResult xiiDataDirectoryReaderWriterBase::Open(
  const char*            szResourcePath,
  xiiDataDirectoryType*  pDataDirectory,
  xiiFileShareMode::Enum FileShareMode)
{
  m_pDataDirectory = pDataDirectory;
  m_sFilePath      = szResourcePath;

  return InternalOpen(FileShareMode);
}

inline const xiiString128& xiiDataDirectoryReaderWriterBase::GetFilePath() const
{
  return m_sFilePath;
}

inline xiiDataDirectoryType* xiiDataDirectoryReaderWriterBase::GetDataDirectory() const
{
  return m_pDataDirectory;
}
