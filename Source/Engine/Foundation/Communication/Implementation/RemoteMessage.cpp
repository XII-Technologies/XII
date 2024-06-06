#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/RemoteMessage.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcessMessage, 1, xiiRTTIDefaultAllocator<xiiProcessMessage>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRemoteMessage::xiiRemoteMessage() :
  m_Reader(&m_Storage), m_Writer(&m_Storage)
{
}

xiiRemoteMessage::xiiRemoteMessage(const xiiRemoteMessage& rhs) :
  m_Storage(rhs.m_Storage), m_Reader(&m_Storage), m_Writer(&m_Storage)
{
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID    = rhs.m_uiMsgID;
}


xiiRemoteMessage::xiiRemoteMessage(xiiUInt32 uiSystemID, xiiUInt32 uiMessageID) :
  m_Reader(&m_Storage), m_Writer(&m_Storage)
{
  m_uiSystemID = uiSystemID;
  m_uiMsgID    = uiMessageID;
}

void xiiRemoteMessage::operator=(const xiiRemoteMessage& rhs)
{
  m_Storage         = rhs.m_Storage;
  m_uiApplicationID = rhs.m_uiApplicationID;
  m_uiSystemID      = rhs.m_uiSystemID;
  m_uiMsgID         = rhs.m_uiMsgID;
  m_Reader.SetStorage(&m_Storage);
  m_Writer.SetStorage(&m_Storage);
}

xiiRemoteMessage::~xiiRemoteMessage()
{
  m_Reader.SetStorage(nullptr);
  m_Writer.SetStorage(nullptr);
}

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_RemoteMessage);
