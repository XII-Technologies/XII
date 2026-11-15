/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/NetworkError.h>

XII_FOUNDATION_INTERNAL_HEADER

namespace xiiNetworkIOInternal
{
  struct NativeThread
  {
    void* m_pData = nullptr;
  };

  xiiNetworkError StartThread(NativeThread& thread, void (*pFunction)(void*), void* pUserData);
  void            JoinThread(NativeThread& thread);
} // namespace xiiNetworkIOInternal
