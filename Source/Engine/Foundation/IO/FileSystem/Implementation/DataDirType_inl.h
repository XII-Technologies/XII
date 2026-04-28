/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/StringBuilder.h>

inline xiiDataDirectoryReaderWriterBase::xiiDataDirectoryReaderWriterBase(xiiInt32 iDataDirUserData, bool bIsReader)
{
  m_iDataDirUserData = iDataDirUserData;
  m_pDataDirType     = nullptr;
  m_bIsReader        = bIsReader;
}

inline xiiResult xiiDataDirectoryReaderWriterBase::Open(xiiStringView sFile, xiiDataDirectoryType* pDataDirectory, xiiFileShareMode::Enum fileShareMode)
{
  m_pDataDirType = pDataDirectory;
  m_sFilePath    = sFile;

  return InternalOpen(fileShareMode);
}

inline const xiiString128& xiiDataDirectoryReaderWriterBase::GetFilePath() const
{
  return m_sFilePath;
}

inline xiiDataDirectoryType* xiiDataDirectoryReaderWriterBase::GetDataDirectory() const
{
  return m_pDataDirType;
}
