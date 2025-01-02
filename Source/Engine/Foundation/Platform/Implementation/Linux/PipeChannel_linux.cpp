
#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Platform/Implementation/Linux/PipeChannel_linux.h>

#  include <Foundation/Platform/Implementation/Linux/MessageLoop_linux.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>

#  include <fcntl.h>
#  include <sys/socket.h>
#  include <sys/un.h>

xiiPipeChannel_linux::xiiPipeChannel_linux(xiiStringView sAddress, Mode::Enum Mode) :
  xiiIpcChannel(sAddress, Mode)
{
  xiiStringBuilder pipePath = xiiOSFile::GetTempDataFolder("XII-Pipes");

  // Make sure the directory exists that we want to place the pipes in.
  xiiOSFile::CreateDirectoryStructure(pipePath).IgnoreResult();

  pipePath.AppendPath(sAddress);
  pipePath.Append(".server");

  m_serverSocketPath = pipePath;
  pipePath.Shrink(0, 7); // strip .server
  pipePath.Append(".client");
  m_clientSocketPath = pipePath;

  m_pOwner->AddChannel(this);
}

xiiPipeChannel_linux::~xiiPipeChannel_linux()
{
  if (m_pOwner)
  {
    static_cast<xiiMessageLoop_linux*>(m_pOwner)->RemovePendingWaits(this);
  }

  if (m_serverSocketFd >= 0)
  {
    close(m_serverSocketFd);
    m_serverSocketFd = -1;
  }

  if (m_clientSocketFd >= 0)
  {
    close(m_clientSocketFd);
    m_clientSocketFd = -1;
  }

  if (m_Mode == Mode::Server)
  {
    xiiOSFile::DeleteFile(m_serverSocketPath).IgnoreResult();
  }
  else
  {
    xiiOSFile::DeleteFile(m_clientSocketPath).IgnoreResult();
  }
}

void xiiPipeChannel_linux::InternalConnect()
{
  if (GetConnectionState() != ConnectionState::Disconnected)
    return;

  int& targetSocket = (m_Mode == Mode::Server) ? m_serverSocketFd : m_clientSocketFd;

  if (targetSocket < 0)
  {
    const char* thisSocketPath = (m_Mode == Mode::Server) ? m_serverSocketPath.GetData() : m_clientSocketPath.GetData();

    targetSocket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (targetSocket == -1)
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
      close(targetSocket);
      targetSocket = -1;
      return;
    }

    strcpy(addr.sun_path, thisSocketPath);
    if (bind(targetSocket, (struct sockaddr*)&addr, SUN_LEN(&addr)) == -1)
    {
      xiiLog::Error("[IPC]Failed to bind unix domain socket to '{}' error {}", thisSocketPath, errno);
      close(targetSocket);
      targetSocket = -1;
      return;
    }
  }

  if (m_Mode == Mode::Server)
  {
    if (m_serverSocketFd < 0)
    {
      return;
    }
    listen(m_serverSocketFd, 1);
    SetConnectionState(ConnectionState::Connecting);
    static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::Accept, m_serverSocketFd);
  }
  else
  {
    if (m_clientSocketFd < 0)
    {
      return;
    }
    SetConnectionState(ConnectionState::Connecting);
    struct sockaddr_un serverAddress = {};
    serverAddress.sun_family         = AF_UNIX;
    strcpy(serverAddress.sun_path, m_serverSocketPath.GetData());

    xiiInt32 iConnectResult = connect(m_clientSocketFd, (struct sockaddr*)&serverAddress, SUN_LEN(&serverAddress));
    XII_IGNORE_UNUSED(iConnectResult);

    static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::Connect, m_clientSocketFd);
  }
}

void xiiPipeChannel_linux::InternalDisconnect()
{
  if (GetConnectionState() == ConnectionState::Disconnected)
    return;

  static_cast<xiiMessageLoop_linux*>(m_pOwner)->RemovePendingWaits(this);

  close(m_clientSocketFd);
  m_clientSocketFd = -1;

  {
    XII_LOCK(m_OutputQueueMutex);
    m_OutputQueue.Clear();
  }

  SetConnectionState(ConnectionState::Disconnected);

  m_IncomingMessages.RaiseSignal(); // Wakeup anyone still waiting for messages
}

void xiiPipeChannel_linux::InternalSend()
{
  const xiiMemoryStreamStorageInterface* storage = nullptr;
  {
    XII_LOCK(m_OutputQueueMutex);
    if (m_OutputQueue.IsEmpty())
    {
      return;
    }
    storage = &m_OutputQueue.PeekFront();
  }

  while (true)
  {

    xiiUInt64 uiToWrite    = storage->GetStorageSize64() - m_previousSendOffset;
    xiiUInt64 uiNextOffset = m_previousSendOffset;
    while (uiToWrite > 0)
    {
      const xiiArrayPtr<const xiiUInt8> range = storage->GetContiguousMemoryRange(uiNextOffset);

      xiiInt32 res = send(m_clientSocketFd, range.GetPtr(), range.GetCount(), 0);

      if (res < 0)
      {
        xiiInt32 errorCode = errno;
        // We can't send at the moment. Wait until we can send again.
        if (errorCode == EWOULDBLOCK)
        {
          m_previousSendOffset = uiNextOffset;
          static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::Send, m_clientSocketFd);
          return;
        }
        xiiLog::Error("[IPC]xiiPipeChannel_linux failed to send. Error {}", errorCode);
        InternalDisconnect();
        return;
      }

      uiToWrite -= static_cast<xiiUInt64>(res);
      uiNextOffset += res;
    }
    m_previousSendOffset = 0;

    {
      XII_LOCK(m_OutputQueueMutex);
      m_OutputQueue.PopFront();
      if (m_OutputQueue.IsEmpty())
      {
        return;
      }
      storage = &m_OutputQueue.PeekFront();
    }
  }
}

void xiiPipeChannel_linux::AcceptIncomingConnection()
{
  struct sockaddr_un incomingConnection = {};
  socklen_t          len                = sizeof(incomingConnection);
  m_clientSocketFd                      = accept4(m_serverSocketFd, (struct sockaddr*)&incomingConnection, &len, SOCK_NONBLOCK);
  if (m_clientSocketFd == -1)
  {
    xiiLog::Error("[IPC]Failed to accept incoming connection. Error {}", errno);
    // Wait for the next incoming connection
    listen(m_serverSocketFd, 1);
    static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::Accept, m_serverSocketFd);
  }
  else
  {
    SetConnectionState(ConnectionState::Connected);
    // We are connected. Register for incoming messages events.
    static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::IncomingMessage, m_clientSocketFd);
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
  static_cast<xiiMessageLoop_linux*>(m_pOwner)->RegisterWait(this, xiiMessageLoop_linux::WaitType::IncomingMessage, m_clientSocketFd);
}

void xiiPipeChannel_linux::ProcessIncomingPackages()
{
  while (true)
  {
    ssize_t recieveResult = recv(m_clientSocketFd, m_InputBuffer, XII_ARRAY_SIZE(m_InputBuffer), 0);
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
