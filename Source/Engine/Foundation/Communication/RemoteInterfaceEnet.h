/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Communication/RemoteInterface.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

/// An implementation for xiiRemoteInterface built on top of Enet
class XII_FOUNDATION_DLL xiiRemoteInterfaceEnet : public xiiRemoteInterface
{
public:
  ~xiiRemoteInterfaceEnet();

  /// Allocates a new instance with the given allocator
  static xiiInternal::NewInstance<xiiRemoteInterfaceEnet> Make(xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator());

  /// The port through which the connection was started
  xiiUInt16 GetPort() const { return m_uiPort; }

private:
  xiiRemoteInterfaceEnet();
  friend class xiiRemoteInterfaceEnetImpl;

protected:
  xiiUInt16 m_uiPort = 0;
};

#endif
