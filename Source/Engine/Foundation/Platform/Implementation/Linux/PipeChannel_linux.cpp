/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Platform/Implementation/Linux/PipeChannel_linux.h>

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Implementation/Linux/MessageLoop_linux.h>

#  include <fcntl.h>
#  include <sys/socket.h>
#  include <sys/un.h>

xiiPipeChannel_linux::xiiPipeChannel_linux(xiiStringView sAddress, Mode::Enum Mode) :
  xiiIpcChannel(sAddress, Mode)
{
  xiiStringBuilder sPipePath = xiiOSFile::GetTempDataFolder("XII-Pipes");

  // Make sure the directory exists that we want to place the pipes in.
  xiiOSFile::CreateDirectoryStructure(sPipePath).IgnoreResult();

  sPipePath.AppendPath(sAddress);
  sPipePath.Append(".server");

  m_ServerSocketPath = sPipePath;
  sPipePath.Shrink(0, 7); // strip .server
  sPipePath.Append(".client");
  m_ClientSocketPath = sPipePath;

  m_pOwner->AddChannel(this);
}

xiiPipeChannel_linux::~xiiPipeChannel_linux()
{
  if (m_pOwner)
  {
    static_cast<xiiMessageLoop_linux*>(m_pOwner)->RemovePendingWaits(this);
  }

  if (m_ServerSocketFd >= 0)
  {
    close(m_ServerSocketFd);
    m_ServerSocketFd = -1;
  }

  if (m_ClientSocketFd >= 0)
  {
    close(m_ClientSocketFd);
    m_ClientSocketFd = -1;
  }

  if (m_Mode == Mode::Server)
  {
    xiiOSFile::DeleteFile(m_ServerSocketPath).IgnoreResult();
  }
  else
  {
    xiiOSFile::DeleteFile(m_ClientSocketPath).IgnoreResult();
  }
}

void xiiPipeChannel_linux::InternalConnect()
{
  if (GetConnectionState() != ConnectionState::Disconnected)
    return;

  int& iTargetSocket = (m_Mode == Mode::Server) ? m_ServerSocketFd : m_ClientSocketFd;

  if (iTargetSocket < 0)
  {
    const char* thisSocketPath = (m_Mode == Mode::Server) ? m_ServerSocketPath.GetData() : m_ClientSocketPath.GetData();

    iTargetSocket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (iTargetSocket == -1)
    {
      xiiLog::Error("[IPC]Failed to create unix domain socket. error {}", errno);
      return;
    }

    // If the socket file already exists, delete it
    xiiOSFile::DeleteFile(thisSocketPath).IgnoreResult();

    struct sockaddr_un addr = {};
    addr.sun_family         = AF_UNIX;

    if (strlen(thisSocketPath) >= XII_ARRAY_SIZE(addr.sun_path) - 1)
    {
      xiiLog::Error("[IPC]Given ipc channel address is to long. Resulting path '{}' path length limit {}", strlen(thisSocketPath), XII_ARRAY_SIZE(addr.sun_path) - 1);
      close(iTargetSocket);
      iTargetSocket = -1;
      return;
    }

    strcpy(addr.sun_path, thisSocketPath);
    if (bind(iTargetSocket, (struct sockaddr*)&addr, SUN_LEN(&addr)) == -1)
    {
      xiiLog::Error("[IPC]Failed to bind unix domain socket to '{}' error {}", thisSocketPath, errno);
      close(iTargetSocket);
      iTargetSocket = -1;
      return;
    }
  }

  if (m_Mode == Mode::Server)
  {
    if (m_ServerSocketFd < 0)
    {
      return;
    }
    listen(m_ServerSocketFd, 1);
    SetConnectionState(ConnectionState::Connecting);
    static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::Accept, m_ServerSocketFd);
  }
  else
  {
    if (m_ClientSocketFd < 0)
    {
      return;
    }
    SetConnectionState(ConnectionState::Connecting);
    struct sockaddr_un serverAddress = {};
    serverAddress.sun_family         = AF_UNIX;
    strcpy(serverAddress.sun_path, m_ServerSocketPath.GetData());

    xiiInt32 iConnectResult = connect(m_ClientSocketFd, (struct sockaddr*)&serverAddress, SUN_LEN(&serverAddress));
    XII_IGNORE_UNUSED(iConnectResult);

    static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::Connect, m_ClientSocketFd);
  }
}

void xiiPipeChannel_linux::InternalDisconnect()
{
  if (GetConnectionState() == ConnectionState::Disconnected)
    return;

  static_cast<xiiMessageLoop_linux*>(m_pOwner)->RemovePendingWaits(this);

  close(m_ClientSocketFd);
  m_ClientSocketFd = -1;

  {
    XII_LOCK(m_OutputQueueMutex);
    m_OutputQueue.Clear();
  }

  SetConnectionState(ConnectionState::Disconnected);

  m_IncomingMessages.RaiseSignal(); // Wakeup anyone still waiting for messages
}

void xiiPipeChannel_linux::InternalSend()
{
  const xiiMemoryStreamStorageInterface* pStorage = nullptr;
  {
    XII_LOCK(m_OutputQueueMutex);
    if (m_OutputQueue.IsEmpty())
    {
      return;
    }
    pStorage = &m_OutputQueue.PeekFront();
  }

  while (true)
  {

    xiiUInt64 uiToWrite    = pStorage->GetStorageSize64() - m_uiPreviousSendOffset;
    xiiUInt64 uiNextOffset = m_uiPreviousSendOffset;
    while (uiToWrite > 0)
    {
      const xiiArrayPtr<const xiiUInt8> range = pStorage->GetContiguousMemoryRange(uiNextOffset);

      xiiInt32 res = send(m_ClientSocketFd, range.GetPtr(), range.GetCount(), 0);

      if (res < 0)
      {
        xiiInt32 errorCode = errno;
        // We can't send at the moment. Wait until we can send again.
        if (errorCode == EWOULDBLOCK)
        {
          m_uiPreviousSendOffset = uiNextOffset;
          static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::Send, m_ClientSocketFd);
          return;
        }
        xiiLog::Error("[IPC]xiiPipeChannel_linux failed to send. Error {}", errorCode);
        InternalDisconnect();
        return;
      }

      uiToWrite -= static_cast<xiiUInt64>(res);
      uiNextOffset += res;
    }
    m_uiPreviousSendOffset = 0;

    {
      XII_LOCK(m_OutputQueueMutex);
      m_OutputQueue.PopFront();
      if (m_OutputQueue.IsEmpty())
      {
        return;
      }
      pStorage = &m_OutputQueue.PeekFront();
    }
  }
}

void xiiPipeChannel_linux::AcceptIncomingConnection()
{
  struct sockaddr_un incomingConnection = {};
  socklen_t          len                = sizeof(incomingConnection);
  m_ClientSocketFd                      = accept4(m_ServerSocketFd, (struct sockaddr*)&incomingConnection, &len, SOCK_NONBLOCK);
  if (m_ClientSocketFd == -1)
  {
    xiiLog::Error("[IPC]Failed to accept incoming connection. Error {}", errno);
    // Wait for the next incoming connection
    listen(m_ServerSocketFd, 1);
    static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::Accept, m_ServerSocketFd);
  }
  else
  {
    SetConnectionState(ConnectionState::Connected);
    // We are connected. Register for incoming messages events.
    static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::IncomingMessage, m_ClientSocketFd);
  }
}

bool xiiPipeChannel_linux::NeedWakeup() const
{
  return true;
}

void xiiPipeChannel_linux::ProcessConnectSuccessfull()
{
  SetConnectionState(ConnectionState::Connected);

  // We are connected. Register for incoming messages events.
  static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::IncomingMessage, m_ClientSocketFd);
}

void xiiPipeChannel_linux::ProcessIncomingPackages()
{
  while (true)
  {
    ssize_t recieveResult = recv(m_ClientSocketFd, m_InputBuffer, XII_ARRAY_SIZE(m_InputBuffer), 0);
    if (recieveResult == 0)
    {
      InternalDisconnect();
      return;
    }

    if (recieveResult < 0)
    {
      xiiInt32 errorCode = errno;
      if (errorCode == EWOULDBLOCK)
      {
        return;
      }
      if (errorCode != ECONNRESET)
      {
        xiiLog::Error("[IPC]xiiPipeChannel_linux recieve error {}", errorCode);
      }
      InternalDisconnect();
      return;
    }

    ReceiveData(xiiArrayPtr(m_InputBuffer, static_cast<xiiUInt32>(recieveResult)));
  }
}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Linux_PipeChannel_linux);
