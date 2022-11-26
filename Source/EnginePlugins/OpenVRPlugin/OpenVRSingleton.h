#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/Interfaces/VRInterface.h>
#include <OpenVRPlugin/Basics.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

struct xiiGameApplicationExecutionEvent;
typedef xiiTypedResourceHandle<class xiiShaderResource> xiiShaderResourceHandle;

class XII_OPENVRPLUGIN_DLL xiiOpenVR : public xiiVRInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiOpenVR, xiiVRInterface);

public:
  xiiOpenVR();

  virtual bool IsHmdPresent() const override;

  virtual bool Initialize() override;
  virtual void Deinitialize() override;
  virtual bool IsInitialized() const override;

  virtual const xiiHMDInfo&                  GetHmdInfo() const override;
  virtual void                               GetDeviceList(xiiHybridArray<xiiVRDeviceID, 64>& out_Devices) const override;
  virtual xiiVRDeviceID                      GetDeviceIDByType(xiiVRDeviceType::Enum type) const override;
  virtual const xiiVRDeviceState&            GetDeviceState(xiiVRDeviceID uiDeviceID) const override;
  virtual xiiEvent<const xiiVRDeviceEvent&>& DeviceEvents() override;

  virtual xiiViewHandle CreateVRView(
    const xiiRenderPipelineResourceHandle& hRenderPipeline,
    xiiCamera*                             pCamera,
    xiiGALMSAASampleCount::Enum            msaaCount) override;
  virtual xiiViewHandle       GetVRView() const override;
  virtual bool                DestroyVRView() override;
  virtual bool                SupportsCompanionView() override;
  virtual bool                SetCompanionViewRenderTarget(xiiGALTextureHandle hRenderTarget) override;
  virtual xiiGALTextureHandle GetCompanionViewRenderTarget() const override;

private:
  void GameApplicationEventHandler(const xiiGameApplicationExecutionEvent& e);
  void GALDeviceEventHandler(const xiiGALDeviceEvent& e);
  void OnBeginRender(xiiUInt64);

  void ReadHMDInfo();
  void OnDeviceActivated(xiiVRDeviceID uiDeviceID);
  void OnDeviceDeactivated(xiiVRDeviceID uiDeviceID);

  void UpdatePoses();
  void UpdateHands();
  void SetStageSpace(xiiVRStageSpace::Enum space);
  void SetHMDCamera(xiiCamera* pCamera);
  void UpdateCamera();

  xiiMat4   GetHMDProjectionEye(vr::Hmd_Eye nEye, float fNear, float fFar) const;
  xiiMat4   GetHMDEyePose(vr::Hmd_Eye nEye) const;
  xiiString GetTrackedDeviceString(
    vr::TrackedDeviceIndex_t  unDevice,
    vr::TrackedDeviceProperty prop,
    vr::TrackedPropertyError* peError = nullptr) const;

  static xiiMat4 ConvertSteamVRMatrix(const vr::HmdMatrix34_t& matPose);
  static xiiVec3 ConvertSteamVRVector(const vr::HmdVector3_t& vector);

private:
  bool m_bInitialized = false;

  vr::IVRSystem*       m_pHMD          = nullptr;
  vr::IVRRenderModels* m_pRenderModels = nullptr;

  xiiHMDInfo                        m_Info;
  xiiVRDeviceState                  m_DeviceState[vr::k_unMaxTrackedDeviceCount];
  xiiInt8                           m_iLeftControllerDeviceID  = -1;
  xiiInt8                           m_iRightControllerDeviceID = -1;
  xiiEvent<const xiiVRDeviceEvent&> m_DeviceEvents;

  xiiWorld*                m_pWorld               = nullptr;
  xiiCamera*               m_pCameraToSynchronize = nullptr;
  xiiEnum<xiiVRStageSpace> m_StageSpace;

  xiiCamera                        m_VRCamera;
  xiiUInt32                        m_uiSettingsModificationCounter = 0;
  xiiViewHandle                    m_hView;
  xiiGALRenderTagetSetup           m_RenderTargetSetup;
  xiiGALTextureCreationDescription m_eyeDesc;
  xiiGALTextureHandle              m_hColorRT;
  xiiGALTextureHandle              m_hDepthRT;

  xiiGALTextureHandle            m_hCompanionRenderTarget;
  xiiConstantBufferStorageHandle m_hCompanionConstantBuffer;
  xiiShaderResourceHandle        m_hCompanionShader;
};

XII_DYNAMIC_PLUGIN_DECLARATION(XII_OPENVRPLUGIN_DLL, xiiOpenVRPlugin);
