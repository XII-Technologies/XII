#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

xiiIpcProcessMessageProtocol::xiiIpcProcessMessageProtocol(xiiIpcChannel* pChannel)
{
  m_pChannel = pChannel;
  m_pChannel->SetReceiveCallback(xiiMakeDelegate(&xiiIpcProcessMessageProtocol::ReceiveMessageData, this));
}

xiiIpcProcessMessageProtocol::~xiiIpcProcessMessageProtocol()
{
  m_pChannel->SetReceiveCallback({});

  while (xiiUniquePtr<xiiProcessMessage> msg = PopMessage())
  {
  }
}

bool xiiIpcProcessMessageProtocol::Send(xiiProcessMessage* pMsg)
{
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  xiiReflectionSerializer::WriteObjectToBinary(writer, pMsg->GetDynamicRTTI(), pMsg);
  return m_pChannel->Send(xiiArrayPtr<const xiiUInt8>(storage.GetData(), storage.GetStorageSize32()));
}

bool xiiIpcProcessMessageProtocol::ProcessMessages()
{
  bool bMessagesPresent = false;

  while (xiiUniquePtr<xiiProcessMessage> msg = PopMessage())
  {
    bMessagesPresent = true;
    Event e;
    e.m_pMessage                    = msg.Borrow();
    e.m_bInterruptMessageProcessing = false;
    m_MessageEvent.Broadcast(e);

    if (e.m_bInterruptMessageProcessing)
      break;
  }

  return bMessagesPresent;
}

xiiResult xiiIpcProcessMessageProtocol::WaitForMessages(xiiTime timeout)
{
  // Message processing can be interrupted via the m_bInterruptMessageProcessing flag. Thus, there is no guarantee that the queue is empty at this point. Only wait if the queue is empty.
  if (ProcessMessages())
    return XII_SUCCESS;

  xiiResult res = m_pChannel->WaitForMessages(timeout);
  if (res.Succeeded())
  {
    ProcessMessages();
  }
  return res;
}

void xiiIpcProcessMessageProtocol::ReceiveMessageData(xiiArrayPtr<const xiiUInt8> data)
{
  // Message complete, de-serialize
  xiiRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  const xiiRTTI*           pRtti = nullptr;

  xiiProcessMessage*              pMsg = (xiiProcessMessage*)xiiReflectionSerializer::ReadObjectFromBinary(reader, pRtti);
  xiiUniquePtr<xiiProcessMessage> msg(pMsg, xiiFoundation::GetDefaultAllocator());
  if (msg != nullptr)
  {
    EnqueueMessage(std::move(msg));
  }
  else
  {
    xiiLog::Error("Channel received invalid Message!");
  }
}

void xiiIpcProcessMessageProtocol::EnqueueMessage(xiiUniquePtr<xiiProcessMessage>&& msg)
{
  XII_LOCK(m_IncomingQueueMutex);

  m_IncomingQueue.PushBack(std::move(msg));
}

xiiUniquePtr<xiiProcessMessage> xiiIpcProcessMessageProtocol::PopMessage()
{
  XII_LOCK(m_IncomingQueueMutex);

  if (m_IncomingQueue.IsEmpty())
    return {};

  xiiUniquePtr<xiiProcessMessage> pFront = std::move(m_IncomingQueue.PeekFront());
  m_IncomingQueue.PopFront();
  return pFront;
}

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_IpcProcessMessageProtocol);
