#include <GameEngine/GameEnginePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/World/World.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/Configuration/XRConfig.h>
#include <GameEngine/XR/DummyXR.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <GameEngine/XR/XRWindow.h>
#include <GraphicsCore/Components/CameraComponent.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Texture.h>

XII_IMPLEMENT_SINGLETON(xiiDummyXR);

xiiDummyXR::xiiDummyXR() :
  m_SingletonRegistrar(this)
{
}

bool xiiDummyXR::IsHmdPresent() const
{
  return true;
}

xiiResult xiiDummyXR::Initialize()
{
  if (m_bInitialized)
    return XII_FAILURE;

  m_Info.m_sDeviceName          = "Dummy VR device";
  m_Info.m_vEyeRenderTargetSize = xiiSizeU32(640, 720);

  m_GALdeviceEventsId = xiiGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiDummyXR::GALDeviceEventHandler, this));
  m_ExecutionEventsId = xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(xiiMakeDelegate(&xiiDummyXR::GameApplicationEventHandler, this));

  m_bInitialized = true;
  return XII_SUCCESS;
}

void xiiDummyXR::Deinitialize()
{
  m_bInitialized = false;
  if (m_GALdeviceEventsId != 0)
  {
    xiiGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(m_GALdeviceEventsId);
  }
  if (m_ExecutionEventsId != 0)
  {
    xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(m_ExecutionEventsId);
  }
}

bool xiiDummyXR::IsInitialized() const
{
  return m_bInitialized;
}

const xiiHMDInfo& xiiDummyXR::GetHmdInfo() const
{
  return m_Info;
}

xiiXRInputDevice& xiiDummyXR::GetXRInput() const
{
  return m_Input;
}

bool xiiDummyXR::SupportsCompanionView()
{
  return true;
}

xiiUniquePtr<xiiActor> xiiDummyXR::CreateActor(xiiView* pView, xiiEnum<xiiGALSampleCount> msaaCount, xiiUniquePtr<xiiWindowBase> pCompanionWindow, xiiUniquePtr<xiiWindowOutputTargetGAL> pCompanionWindowOutput)
{
  XII_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Create dummy swap chain
  {
    xiiGALTextureCreationDescription textureDesc;
    textureDesc.m_Size               = m_Info.m_vEyeRenderTargetSize;
    textureDesc.m_uiArraySizeOrDepth = 2;
    textureDesc.m_uiMipLevels        = 1;
    textureDesc.m_uiSampleCount      = msaaCount;
    textureDesc.m_Usage              = xiiGALResourceUsage::Immutable;
    textureDesc.m_BindFlags.Add(xiiGALBindFlags::RenderTarget | xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource);

    m_hColorRT = pDevice->CreateTexture(textureDesc);

    textureDesc.m_Format = xiiGALTextureFormat::D24UNormalizedS8UInt;
    m_hDepthRT           = pDevice->CreateTexture(textureDesc);
  }

  // SetHMDCamera
  {
    m_pCameraToSynchronize = pView->GetCamera();
    m_pCameraToSynchronize->SetCameraMode(xiiCameraMode::Stereo, m_pCameraToSynchronize->GetFovOrDim(), m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
  }

  xiiUniquePtr<xiiActor> pActor = XII_DEFAULT_NEW(xiiActor, "DummyXR", this);
  XII_ASSERT_DEV((pCompanionWindow != nullptr) == (pCompanionWindowOutput != nullptr), "Both companionWindow and companionWindowOutput must either be null or valid.");

  xiiUniquePtr<xiiActorPluginWindowXR> pActorPlugin = XII_DEFAULT_NEW(xiiActorPluginWindowXR, this, std::move(pCompanionWindow), std::move(pCompanionWindowOutput));
  m_pCompanion                                      = static_cast<xiiWindowOutputTargetXR*>(pActorPlugin->GetOutputTarget());

  pActor->AddPlugin(std::move(pActorPlugin));

  m_hView  = pView->GetHandle();
  m_pWorld = pView->GetWorld();
  XII_ASSERT_DEV(m_pWorld != nullptr, "");


  xiiGALRenderTargets renderTargets;
  renderTargets.m_hRTs[0]   = m_hColorRT;
  renderTargets.m_hDSTarget = m_hDepthRT;
  pView->SetRenderTargets(renderTargets);

  pView->SetViewport(xiiRectFloat((float)m_Info.m_vEyeRenderTargetSize.width, (float)m_Info.m_vEyeRenderTargetSize.height));

  return std::move(pActor);
}

xiiGALTextureHandle xiiDummyXR::GetCurrentTexture()
{
  return m_hColorRT;
}

void xiiDummyXR::OnActorDestroyed()
{
  if (m_hView.IsInvalidated())
    return;

  m_pCompanion           = nullptr;
  m_pWorld               = nullptr;
  m_pCameraToSynchronize = nullptr;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (!m_hColorRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hColorRT);
    m_hColorRT.Invalidate();
  }
  if (!m_hDepthRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hDepthRT);
    m_hDepthRT.Invalidate();
  }

  xiiRenderWorld::RemoveMainView(m_hView);
  m_hView.Invalidate();
}

void xiiDummyXR::GALDeviceEventHandler(const xiiGALDeviceEvent& e)
{
  if (e.m_Type == xiiGALDeviceEventType::BeforeBeginFrame)
  {
  }
  else if (e.m_Type == xiiGALDeviceEventType::BeforeEndFrame)
  {
    // Screenshots are taken during present callback so ideally we need to render the companion view before that to capture the current XR frame.
    // For backwards compatibility draw the companion view here (after present) which means that if We are in frame 100, we just rendered frame 99 (due to multi-threaded rendering) but due to this bug here we captured frame 98 for image comparison.
    // This will change once read back API is refactored to be async and will be executed at a different point in time.
    if (m_pCompanion)
    {
      // We capture the companion view in unit tests so we don't want to skip any frames.
      m_pCompanion->RenderCompanionView(false);
    }
  }
}

void xiiDummyXR::GameApplicationEventHandler(const xiiGameApplicationExecutionEvent& e)
{
  if (e.m_Type == xiiGameApplicationExecutionEvent::Type::BeforePresent)
  {
  }
  else if (e.m_Type == xiiGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    xiiView* pView0 = nullptr;
    if (xiiRenderWorld::TryGetView(m_hView, pView0))
    {
      if (xiiWorld* pWorld0 = pView0->GetWorld())
      {
        XII_LOCK(pWorld0->GetWriteMarker());
        xiiCameraComponent* pCameraComponent = pWorld0->GetComponentManager<xiiCameraComponentManager>()->GetCameraByUsageHint(xiiCameraUsageHint::MainView);
        if (!pCameraComponent)
          return;

        pCameraComponent->SetCameraMode(xiiCameraMode::Stereo);

        // Projection
        {
          const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.width / (float)m_Info.m_vEyeRenderTargetSize.height;

          xiiMat4 mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(xiiAngle::Degree(pCameraComponent->GetFieldOfView()), fAspectRatio,
                                                                                      pCameraComponent->GetNearPlane(), xiiMath::Max(pCameraComponent->GetNearPlane() + 0.00001f, pCameraComponent->GetFarPlane()));

          m_pCameraToSynchronize->SetStereoProjection(mProj, mProj, fAspectRatio);
        }

        // Update camera view
        {
          xiiTransform add;
          add.SetIdentity();
          xiiView* pView = nullptr;
          if (xiiRenderWorld::TryGetView(m_hView, pView))
          {
            if (const xiiWorld* pWorld = pView->GetWorld())
            {
              XII_LOCK(pWorld->GetReadMarker());
              if (const xiiStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<xiiStageSpaceComponentManager>())
              {
                if (const xiiStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
                {
                  xiiEnum<xiiXRStageSpace> stageSpace = pStage->GetStageSpace();
                  if (m_StageSpace != stageSpace)
                    m_StageSpace = pStage->GetStageSpace();
                  add = pStage->GetOwner()->GetGlobalTransform();
                }
              }
            }
          }

          {
            // Update device state
            xiiQuat rot;
            rot.SetIdentity();
            xiiVec3 pos = xiiVec3::ZeroVector();
            if (m_StageSpace == xiiXRStageSpace::Standing)
            {
              pos.z = m_fHeadHeight;
            }

            m_Input.m_DeviceState[0].m_vGripPosition      = pos;
            m_Input.m_DeviceState[0].m_qGripRotation      = rot;
            m_Input.m_DeviceState[0].m_vAimPosition       = pos;
            m_Input.m_DeviceState[0].m_qAimRotation       = rot;
            m_Input.m_DeviceState[0].m_Type               = xiiXRDeviceType::HMD;
            m_Input.m_DeviceState[0].m_bGripPoseIsValid   = true;
            m_Input.m_DeviceState[0].m_bAimPoseIsValid    = true;
            m_Input.m_DeviceState[0].m_bDeviceIsConnected = true;
          }

          // Set view matrix
          {
            const float   fHeight         = m_StageSpace == xiiXRStageSpace::Standing ? m_fHeadHeight : 0.0f;
            const xiiMat4 mStageTransform = add.GetInverse().GetAsMat4();
            xiiMat4       poseLeft;
            poseLeft.SetTranslationMatrix(xiiVec3(0, -m_fEyeOffset, fHeight));
            xiiMat4 poseRight;
            poseRight.SetTranslationMatrix(xiiVec3(0, m_fEyeOffset, fHeight));

            // XII Forward is +X, need to add this to align the forward projection
            const xiiMat4 viewMatrix          = xiiGraphicsUtils::CreateLookAtViewMatrix(xiiVec3::ZeroVector(), xiiVec3(1, 0, 0), xiiVec3(0, 0, 1));
            const xiiMat4 mViewTransformLeft  = viewMatrix * mStageTransform * poseLeft.GetInverse();
            const xiiMat4 mViewTransformRight = viewMatrix * mStageTransform * poseRight.GetInverse();

            m_pCameraToSynchronize->SetViewMatrix(mViewTransformLeft, xiiCameraEye::Left);
            m_pCameraToSynchronize->SetViewMatrix(mViewTransformRight, xiiCameraEye::Right);
          }
        }
      }
    }
  }
}


//////////////////////////////////////////////////////////////////////////

void xiiDummyXRInput::GetDeviceList(xiiHybridArray<xiiXRDeviceID, 64>& out_devices) const
{
  out_devices.PushBack(0);
}

xiiXRDeviceID xiiDummyXRInput::GetDeviceIDByType(xiiXRDeviceType::Enum type) const
{
  xiiXRDeviceID deviceID = -1;
  switch (type)
  {
    case xiiXRDeviceType::HMD:
      deviceID = 0;
      break;
    default:
      deviceID = -1;
  }

  return deviceID;
}

const xiiXRDeviceState& xiiDummyXRInput::GetDeviceState(xiiXRDeviceID deviceID) const
{
  XII_ASSERT_DEV(deviceID < 1 && deviceID >= 0, "Invalid device ID.");
  return m_DeviceState[deviceID];
}

xiiString xiiDummyXRInput::GetDeviceName(xiiXRDeviceID deviceID) const
{
  XII_ASSERT_DEV(deviceID < 1 && deviceID >= 0, "Invalid device ID.");
  return "Dummy HMD";
}

xiiBitflags<xiiXRDeviceFeatures> xiiDummyXRInput::GetDeviceFeatures(xiiXRDeviceID deviceID) const
{
  XII_ASSERT_DEV(deviceID < 1 && deviceID >= 0, "Invalid device ID.");
  return xiiXRDeviceFeatures::AimPose | xiiXRDeviceFeatures::GripPose;
}

void xiiDummyXRInput::InitializeDevice()
{
}

void xiiDummyXRInput::UpdateInputSlotValues()
{
}

void xiiDummyXRInput::RegisterInputSlots()
{
}


XII_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_DummyXR);
