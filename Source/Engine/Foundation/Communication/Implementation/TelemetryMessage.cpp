/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/TelemetryMessage.h>

xiiTelemetryMessage::xiiTelemetryMessage() :
  m_Reader(&m_Storage), m_Writer(&m_Storage)
{
  m_uiSystemID = 0;
  m_uiMsgID    = 0;
}

xiiTelemetryMessage::xiiTelemetryMessage(const xiiTelemetryMessage& rhs) :
  m_Storage(rhs.m_Storage), m_Reader(&m_Storage), m_Writer(&m_Storage)
{
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID    = rhs.m_uiMsgID;
}

void xiiTelemetryMessage::operator=(const xiiTelemetryMessage& rhs)
{
  m_Storage    = rhs.m_Storage;
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID    = rhs.m_uiMsgID;
  m_Reader.SetStorage(&m_Storage);
  m_Writer.SetStorage(&m_Storage);
}

xiiTelemetryMessage::~xiiTelemetryMessage()
{
  m_Reader.SetStorage(nullptr);
  m_Writer.SetStorage(nullptr);
}

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_TelemetryMessage);
