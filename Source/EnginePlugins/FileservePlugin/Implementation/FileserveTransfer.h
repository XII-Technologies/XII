/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Network/NetworkHost.h>

namespace xiiFileserveTransfer
{
  enum class Operation : xiiUInt8
  {
    Mount = 1,
    Unmount,
    Read,
    Delete,
    UploadBegin,
    UploadChunk,
    UploadEnd,
    UploadResult,
    DownloadChunk,
    DownloadResult,
    Reload,
    Invalidate,
    CompileShader,
    ShaderResult
  };

  constexpr xiiUInt32 ChunkBytes  = 16 * 1024;
  constexpr xiiUInt32 PacketBytes = 64 * 1024 + 128;

  /// A FileServe operation and its typed field sequence. The host supplies all framing.
  class Request final
  {
  public:
    explicit Request(Operation operation);
    xiiStreamWriter& GetWriter() { return m_Writer; }
    xiiNetworkError  Send(xiiNetworkHost& host, xiiNetworkChannel channel) const;

  private:
    xiiContiguousMemoryStreamStorage m_Storage;
    xiiMemoryStreamWriter            m_Writer;
  };

  xiiResult       Start(xiiNetworkHost& host, bool bServer);
  xiiNetworkError Listen(xiiNetworkHost& host, xiiUInt16 uiPort, xiiNetworkListener& out_listener);
  xiiNetworkError Connect(xiiNetworkHost& host, xiiStringView sAddress, xiiNetworkConnection& out_connection);
  xiiResult       ReadOperation(xiiRawMemoryStreamReader& reader, Operation& out_operation);
} // namespace xiiFileserveTransfer
