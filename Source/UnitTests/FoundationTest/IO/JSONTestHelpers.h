/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/OSFile.h>

class StreamComparer : public xiiStreamWriter
{
public:
  StreamComparer(const char* szExpectedData, bool bOnlyWriteResult = false)
  {
    m_bOnlyWriteResult = bOnlyWriteResult;
    m_szExpectedData   = szExpectedData;
  }

  ~StreamComparer()
  {
    if (m_bOnlyWriteResult)
    {
      xiiOSFile f;
      f.Open("C:\\Code\\JSON.txt", xiiFileOpenMode::Write).IgnoreResult();
      f.Write(m_sResult.GetData(), m_sResult.GetElementCount()).IgnoreResult();
      f.Close();
    }
    else
      XII_TEST_BOOL(*m_szExpectedData == '\0');
  }

  xiiResult WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite)
  {
    if (m_bOnlyWriteResult)
      m_sResult.Append((const char*)pWriteBuffer);
    else
    {
      const char* szWritten = (const char*)pWriteBuffer;

      XII_TEST_BOOL(xiiMemoryUtils::IsEqual(szWritten, m_szExpectedData, (xiiUInt32)uiBytesToWrite));
      m_szExpectedData += uiBytesToWrite;
    }

    return XII_SUCCESS;
  }

private:
  bool             m_bOnlyWriteResult;
  xiiStringBuilder m_sResult;
  const char*      m_szExpectedData;
};


class StringStream : public xiiStreamReader
{
public:
  StringStream(const void* pData)
  {
    m_pData    = pData;
    m_uiLength = xiiStringUtils::GetStringElementCount((const char*)pData);
  }

  virtual xiiUInt64 ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead)
  {
    uiBytesToRead = xiiMath::Min(uiBytesToRead, m_uiLength);
    m_uiLength -= uiBytesToRead;

    if (uiBytesToRead > 0)
    {
      xiiMemoryUtils::Copy((xiiUInt8*)pReadBuffer, (xiiUInt8*)m_pData, (size_t)uiBytesToRead);
      m_pData = xiiMemoryUtils::AddByteOffset(m_pData, (ptrdiff_t)uiBytesToRead);
    }

    return uiBytesToRead;
  }

private:
  const void* m_pData;
  xiiUInt64   m_uiLength;
};
