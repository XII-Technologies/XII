#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/IpcChannelEnet.h>
#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Communication/Implementation/Win/PipeChannel_win.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Communication/Implementation/Linux/PipeChannel_linux.h>
#endif

xiiIpcChannel::xiiIpcChannel(const char* szAddress, Mode::Enum mode) :
  m_Mode(mode), m_pOwner(xiiMessageLoop::GetSingleton())
{
}

xiiIpcChannel::~xiiIpcChannel()
{
  xiiDeque<xiiUniquePtr<xiiProcessMessage>> messages;
  SwapWorkQueue(messages);
  messages.Clear();

  m_pOwner->RemoveChannel(this);
}

xiiIpcChannel* xiiIpcChannel::CreatePipeChannel(const char* szAddress, Mode::Enum mode)
{
  if (xiiStringUtils::IsNullOrEmpty(szAddress) || xiiStringUtils::GetStringElementCount(szAddress) > 200)
  {
    xiiLog::Error("Failed co create pipe '{0}', name is not valid", szAddress);
    return nullptr;
  }

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  return XII_DEFAULT_NEW(xiiPipeChannel_win, szAddress, mode);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  return XII_DEFAULT_NEW(xiiPipeChannel_linux, szAddress, mode);
#else
  XII_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}


xiiIpcChannel* xiiIpcChannel::CreateNetworkChannel(const char* szAddress, Mode::Enum mode)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  return XII_DEFAULT_NEW(xiiIpcChannelEnet, szAddress, mode);
#else
  XII_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}

void xiiIpcChannel::Connect()
{
  XII_LOCK(m_pOwner->m_TasksMutex);
  m_pOwner->m_ConnectQueue.PushBack(this);
  m_pOwner->WakeUp();
}


void xiiIpcChannel::Disconnect()
{
  XII_LOCK(m_pOwner->m_TasksMutex);
  m_pOwner->m_DisconnectQueue.PushBack(this);
  m_pOwner->WakeUp();
}

bool xiiIpcChannel::Send(xiiProcessMessage* pMsg)
{
  {
    XII_LOCK(m_OutputQueueMutex);
    xiiMemoryStreamStorageInterface& storage = m_OutputQueue.ExpandAndGetRef();
    xiiMemoryStreamWriter            writer(&storage);
    xiiUInt32                        uiSize  = 0;
    xiiUInt32                        uiMagic = MAGIC_VALUE;
    writer << uiMagic;
    writer << uiSize;
    XII_ASSERT_DEBUG(storage.GetStorageSize32() == HEADER_SIZE, "Magic value and size should have written HEADER_SIZE bytes.");
    xiiReflectionSerializer::WriteObjectToBinary(writer, pMsg->GetDynamicRTTI(), pMsg);

    // reset to the beginning and write the stored size again
    writer.SetWritePosition(4);
    writer << storage.GetStorageSize32();
  }
  if (m_bConnected)
  {
    if (NeedWakeup())
    {
      XII_LOCK(m_pOwner->m_TasksMutex);
      if (!m_pOwner->m_SendQueue.Contains(this))
        m_pOwner->m_SendQueue.PushBack(this);
      m_pOwner->WakeUp();
      return true;
    }
  }
  return false;
}

bool xiiIpcChannel::ProcessMessages()
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

void xiiIpcChannel::WaitForMessages()
{
  if (m_bConnected)
  {
    m_IncomingMessages.WaitForSignal();
    ProcessMessages();
  }
}

xiiResult xiiIpcChannel::WaitForMessages(xiiTime timeout)
{
  if (m_bConnected)
  {
    if (m_IncomingMessages.WaitForSignal(timeout) == xiiThreadSignal::WaitResult::Timeout)
    {
      return XII_FAILURE;
    }
    ProcessMessages();
  }

  return XII_SUCCESS;
}

void xiiIpcChannel::ReceiveMessageData(xiiArrayPtr<const xiiUInt8> data)
{
  xiiArrayPtr<const xiiUInt8> remainingData = data;
  while (true)
  {
    if (m_MessageAccumulator.GetCount() < HEADER_SIZE)
    {
      if (remainingData.GetCount() + m_MessageAccumulator.GetCount() < HEADER_SIZE)
      {
        m_MessageAccumulator.PushBackRange(remainingData);
        return;
      }
      else
      {
        xiiUInt32                   uiRemainingHeaderData = HEADER_SIZE - m_MessageAccumulator.GetCount();
        xiiArrayPtr<const xiiUInt8> headerData            = remainingData.GetSubArray(0, uiRemainingHeaderData);
        m_MessageAccumulator.PushBackRange(headerData);
        XII_ASSERT_DEBUG(m_MessageAccumulator.GetCount() == HEADER_SIZE, "We should have a full header now.");
        remainingData = remainingData.GetSubArray(uiRemainingHeaderData);
      }
    }

    XII_ASSERT_DEBUG(m_MessageAccumulator.GetCount() >= HEADER_SIZE, "Header must be complete at this point.");
    if (remainingData.IsEmpty())
      return;

    // Read and verify header
    xiiUInt32 uiMagic = *reinterpret_cast<const xiiUInt32*>(m_MessageAccumulator.GetData());
    XII_IGNORE_UNUSED(uiMagic);
    XII_ASSERT_DEBUG(uiMagic == MAGIC_VALUE, "Message received with wrong magic value.");
    xiiUInt32 uiMessageSize = *reinterpret_cast<const xiiUInt32*>(m_MessageAccumulator.GetData() + 4);
    XII_ASSERT_DEBUG(uiMessageSize < MAX_MESSAGE_SIZE, "Message too big: {0}! Either the stream got corrupted or you need to increase MAX_MESSAGE_SIZE.", uiMessageSize);
    if (uiMessageSize > remainingData.GetCount() + m_MessageAccumulator.GetCount())
    {
      m_MessageAccumulator.PushBackRange(remainingData);
      return;
    }

    // Write missing data into message accumulator
    xiiUInt32                   remainingMessageData = uiMessageSize - m_MessageAccumulator.GetCount();
    xiiArrayPtr<const xiiUInt8> messageData          = remainingData.GetSubArray(0, remainingMessageData);
    m_MessageAccumulator.PushBackRange(messageData);
    XII_ASSERT_DEBUG(m_MessageAccumulator.GetCount() == uiMessageSize, "");
    remainingData = remainingData.GetSubArray(remainingMessageData);

    {
      // Message complete, de-serialize
      xiiRawMemoryStreamReader reader(m_MessageAccumulator.GetData() + HEADER_SIZE, uiMessageSize - HEADER_SIZE);
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
      m_MessageAccumulator.Clear();
    }
  }
}

void xiiIpcChannel::EnqueueMessage(xiiUniquePtr<xiiProcessMessage>&& msg)
{
  {
    XII_LOCK(m_IncomingQueueMutex);
    m_IncomingQueue.PushBack(std::move(msg));
  }
  m_IncomingMessages.RaiseSignal();

  m_Events.Broadcast(xiiIpcChannelEvent(xiiIpcChannelEvent::NewMessages, this));
}

void xiiIpcChannel::SwapWorkQueue(xiiDeque<xiiUniquePtr<xiiProcessMessage>>& messages)
{
  XII_ASSERT_DEBUG(messages.IsEmpty(), "Swap target must be empty!");
  XII_LOCK(m_IncomingQueueMutex);
  if (m_IncomingQueue.IsEmpty())
    return;
  messages.Swap(m_IncomingQueue);
}

void xiiIpcChannel::FlushPendingOperations()
{
  m_pOwner->WaitForMessages(-1, this);
}



XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_IpcChannel);
