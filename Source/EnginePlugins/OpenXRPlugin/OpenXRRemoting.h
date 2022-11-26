#pragma once

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
#  include <OpenXRPlugin/Basics.h>
#  include <OpenXRPlugin/OpenXRIncludes.h>

#  include <Foundation/Configuration/Singleton.h>
#  include <GameEngine/XR/XRRemotingInterface.h>


class xiiOpenXR;

class xiiOpenXRRemoting : public xiiXRRemotingInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiOpenXRRemoting, xiiXRRemotingInterface);

public:
  xiiOpenXRRemoting(xiiOpenXR* pOpenXR);
  ~xiiOpenXRRemoting();

  virtual xiiResult Initialize() override;
  virtual xiiResult Deinitialize() override;
  virtual bool      IsInitialized() const override;

  virtual xiiResult                             Connect(const char* remoteHostName, uint16_t remotePort, bool enableAudio, int maxBitrateKbps) override;
  virtual xiiResult                             Disconnect() override;
  virtual xiiEnum<xiiXRRemotingConnectionState> GetConnectionState() const override;
  virtual xiiXRRemotingConnectionEvent&         GetConnectionEvent() override;

private:
  friend class xiiOpenXR;

  void HandleEvent(const XrEventDataBuffer& event);

private:
  bool                         m_bInitialized = false;
  xiiString                    m_sPreviousRuntime;
  xiiOpenXR*                   m_pOpenXR = nullptr;
  xiiXRRemotingConnectionEvent m_event;
};
#endif
