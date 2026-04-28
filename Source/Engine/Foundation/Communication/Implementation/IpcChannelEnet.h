/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/IpcChannel.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

class xiiRemoteInterface;
class xiiRemoteMessage;

class XII_FOUNDATION_DLL xiiIpcChannelEnet : public xiiIpcChannel
{
public:
  xiiIpcChannelEnet(xiiStringView sAddress, Mode::Enum mode);
  ~xiiIpcChannelEnet();

protected:
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;
  virtual bool RequiresRegularTick() override { return true; }
  virtual void Tick() override;
  void         NetworkMessageHandler(xiiRemoteMessage& msg);
  void         EnetEventHandler(const xiiRemoteEvent& e);

  xiiString                        m_sAddress;
  xiiString                        m_sLastAddress;
  xiiTime                          m_LastConnectAttempt;
  xiiUniquePtr<xiiRemoteInterface> m_pNetwork;
};

#endif
