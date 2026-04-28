/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/MemoryStream.h>

class XII_FOUNDATION_DLL xiiTelemetryMessage
{
public:
  xiiTelemetryMessage();
  xiiTelemetryMessage(const xiiTelemetryMessage& rhs);
  ~xiiTelemetryMessage();

  void operator=(const xiiTelemetryMessage& rhs);

  XII_ALWAYS_INLINE xiiStreamReader& GetReader() { return m_Reader; }
  XII_ALWAYS_INLINE xiiStreamWriter& GetWriter() { return m_Writer; }

  XII_ALWAYS_INLINE xiiUInt32 GetSystemID() const { return m_uiSystemID; }
  XII_ALWAYS_INLINE xiiUInt32 GetMessageID() const { return m_uiMsgID; }

  XII_ALWAYS_INLINE void SetMessageID(xiiUInt32 uiSystemID, xiiUInt32 uiMessageID)
  {
    m_uiSystemID = uiSystemID;
    m_uiMsgID    = uiMessageID;
  }

  // xiiUInt64 GetMessageSize() const { return m_Storage.GetStorageSize64(); }

private:
  friend class xiiTelemetry;

  xiiUInt32 m_uiSystemID;
  xiiUInt32 m_uiMsgID;

  xiiContiguousMemoryStreamStorage m_Storage;
  xiiMemoryStreamReader            m_Reader;
  xiiMemoryStreamWriter            m_Writer;
};
