#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/IpcProcessMessageProtocol.h>
// #include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
// #include <Foundation/Communication/RemoteMessage.h>
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

  xiiDeque<xiiUniquePtr<xiiProcessMessage>> messages;
  SwapWorkQueue(messages);
  messages.Clear();
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
  xiiDeque<xiiUniquePtr<xiiProcessMessage>> messages;
  SwapWorkQueue(messages);
  if (messages.IsEmpty())
  {
    return false;
  }

  while (!messages.IsEmpty())
  {
    xiiUniquePtr<xiiProcessMessage> msg = std::move(messages.PeekFront());
    messages.PopFront();
    m_MessageEvent.Broadcast(msg.Borrow());
  }

  return true;
}

xiiResult xiiIpcProcessMessageProtocol::WaitForMessages(xiiTime timeout)
{
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

void xiiIpcProcessMessageProtocol::SwapWorkQueue(xiiDeque<xiiUniquePtr<xiiProcessMessage>>& messages)
{
  XII_ASSERT_DEBUG(messages.IsEmpty(), "Swap target must be empty!");
  XII_LOCK(m_IncomingQueueMutex);

  if (m_IncomingQueue.IsEmpty())
    return;

  messages.Swap(m_IncomingQueue);
}

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_IpcProcessMessageProtocol);
