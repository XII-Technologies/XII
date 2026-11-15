/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/Protocol/NetworkFrame.h>

XII_FOUNDATION_INTERNAL_HEADER

/// One incremental receive sequence per TCP connection or QUIC stream. Only the protocol driver
/// touches this state. Native stream buffers may remain pended while its application credits fill.
struct xiiNetworkReceiveState
{
  xiiNetworkFrameDecoder m_Decoder{8192};
  xiiUInt64              m_uiReservedBytes       = 0;
  xiiUInt64              m_uiFrames              = 0;
  xiiUInt32              m_uiBoundChannel        = 0;
  bool                   m_bEventReserved        = false;
  bool                   m_bRequireControlFrames = false;
  bool                   m_bRequireDataFrames    = false;
  bool                   m_bSingleFrame          = false;
  bool                   m_bAwaitingChannel      = false;
  bool                   m_bHeaderValidated      = false;
};
