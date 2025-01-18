#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/IpcChannelEnet.h>
#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/PipeChannel_win.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Platform/Implementation/Linux/PipeChannel_linux.h>
#endif

static_assert((xiiInt32)xiiIpcChannel::ConnectionState::Disconnected == (xiiInt32)xiiIpcChannelEvent::Disconnected);
static_assert((xiiInt32)xiiIpcChannel::ConnectionState::Connecting == (xiiInt32)xiiIpcChannelEvent::Connecting);
static_assert((xiiInt32)xiiIpcChannel::ConnectionState::Connected == (xiiInt32)xiiIpcChannelEvent::Connected);

xiiIpcChannel::xiiIpcChannel(xiiStringView sAddress, Mode::Enum mode) :
  m_sAddress(sAddress), m_Mode(mode), m_pOwner(xiiMessageLoop::GetSingleton())
{
}

xiiIpcChannel::~xiiIpcChannel()
{
  m_pOwner->RemoveChannel(this);
}

xiiInternal::NewInstance<xiiIpcChannel> xiiIpcChannel::CreatePipeChannel(xiiStringView sAddress, Mode::Enum mode)
{
  if (sAddress.IsEmpty() || sAddress.GetElementCount() > 200)
  {
    xiiLog::Error("Failed co create pipe '{0}', name is not valid", sAddress);
    return nullptr;
  }

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  return XII_DEFAULT_NEW(xiiPipeChannel_win, sAddress, mode);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  return XII_DEFAULT_NEW(xiiPipeChannel_linux, sAddress, mode);
#else
  XII_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}


xiiInternal::NewInstance<xiiIpcChannel> xiiIpcChannel::CreateNetworkChannel(xiiStringView sAddress, Mode::Enum mode)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  return XII_DEFAULT_NEW(xiiIpcChannelEnet, sAddress, mode);
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


bool xiiIpcChannel::Send(xiiArrayPtr<const xiiUInt8> data)
{
  {
    XII_LOCK(m_OutputQueueMutex);
    xiiMemoryStreamStorageInterface& storage = m_OutputQueue.ExpandAndGetRef();
    xiiMemoryStreamWriter            writer(&storage);
    xiiUInt32                        uiSize  = data.GetCount() + HEADER_SIZE;
    xiiUInt32                        uiMagic = MAGIC_VALUE;
    writer << uiMagic;
    writer << uiSize;
    XII_ASSERT_DEBUG(storage.GetStorageSize32() == HEADER_SIZE, "Magic value and size should have written HEADER_SIZE bytes.");
    writer.WriteBytes(data.GetPtr(), data.GetCount()).AssertSuccess("Failed to write to in-memory buffer, out of memory?");
  }
  if (IsConnected())
  {
    XII_LOCK(m_pOwner->m_TasksMutex);

    if (!m_pOwner->m_SendQueue.Contains(this))
      m_pOwner->m_SendQueue.PushBack(this);

    if (NeedWakeup())
    {
      m_pOwner->WakeUp();
    }
    return true;
  }
  return false;
}

void xiiIpcChannel::SetReceiveCallback(ReceiveCallback callback)
{
  XII_LOCK(m_ReceiveCallbackMutex);

  m_ReceiveCallback = callback;
}

xiiResult xiiIpcChannel::WaitForMessages(xiiTime timeout)
{
  if (IsConnected())
  {
    if (timeout == xiiTime::MakeZero())
    {
      m_IncomingMessages.WaitForSignal();
    }
    else if (m_IncomingMessages.WaitForSignal(timeout) == xiiThreadSignal::WaitResult::Timeout)
    {
      return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}

void xiiIpcChannel::SetConnectionState(xiiEnum<xiiIpcChannel::ConnectionState> state)
{
  const xiiEnum<xiiIpcChannel::ConnectionState> oldValue = m_iConnectionState.Set(state);

  if (state != oldValue)
  {
    m_Events.Broadcast(xiiIpcChannelEvent((xiiIpcChannelEvent::Type)state.GetValue(), this));
  }
}

void xiiIpcChannel::ReceiveData(xiiArrayPtr<const xiiUInt8> data)
{
  XII_LOCK(m_ReceiveCallbackMutex);

  if (!m_ReceiveCallback.IsValid())
  {
    m_MessageAccumulator.PushBackRange(data);
    return;
  }

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
      m_ReceiveCallback(xiiArrayPtr<const xiiUInt8>(m_MessageAccumulator.GetData() + HEADER_SIZE, uiMessageSize - HEADER_SIZE));
      m_IncomingMessages.RaiseSignal();
      m_Events.Broadcast(xiiIpcChannelEvent(xiiIpcChannelEvent::NewMessages, this));
      m_MessageAccumulator.Clear();
    }
  }
}

void xiiIpcChannel::FlushPendingOperations()
{
  m_pOwner->WaitForMessages(-1, this);
}

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_IpcChannel);
