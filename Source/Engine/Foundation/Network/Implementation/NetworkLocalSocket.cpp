/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/NetworkLocalSocket.h>

#include <Foundation/Network/Implementation/NetworkLocalSocketAccess.h>

#include <utility>

xiiNetworkLocalSocket::~xiiNetworkLocalSocket() { Close(); }
xiiNetworkLocalSocket::xiiNetworkLocalSocket(xiiNetworkLocalSocket&& other) noexcept { *this = std::move(other); }

xiiNetworkLocalSocket& xiiNetworkLocalSocket::operator=(xiiNetworkLocalSocket&& other) noexcept
{
  if (this != &other)
  {
    Close();
    m_Endpoint          = std::exchange(other.m_Endpoint, {});
    m_sPeerIdentity     = std::move(other.m_sPeerIdentity);
    m_uiPeerProcessId   = std::exchange(other.m_uiPeerProcessId, 0);
    m_uiNativeHandle    = std::exchange(other.m_uiNativeHandle, InvalidHandle);
    m_uiContextId       = std::exchange(other.m_uiContextId, 0);
    m_uiDirectoryHandle = std::exchange(other.m_uiDirectoryHandle, InvalidHandle);
    m_uiSocketDevice    = std::exchange(other.m_uiSocketDevice, 0);
    m_uiSocketInode     = std::exchange(other.m_uiSocketInode, 0);
    m_uiWorker          = std::exchange(other.m_uiWorker, 0);
    m_bOpen             = std::exchange(other.m_bOpen, false);
    m_bListener         = std::exchange(other.m_bListener, false);
    m_bConnected        = std::exchange(other.m_bConnected, false);
  }
  return *this;
}

void xiiNetworkLocalSocket::Close()
{
  xiiNetworkIOInternal::LocalSocketAccess::Close(*this);
  m_Endpoint = {};
  m_sPeerIdentity.Clear();
  m_uiPeerProcessId = 0;
  m_uiContextId = 0;
  m_bOpen = m_bListener = m_bConnected = false;
}
