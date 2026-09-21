/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/Reflection.h>

/// \todo Add move semantics for xiiRemoteMessage

/// Encapsulates all the data that is transmitted when sending or receiving a message with xiiRemoteInterface
class XII_FOUNDATION_DLL xiiRemoteMessage
{
public:
  xiiRemoteMessage();
  xiiRemoteMessage(xiiUInt32 uiSystemID, xiiUInt32 uiMessageID);
  xiiRemoteMessage(const xiiRemoteMessage& rhs);
  ~xiiRemoteMessage();
  void operator=(const xiiRemoteMessage& rhs);

  /// \name Sending
  ///@{

  /// For setting the message IDs before sending it
  XII_ALWAYS_INLINE void SetMessageID(xiiUInt32 uiSystemID, xiiUInt32 uiMessageID)
  {
    m_uiSystemID = uiSystemID;
    m_uiMsgID    = uiMessageID;
  }

  /// Returns a stream writer to append data to the message
  XII_ALWAYS_INLINE xiiStreamWriter& GetWriter() { return m_Writer; }


  ///@}

  /// \name Receiving
  ///@{

  /// Returns a stream reader for reading the message data
  XII_ALWAYS_INLINE xiiStreamReader& GetReader() { return m_Reader; }
  XII_ALWAYS_INLINE xiiUInt32        GetApplicationID() const { return m_uiApplicationID; }
  XII_ALWAYS_INLINE xiiUInt32        GetSystemID() const { return m_uiSystemID; }
  XII_ALWAYS_INLINE xiiUInt32        GetMessageID() const { return m_uiMsgID; }
  XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt8> GetMessageData() const
  {
    return {m_Storage.GetData(), m_Storage.GetStorageSize32()};
  }

  ///@}

private:
  friend class xiiRemoteInterface;

  xiiUInt32 m_uiApplicationID = 0;
  xiiUInt32 m_uiSystemID      = 0;
  xiiUInt32 m_uiMsgID         = 0;

  xiiContiguousMemoryStreamStorage m_Storage;
  xiiMemoryStreamReader            m_Reader;
  xiiMemoryStreamWriter            m_Writer;
};

/// Base class for IPC messages transmitted by xiiIpcChannel.
class XII_FOUNDATION_DLL xiiProcessMessage : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcessMessage, xiiReflectedClass);

public:
  xiiProcessMessage() = default;
};
