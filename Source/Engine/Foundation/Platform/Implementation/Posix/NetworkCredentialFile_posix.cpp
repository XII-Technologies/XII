/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_OSX)

#  include <Foundation/Network/Security/Implementation/NetworkCertificatePlatform.h>
#  include <Foundation/Strings/String.h>

#  include <cerrno>
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>

#  include <cstring>
#  include <utility>

namespace
{
  struct NetworkCredentialDescriptor
  {
    explicit NetworkCredentialDescriptor(int hFile) : m_hFile(hFile) {}
    ~NetworkCredentialDescriptor() { ::close(m_hFile); }
    NetworkCredentialDescriptor(const NetworkCredentialDescriptor&)            = delete;
    NetworkCredentialDescriptor& operator=(const NetworkCredentialDescriptor&) = delete;
    NetworkCredentialDescriptor(NetworkCredentialDescriptor&&)                 = delete;
    NetworkCredentialDescriptor& operator=(NetworkCredentialDescriptor&&)      = delete;
    int                          m_hFile;
  };

  xiiNetworkError NetworkCredentialFileError(int iError)
  {
    auto code = xiiNetworkErrorCode::Unknown;
    switch (iError)
    {
      case ENOENT:
        code = xiiNetworkErrorCode::NotOpen;
        break;
      case EACCES:
      case EPERM:
        code = xiiNetworkErrorCode::PermissionDenied;
        break;
      case ENOMEM:
      case EMFILE:
      case ENFILE:
        code = xiiNetworkErrorCode::ResourceExhausted;
        break;
      case EINVAL:
        code = xiiNetworkErrorCode::InvalidArgument;
        break;
      default:
        break;
    }
    return xiiNetworkError(code, iError, xiiNetworkErrorDomain::CredentialFile);
  }
} // namespace

namespace xiiNetworkSecurityInternal
{
  xiiNetworkError LoadCredentialFile(xiiStringView sPath, xiiUInt32 uiMaximumBytes, xiiDynamicArray<xiiUInt8>& out_bytes)
  {
    if (sPath.IsEmpty() || std::memchr(sPath.GetStartPointer(), 0, sPath.GetElementCount()) != nullptr)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    const xiiString sFile(sPath);
    int             hFile = -1;
    do
    {
      hFile = ::open(sFile.GetData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    } while (hFile < 0 && errno == EINTR);
    if (hFile < 0)
    {
      return NetworkCredentialFileError(errno);
    }
    NetworkCredentialDescriptor file(hFile);
    struct stat                 information = {};
    if (::fstat(hFile, &information) != 0)
    {
      return NetworkCredentialFileError(errno);
    }
    if (!S_ISREG(information.st_mode) || information.st_size <= 0 || static_cast<xiiUInt64>(information.st_size) > uiMaximumBytes)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    xiiDynamicArray<xiiUInt8> bytes;
    bytes.SetCountUninitialized(static_cast<xiiUInt32>(information.st_size));
    xiiUInt32 uiRead = 0;
    while (uiRead < bytes.GetCount())
    {
      const ssize_t iRead = ::read(hFile, bytes.GetData() + uiRead, bytes.GetCount() - uiRead);
      if (iRead < 0 && errno == EINTR)
      {
        continue;
      }
      if (iRead <= 0)
      {
        const auto error = iRead < 0 ? NetworkCredentialFileError(errno) : xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
        ClearSecret(bytes);
        return error;
      }
      uiRead += static_cast<xiiUInt32>(iRead);
    }
    out_bytes = std::move(bytes);
    return {};
  }
} // namespace xiiNetworkSecurityInternal

#endif
