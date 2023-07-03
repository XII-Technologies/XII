#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/DataTransfer.h>

bool                     xiiDataTransfer::s_bInitialized = false;
xiiSet<xiiDataTransfer*> xiiDataTransfer::s_AllTransfers;

xiiDataTransferObject::xiiDataTransferObject(xiiDataTransfer& ref_belongsTo, xiiStringView sObjectName, xiiStringView sMimeType, xiiStringView sFileExtension) :
  m_BelongsTo(ref_belongsTo)
{
  m_bHasBeenTransferred = false;

  m_Msg.SetMessageID('TRAN', 'DATA');
  m_Msg.GetWriter() << ref_belongsTo.m_sDataName;
  m_Msg.GetWriter() << sObjectName;
  m_Msg.GetWriter() << sMimeType;
  m_Msg.GetWriter() << sFileExtension;
}

xiiDataTransferObject::~xiiDataTransferObject()
{
  XII_ASSERT_DEV(m_bHasBeenTransferred, "The data transfer object has never been transmitted.");
}

void xiiDataTransferObject::Transmit()
{
  XII_ASSERT_DEV(!m_bHasBeenTransferred, "The data transfer object has been transmitted already.");

  if (m_bHasBeenTransferred)
    return;

  m_bHasBeenTransferred = true;

  m_BelongsTo.Transfer(*this);
}

xiiDataTransfer::xiiDataTransfer()
{
  m_bTransferRequested = false;
  m_bEnabled           = false;
}

xiiDataTransfer::~xiiDataTransfer()
{
  DisableDataTransfer();
}

void xiiDataTransfer::SendStatus()
{
  if (!xiiTelemetry::IsConnectedToClient())
    return;

  xiiTelemetryMessage msg;
  msg.GetWriter() << m_sDataName;

  if (m_bEnabled)
  {
    msg.SetMessageID('TRAN', 'ENBL');
  }
  else
  {
    msg.SetMessageID('TRAN', 'DSBL');
  }

  xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
}

void xiiDataTransfer::DisableDataTransfer()
{
  if (!m_bEnabled)
    return;

  xiiDataTransfer::s_AllTransfers.Remove(this);

  m_bEnabled = false;
  SendStatus();

  m_bTransferRequested = false;
  m_sDataName.Clear();
}

void xiiDataTransfer::EnableDataTransfer(xiiStringView sDataName)
{
  if (m_bEnabled && m_sDataName == sDataName)
    return;

  DisableDataTransfer();

  Initialize();

  xiiDataTransfer::s_AllTransfers.Insert(this);

  m_sDataName = sDataName;

  XII_ASSERT_DEV(!m_sDataName.IsEmpty(), "The name for the data transfer must not be empty.");

  m_bEnabled = true;
  SendStatus();
}

void xiiDataTransfer::RequestDataTransfer()
{
  if (!m_bEnabled)
  {
    m_bTransferRequested = false;
    return;
  }

  xiiLog::Dev("Data Transfer Request: {0}", m_sDataName);

  m_bTransferRequested = true;

  OnTransferRequest();
}

bool xiiDataTransfer::IsTransferRequested(bool bReset)
{
  const bool bRes = m_bTransferRequested;

  if (bReset)
    m_bTransferRequested = false;

  return bRes;
}

void xiiDataTransfer::Transfer(xiiDataTransferObject& Object)
{
  if (!m_bEnabled)
    return;

  xiiTelemetry::Broadcast(xiiTelemetry::Reliable, Object.m_Msg);
}

void xiiDataTransfer::Initialize()
{
  if (s_bInitialized)
    return;

  s_bInitialized = true;

  xiiTelemetry::AddEventHandler(TelemetryEventsHandler);
  xiiTelemetry::AcceptMessagesForSystem('DTRA', true, TelemetryMessage, nullptr);
}

void xiiDataTransfer::TelemetryMessage(void* pPassThrough)
{
  xiiTelemetryMessage Msg;

  while (xiiTelemetry::RetrieveMessage('DTRA', Msg) == XII_SUCCESS)
  {
    if (Msg.GetMessageID() == 'REQ')
    {
      xiiStringBuilder sName;
      Msg.GetReader() >> sName;

      xiiLog::Dev("Requested data transfer '{0}'", sName);

      for (auto it = s_AllTransfers.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Key()->m_sDataName == sName)
        {
          it.Key()->RequestDataTransfer();
          break;
        }
      }
    }
  }
}

void xiiDataTransfer::TelemetryEventsHandler(const xiiTelemetry::TelemetryEventData& e)
{
  if (!xiiTelemetry::IsConnectedToClient())
    return;

  switch (e.m_EventType)
  {
    case xiiTelemetry::TelemetryEventData::ConnectedToClient:
      SendAllDataTransfers();
      break;

    default:
      break;
  }
}

void xiiDataTransfer::SendAllDataTransfers()
{
  xiiTelemetryMessage msg;
  msg.SetMessageID('TRAN', ' CLR');
  xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);

  for (auto it = s_AllTransfers.GetIterator(); it.IsValid(); ++it)
  {
    it.Key()->SendStatus();
  }
}



XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_DataTransfer);
