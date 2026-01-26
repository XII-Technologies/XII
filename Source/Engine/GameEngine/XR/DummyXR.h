#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/XR/Declarations.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>
#include <GraphicsCore/Pipeline/Declarations.h>

struct xiiGALDeviceEvent;
struct xiiGameApplicationExecutionEvent;
class xiiWindowOutputTargetXR;

class XII_GAMEENGINE_DLL xiiDummyXRInput : public xiiXRInputDevice
{

public:
  void                             GetDeviceList(xiiHybridArray<xiiXRDeviceID, 64>& out_devices) const override;
  xiiXRDeviceID                    GetDeviceIDByType(xiiXRDeviceType::Enum type) const override;
  const xiiXRDeviceState&          GetDeviceState(xiiXRDeviceID deviceID) const override;
  xiiString                        GetDeviceName(xiiXRDeviceID deviceID) const override;
  xiiBitflags<xiiXRDeviceFeatures> GetDeviceFeatures(xiiXRDeviceID deviceID) const override;

protected:
  void InitializeDevice() override;
  void UpdateInputSlotValues() override;
  void RegisterInputSlots() override;

protected:
  friend class xiiDummyXR;

  xiiXRDeviceState m_DeviceState[1];
};

class XII_GAMEENGINE_DLL xiiDummyXR : public xiiXRInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiDummyXR, xiiXRInterface);

public:
  xiiDummyXR();
  ~xiiDummyXR() = default;

  bool                        IsHmdPresent() const override;
  xiiResult                   Initialize() override;
  void                        Deinitialize() override;
  bool                        IsInitialized() const override;
  const xiiHMDInfo&           GetHmdInfo() const override;
  xiiXRInputDevice&           GetXRInput() const override;
  bool                        SupportsCompanionView() override;
  xiiUniquePtr<xiiActor>      CreateActor(xiiView* pView, xiiEnum<xiiGALSampleCount> msaaCount = xiiGALSampleCount::OneSample, xiiUniquePtr<xiiWindowBase> pCompanionWindow = nullptr, xiiUniquePtr<xiiWindowOutputTargetGAL> pCompanionWindowOutput = nullptr) override;
  xiiSharedPtr<xiiGALTexture> GetCurrentTexture() override;
  void                        OnActorDestroyed() override;
  void                        GALDeviceEventHandler(const xiiGALDeviceEvent& e);
  void                        GameApplicationEventHandler(const xiiGameApplicationExecutionEvent& e);

protected:
  float m_fHeadHeight = 1.7f;
  float m_fEyeOffset  = 0.05f;

  xiiHMDInfo              m_Info;
  mutable xiiDummyXRInput m_Input;
  bool                    m_bInitialized = false;

  xiiEventSubscriptionID m_GALdeviceEventsId = 0;
  xiiEventSubscriptionID m_ExecutionEventsId = 0;

  xiiWorld*                m_pWorld               = nullptr;
  xiiCamera*               m_pCameraToSynchronize = nullptr;
  xiiEnum<xiiXRStageSpace> m_StageSpace           = xiiXRStageSpace::Seated;

  xiiViewHandle               m_hView;
  xiiSharedPtr<xiiGALTexture> m_pColorRT;
  xiiSharedPtr<xiiGALTexture> m_pDepthRT;

  xiiWindowOutputTargetXR* m_pCompanion = nullptr;
};
