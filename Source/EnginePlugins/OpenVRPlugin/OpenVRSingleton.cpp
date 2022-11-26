#include <OpenVRPlugin/OpenVRPluginPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Time/Stopwatch.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Interfaces/SoundInterface.h>
#include <OpenVRPlugin/OpenVRIncludes.h>
#include <OpenVRPlugin/OpenVRSingleton.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

#include <../../../Data/Base/Shaders/Pipeline/VRCompanionViewConstants.h>
#include <Core/World/World.h>
#include <GameEngine/VirtualReality/Components/StageSpaceComponent.h>
#include <RendererCore/Shader/ShaderResource.h>

XII_IMPLEMENT_SINGLETON(xiiOpenVR);

static xiiOpenVR g_OpenVRSingleton;

xiiOpenVR::xiiOpenVR() :
  m_SingletonRegistrar(this)
{
  m_bInitialized = false;
  m_DeviceState[0].m_mPose.SetIdentity();
}

bool xiiOpenVR::IsHmdPresent() const
{
  return vr::VR_IsHmdPresent();
}

bool xiiOpenVR::Initialize()
{
  if (m_bInitialized)
    return true;

  vr::EVRInitError eError = vr::VRInitError_None;
  m_pHMD                  = vr::VR_Init(&eError, vr::VRApplication_Scene);
  if (eError != vr::VRInitError_None)
  {
    m_pHMD = nullptr;
    xiiLog::Error("Unable to init OpenVR runtime: {0}", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
    return false;
  }
  m_pRenderModels = (vr::IVRRenderModels*)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
  if (!m_pRenderModels)
  {
    m_pHMD = nullptr;
    vr::VR_Shutdown();
    xiiLog::Error("Unable to get OpenVR render model interface: {0}", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
    return false;
  }

  m_bInitialized = true;
  xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(
    xiiMakeDelegate(&xiiOpenVR::GameApplicationEventHandler, this));
  xiiRenderWorld::s_BeginRenderEvent.AddEventHandler(xiiMakeDelegate(&xiiOpenVR::OnBeginRender, this));
  xiiGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiOpenVR::GALDeviceEventHandler, this));
  ReadHMDInfo();

  SetStageSpace(xiiVRStageSpace::Standing);
  for (xiiVRDeviceID uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
  {
    if (m_pHMD->IsTrackedDeviceConnected(uiDeviceID))
    {
      OnDeviceActivated(uiDeviceID);
    }
  }

  UpdatePoses();

  xiiLog::Success("OpenVR initialized successfully.");
  return true;
}

void xiiOpenVR::Deinitialize()
{
  if (m_bInitialized)
  {
    xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(
      xiiMakeDelegate(&xiiOpenVR::GameApplicationEventHandler, this));
    xiiRenderWorld::s_BeginRenderEvent.RemoveEventHandler(xiiMakeDelegate(&xiiOpenVR::OnBeginRender, this));
    xiiGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiOpenVR::GALDeviceEventHandler, this));

    for (xiiVRDeviceID uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
    {
      if (m_DeviceState[uiDeviceID].m_bDeviceIsConnected)
      {
        OnDeviceDeactivated(uiDeviceID);
      }
    }

    SetCompanionViewRenderTarget(xiiGALTextureHandle());
    DestroyVRView();

    vr::VR_Shutdown();
    m_pHMD          = nullptr;
    m_pRenderModels = nullptr;
    m_bInitialized  = false;
  }
}

bool xiiOpenVR::IsInitialized() const
{
  return m_bInitialized;
}

const xiiHMDInfo& xiiOpenVR::GetHmdInfo() const
{
  XII_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");
  return m_Info;
}

void xiiOpenVR::GetDeviceList(xiiHybridArray<xiiVRDeviceID, 64>& out_Devices) const
{
  XII_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");
  for (xiiVRDeviceID i = 0; i < vr::k_unMaxTrackedDeviceCount; i++)
  {
    if (m_DeviceState[i].m_bDeviceIsConnected)
    {
      out_Devices.PushBack(i);
    }
  }
}

xiiVRDeviceID xiiOpenVR::GetDeviceIDByType(xiiVRDeviceType::Enum type) const
{
  xiiVRDeviceID deviceID = -1;
  switch (type)
  {
    case xiiVRDeviceType::HMD:
      deviceID = 0;
      break;
    case xiiVRDeviceType::LeftController:
      deviceID = m_iLeftControllerDeviceID;
      break;
    case xiiVRDeviceType::RightController:
      deviceID = m_iRightControllerDeviceID;
      break;
    default:
      deviceID = type - xiiVRDeviceType::DeviceID0;
      break;
  }

  if (deviceID != -1 && !m_DeviceState[deviceID].m_bDeviceIsConnected)
  {
    deviceID = -1;
  }
  return deviceID;
}

const xiiVRDeviceState& xiiOpenVR::GetDeviceState(xiiVRDeviceID uiDeviceID) const
{
  XII_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");
  XII_ASSERT_DEV(uiDeviceID < vr::k_unMaxTrackedDeviceCount, "Invalid device ID.");
  XII_ASSERT_DEV(m_DeviceState[uiDeviceID].m_bDeviceIsConnected, "Invalid device ID.");
  return m_DeviceState[uiDeviceID];
}

xiiEvent<const xiiVRDeviceEvent&>& xiiOpenVR::DeviceEvents()
{
  return m_DeviceEvents;
}

xiiViewHandle xiiOpenVR::CreateVRView(const xiiRenderPipelineResourceHandle& hRenderPipeline, xiiCamera* pCamera, xiiGALMSAASampleCount::Enum msaaCount)
{
  SetHMDCamera(pCamera);

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  xiiView* pMainView = nullptr;
  m_hView            = xiiRenderWorld::CreateView("Holographic View", pMainView);
  pMainView->SetCameraUsageHint(xiiCameraUsageHint::MainView);

  {
    xiiGALTextureCreationDescription tcd;
    tcd.SetAsRenderTarget(m_Info.m_vEyeRenderTargetSize.x, m_Info.m_vEyeRenderTargetSize.y, xiiGALResourceFormat::RGBAUByteNormalizedsRGB, msaaCount);
    tcd.m_uiArraySize = 2;
    m_hColorRT        = pDevice->CreateTexture(tcd);

    // Store desc for one eye for later.
    m_eyeDesc               = tcd;
    m_eyeDesc.m_uiArraySize = 1;
  }
  {
    xiiGALTextureCreationDescription tcd;
    tcd.SetAsRenderTarget(m_Info.m_vEyeRenderTargetSize.x, m_Info.m_vEyeRenderTargetSize.y, xiiGALResourceFormat::DFloat, msaaCount);
    tcd.m_uiArraySize = 2;
    m_hDepthRT        = pDevice->CreateTexture(tcd);
  }

  m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_hColorRT))
    .SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(m_hDepthRT));

  pMainView->SetRenderTargetSetup(m_RenderTargetSetup);
  pMainView->SetRenderPipelineResource(hRenderPipeline);
  pMainView->SetCamera(&m_VRCamera);
  pMainView->SetRenderPassProperty("ColorSource", "MSAA_Mode", (xiiInt32)msaaCount);
  pMainView->SetRenderPassProperty("DepthStencil", "MSAA_Mode", (xiiInt32)msaaCount);

  pMainView->SetViewport(xiiRectFloat((float)m_Info.m_vEyeRenderTargetSize.x, (float)m_Info.m_vEyeRenderTargetSize.y));

  xiiRenderWorld::AddMainView(m_hView);
  return m_hView;
}

xiiViewHandle xiiOpenVR::GetVRView() const
{
  return m_hView;
}

bool xiiOpenVR::DestroyVRView()
{
  if (m_hView.IsInvalidated())
    return false;

  m_pWorld = nullptr;
  SetHMDCamera(nullptr);

  vr::VRCompositor()->ClearLastSubmittedFrame();
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  xiiRenderWorld::RemoveMainView(m_hView);
  xiiRenderWorld::DeleteView(m_hView);
  m_hView.Invalidate();
  m_RenderTargetSetup.DestroyAllAttachedViews();

  pDevice->DestroyTexture(m_hColorRT);
  m_hColorRT.Invalidate();
  pDevice->DestroyTexture(m_hDepthRT);
  m_hDepthRT.Invalidate();
  return true;
}

bool xiiOpenVR::SupportsCompanionView()
{
  return true;
}

bool xiiOpenVR::SetCompanionViewRenderTarget(xiiGALTextureHandle hRenderTarget)
{
  if (!m_hCompanionRenderTarget.IsInvalidated() && !hRenderTarget.IsInvalidated())
  {
    // Maintain already created resources (just switch target).
  }
  else if (!m_hCompanionRenderTarget.IsInvalidated() && hRenderTarget.IsInvalidated())
  {
    // Delete companion resources.
    xiiRenderContext::DeleteConstantBufferStorage(m_hCompanionConstantBuffer);
    m_hCompanionConstantBuffer.Invalidate();
  }
  else if (m_hCompanionRenderTarget.IsInvalidated() && !hRenderTarget.IsInvalidated())
  {
    // Create companion resources.
    m_hCompanionShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/VRCompanionView.xiiShader");
    XII_ASSERT_DEV(m_hCompanionShader.IsValid(), "Could not load VR companion view shader!");
    m_hCompanionConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiVRCompanionViewConstants>();
    m_hCompanionRenderTarget   = hRenderTarget;
  }
  return true;
}

xiiGALTextureHandle xiiOpenVR::GetCompanionViewRenderTarget() const
{
  return m_hCompanionRenderTarget;
}

void xiiOpenVR::GameApplicationEventHandler(const xiiGameApplicationExecutionEvent& e)
{
  XII_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  if (e.m_Type == xiiGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    vr::VREvent_t event;
    while (m_pHMD->PollNextEvent(&event, sizeof(event)))
    {
      switch (event.eventType)
      {
        case vr::VREvent_TrackedDeviceActivated:
        {
          OnDeviceActivated((xiiVRDeviceID)event.trackedDeviceIndex);
        }
        break;
        case vr::VREvent_TrackedDeviceDeactivated:
        {
          OnDeviceDeactivated((xiiVRDeviceID)event.trackedDeviceIndex);
        }
        break;
      }
    }
  }
  else if (e.m_Type == xiiGameApplicationExecutionEvent::Type::BeforePresent)
  {
    if (m_hView.IsInvalidated())
      return;

    xiiGALDevice*  pDevice     = xiiGALDevice::GetDefaultDevice();
    xiiGALContext* pGALContext = pDevice->GetPrimaryContext();

    xiiGALTextureHandle hLeft;
    xiiGALTextureHandle hRight;

    if (m_eyeDesc.m_SampleCount == xiiGALMSAASampleCount::None)
    {
      // OpenVR does not support submitting texture arrays, as there is no slice param in the VRTextureBounds_t :-/
      // However, it reads it just fine for the first slice (left eye) so we only need to copy the right eye into a
      // second texture to submit it :-)
      hLeft  = m_hColorRT;
      hRight = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(m_eyeDesc);
      xiiGALTextureSubresource sourceSubRes;
      sourceSubRes.m_uiArraySlice = 1;
      sourceSubRes.m_uiMipLevel   = 0;

      xiiGALTextureSubresource destSubRes;
      destSubRes.m_uiArraySlice = 0;
      destSubRes.m_uiMipLevel   = 0;

      pGALContext->CopyTextureRegion(hRight, destSubRes, xiiVec3U32(0, 0, 0), m_hColorRT, sourceSubRes,
                                     xiiBoundingBoxu32(xiiVec3U32(0, 0, 0), xiiVec3U32(m_Info.m_vEyeRenderTargetSize.x, m_Info.m_vEyeRenderTargetSize.y, 1)));
    }
    else
    {
      // Submitting the multi-sampled m_hColorRT will cause dx errors on submit :-/
      // So have to resolve both eyes.
      xiiGALTextureCreationDescription tempDesc = m_eyeDesc;
      tempDesc.m_SampleCount                    = xiiGALMSAASampleCount::None;
      hLeft                                     = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempDesc);
      hRight                                    = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempDesc);

      xiiGALTextureSubresource sourceSubRes;
      sourceSubRes.m_uiArraySlice = 0;
      sourceSubRes.m_uiMipLevel   = 0;

      xiiGALTextureSubresource destSubRes;
      destSubRes.m_uiArraySlice = 0;
      destSubRes.m_uiMipLevel   = 0;
      pGALContext->ResolveTexture(hLeft, destSubRes, m_hColorRT, sourceSubRes);
      sourceSubRes.m_uiArraySlice = 1;
      pGALContext->ResolveTexture(hRight, destSubRes, m_hColorRT, sourceSubRes);
    }

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    // TODO: We currently assume that we always use dx11 on windows. Need to figure out how to check for that.
    vr::Texture_t Texture;
    Texture.eType       = vr::TextureType_DirectX;
    Texture.eColorSpace = vr::ColorSpace_Auto;

    {
      const xiiGALTexture*     pTex   = pDevice->GetTexture(hLeft);
      const xiiGALTextureDX11* pTex11 = static_cast<const xiiGALTextureDX11*>(pTex);
      Texture.handle                  = pTex11->GetDXTexture();
      vr::EVRCompositorError err      = vr::VRCompositor()->Submit(vr::Eye_Left, &Texture, nullptr);
    }

    {
      const xiiGALTexture*     pTex   = pDevice->GetTexture(hRight);
      const xiiGALTextureDX11* pTex11 = static_cast<const xiiGALTextureDX11*>(pTex);
      Texture.handle                  = pTex11->GetDXTexture();
      vr::EVRCompositorError err      = vr::VRCompositor()->Submit(vr::Eye_Right, &Texture, nullptr);
    }
#endif

    if (m_eyeDesc.m_SampleCount == xiiGALMSAASampleCount::None)
    {
      xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hRight);
    }
    else
    {
      xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hLeft);
      xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hRight);
    }
  }
  else if (e.m_Type == xiiGameApplicationExecutionEvent::Type::BeginAppTick)
  {
  }
  else if (e.m_Type == xiiGameApplicationExecutionEvent::Type::AfterPresent)
  {
    // This tells the compositor we submitted the frames are done rendering to them this frame.
    vr::VRCompositor()->PostPresentHandoff();

    xiiGALDevice*     pDevice          = xiiGALDevice::GetDefaultDevice();
    xiiGALContext*    pGALContext      = pDevice->GetPrimaryContext();
    xiiRenderContext* m_pRenderContext = xiiRenderContext::GetDefaultInstance();

    if (const xiiGALTexture* tex = pDevice->GetTexture(m_hCompanionRenderTarget))
    {
      // We are rendering the companion window at the very start of the frame, using the content
      // of the last frame. That way we do not add additional delay before submitting the frames.
      XII_PROFILE_AND_MARKER(pGALContext, "VR CompanionView");

      m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, 1);
      m_pRenderContext->BindConstantBuffer("xiiVRCompanionViewConstants", m_hCompanionConstantBuffer);
      m_pRenderContext->BindShader(m_hCompanionShader);

      auto    hRenderTargetView = xiiGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_hCompanionRenderTarget);
      xiiVec2 targetSize        = xiiVec2((float)tex->GetDescription().m_uiWidth, (float)tex->GetDescription().m_uiHeight);

      xiiGALRenderTagetSetup renderTargetSetup;
      renderTargetSetup.SetRenderTarget(0, hRenderTargetView);
      pGALContext->SetRenderTargetSetup(renderTargetSetup);
      pGALContext->SetViewport(xiiRectFloat(targetSize.x, targetSize.y));

      auto* constants       = xiiRenderContext::GetConstantBufferData<xiiVRCompanionViewConstants>(m_hCompanionConstantBuffer);
      constants->TargetSize = targetSize;

      xiiGALResourceViewHandle hInputView = pDevice->GetDefaultResourceView(m_hColorRT);
      m_pRenderContext->BindTexture2D("VRTexture", hInputView);
      m_pRenderContext->DrawMeshBuffer();
    }
  }
}

void xiiOpenVR::GALDeviceEventHandler(const xiiGALDeviceEvent& e)
{
  if (e.m_Type == xiiGALDeviceEvent::Type::BeforeBeginFrame)
  {
    // Will call 'WaitGetPoses' which will block the thread. Alternatively we can
    // call 'PostPresentHandoff' but then we need to do more work ourselves.
    // According to the docu at 'PostPresentHandoff' both calls should happen
    // after present, the docu for 'WaitGetPoses' contradicts this :-/
    // This needs to happen on the render thread as OpenVR will use DX calls.
    UpdatePoses();

    // This will update the extracted view from last frame with the new data we got
    // this frame just before starting to render.
    xiiView* pView = nullptr;
    if (xiiRenderWorld::TryGetView(m_hView, pView))
    {
      pView->UpdateViewData(xiiRenderWorld::GetDataIndexForRendering());
    }
  }
}

void xiiOpenVR::OnBeginRender(xiiUInt64)
{
  // TODO: Ideally we would like to call UpdatePoses() here and block and in BeforeBeginFrame
  // we would predict the pose in two frames.
}

void xiiOpenVR::ReadHMDInfo()
{
  XII_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  m_Info.m_sDeviceName   = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
  m_Info.m_sDeviceDriver = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
  m_pHMD->GetRecommendedRenderTargetSize(&m_Info.m_vEyeRenderTargetSize.x, &m_Info.m_vEyeRenderTargetSize.y);
  m_Info.m_mat4eyePosLeft  = GetHMDEyePose(vr::Eye_Left);
  m_Info.m_mat4eyePosRight = GetHMDEyePose(vr::Eye_Right);
}


void xiiOpenVR::OnDeviceActivated(xiiVRDeviceID uiDeviceID)
{
  m_DeviceState[uiDeviceID].m_bDeviceIsConnected = true;
  switch (m_pHMD->GetTrackedDeviceClass(uiDeviceID))
  {
    case vr::TrackedDeviceClass_HMD:
      m_DeviceState[uiDeviceID].m_Type = xiiVRDeviceState::Type::HMD;
      break;
    case vr::TrackedDeviceClass_Controller:
      m_DeviceState[uiDeviceID].m_Type = xiiVRDeviceState::Type::Controller;

      break;
    case vr::TrackedDeviceClass_GenericTracker:
      m_DeviceState[uiDeviceID].m_Type = xiiVRDeviceState::Type::Tracker;
      break;
    case vr::TrackedDeviceClass_TrackingReference:
      m_DeviceState[uiDeviceID].m_Type = xiiVRDeviceState::Type::Reference;
      break;
    default:
      m_DeviceState[uiDeviceID].m_Type = xiiVRDeviceState::Type::Unknown;
      break;
  }

  UpdateHands();

  {
    xiiVRDeviceEvent e;
    e.m_Type     = xiiVRDeviceEvent::Type::DeviceAdded;
    e.uiDeviceID = uiDeviceID;
    m_DeviceEvents.Broadcast(e);
  }
}


void xiiOpenVR::OnDeviceDeactivated(xiiVRDeviceID uiDeviceID)
{
  m_DeviceState[uiDeviceID].m_bDeviceIsConnected = false;
  m_DeviceState[uiDeviceID].m_bPoseIsValid       = false;
  UpdateHands();
  {
    xiiVRDeviceEvent e;
    e.m_Type     = xiiVRDeviceEvent::Type::DeviceRemoved;
    e.uiDeviceID = uiDeviceID;
    m_DeviceEvents.Broadcast(e);
  }
}

void xiiOpenVR::UpdatePoses()
{
  XII_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  xiiStopwatch sw;

  UpdateHands();
  vr::TrackedDevicePose_t TrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
  vr::EVRCompositorError  err = vr::VRCompositor()->WaitGetPoses(TrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
  for (xiiVRDeviceID uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
  {
    m_DeviceState[uiDeviceID].m_bPoseIsValid = TrackedDevicePose[uiDeviceID].bPoseIsValid;
    if (TrackedDevicePose[uiDeviceID].bPoseIsValid)
    {
      m_DeviceState[uiDeviceID].m_vVelocity        = ConvertSteamVRVector(TrackedDevicePose[uiDeviceID].vVelocity);
      m_DeviceState[uiDeviceID].m_vAngularVelocity = ConvertSteamVRVector(TrackedDevicePose[uiDeviceID].vAngularVelocity);
      m_DeviceState[uiDeviceID].m_mPose            = ConvertSteamVRMatrix(TrackedDevicePose[uiDeviceID].mDeviceToAbsoluteTracking);
      m_DeviceState[uiDeviceID].m_vPosition        = m_DeviceState[uiDeviceID].m_mPose.GetTranslationVector();
      m_DeviceState[uiDeviceID].m_qRotation.SetFromMat3(m_DeviceState[uiDeviceID].m_mPose.GetRotationalPart());
    }
  }

  if (m_pCameraToSynchronize)
  {
    UpdateCamera();

    xiiMat4 viewMatrix;
    viewMatrix.SetLookAtMatrix(xiiVec3::ZeroVector(), xiiVec3(0, -1, 0), xiiVec3(0, 0, 1));

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
            auto stageSpace = pStage->GetStageSpace();
            if (m_StageSpace != stageSpace)
              SetStageSpace(pStage->GetStageSpace());
            add = pStage->GetOwner()->GetGlobalTransform();
          }
        }
      }
    }

    const xiiMat4 mAdd        = add.GetAsMat4();
    xiiMat4       mShiftedPos = m_DeviceState[0].m_mPose * mAdd;
    mShiftedPos.Invert();

    const xiiMat4 mViewTransformLeft  = viewMatrix * m_Info.m_mat4eyePosLeft * mShiftedPos;
    const xiiMat4 mViewTransformRight = viewMatrix * m_Info.m_mat4eyePosRight * mShiftedPos;

    m_VRCamera.SetViewMatrix(mViewTransformLeft, xiiCameraEye::Left);
    m_VRCamera.SetViewMatrix(mViewTransformRight, xiiCameraEye::Right);

    // put the camera orientation into the sound listener and enable the listener override mode
    if (xiiSoundInterface* pSoundInterface = xiiSingletonRegistry::GetSingletonInstance<xiiSoundInterface>("xiiSoundInterface"))
    {
      pSoundInterface->SetListener(
        -1, m_VRCamera.GetCenterPosition(), m_VRCamera.GetCenterDirForwards(), m_VRCamera.GetCenterDirUp(), xiiVec3::ZeroVector());
    }
  }
}

void xiiOpenVR::UpdateHands()
{
  m_iLeftControllerDeviceID  = m_pHMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
  m_iRightControllerDeviceID = m_pHMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
}

void xiiOpenVR::SetStageSpace(xiiVRStageSpace::Enum space)
{
  m_StageSpace = space;
  switch (space)
  {
    case xiiVRStageSpace::Seated:
      vr::VRCompositor()->SetTrackingSpace(vr::TrackingUniverseOrigin::TrackingUniverseSeated);
      break;
    case xiiVRStageSpace::Standing:
      vr::VRCompositor()->SetTrackingSpace(vr::TrackingUniverseOrigin::TrackingUniverseStanding);
      break;
  }
}

void xiiOpenVR::SetHMDCamera(xiiCamera* pCamera)
{
  XII_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  if (m_pCameraToSynchronize == pCamera)
    return;

  m_pCameraToSynchronize = pCamera;
  if (m_pCameraToSynchronize)
  {
    m_uiSettingsModificationCounter = m_pCameraToSynchronize->GetSettingsModificationCounter() + 1;
    m_VRCamera                      = *m_pCameraToSynchronize;
    m_VRCamera.SetCameraMode(xiiCameraMode::Stereo, 90.0f, m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
    UpdateCamera();
  }

  if (xiiSoundInterface* pSoundInterface = xiiSingletonRegistry::GetSingletonInstance<xiiSoundInterface>("xiiSoundInterface"))
  {
    pSoundInterface->SetListenerOverrideMode(m_pCameraToSynchronize != nullptr);
  }
}


void xiiOpenVR::UpdateCamera()
{
  if (m_uiSettingsModificationCounter != m_pCameraToSynchronize->GetSettingsModificationCounter())
  {
    const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.x / (float)m_Info.m_vEyeRenderTargetSize.y;
    xiiMat4     projLeft     = GetHMDProjectionEye(vr::Hmd_Eye::Eye_Left, m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
    xiiMat4     projRight    = GetHMDProjectionEye(vr::Hmd_Eye::Eye_Right, m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
    m_VRCamera.SetStereoProjection(projLeft, projRight, fAspectRatio);
  }
}

xiiMat4 xiiOpenVR::GetHMDProjectionEye(vr::Hmd_Eye nEye, float fNear, float fFar) const
{
  XII_ASSERT_DEV(m_pHMD, "Need to call 'Initialize' first.");

  float Left, Right, Top, Bottom;
  m_pHMD->GetProjectionRaw(nEye, &Left, &Right, &Top, &Bottom);
  xiiMat4 proj;
  proj.SetPerspectiveProjectionMatrix(Left * fNear, Right * fNear, Top * fNear, Bottom * fNear, fNear, fFar);
  return proj;
}

xiiMat4 xiiOpenVR::GetHMDEyePose(vr::Hmd_Eye nEye) const
{
  XII_ASSERT_DEV(m_pHMD, "Need to call 'Initialize' first.");

  vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
  xiiMat4           matrixObj   = ConvertSteamVRMatrix(matEyeRight);
  matrixObj.Invert();
  return matrixObj;
}

xiiString xiiOpenVR::GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError) const
{
  XII_ASSERT_DEV(m_pHMD, "Need to call 'Initialize' first.");

  const xiiUInt32           uiCharCount = m_pHMD->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
  xiiHybridArray<char, 128> temp;
  temp.SetCountUninitialized(uiCharCount);
  if (uiCharCount > 0)
  {
    m_pHMD->GetStringTrackedDeviceProperty(unDevice, prop, temp.GetData(), uiCharCount, peError);
  }
  else
  {
    temp.SetCount(1);
    temp[0] = 0;
  }
  return xiiString(temp.GetData());
}

xiiMat4 xiiOpenVR::ConvertSteamVRMatrix(const vr::HmdMatrix34_t& matPose)
{
  // clang-format off
  // Convert right handed to left handed with Y and Z swapped.
  // Same as A^t * matPose * A with A being identity with y and z swapped.
  xiiMat4 mMat(
    matPose.m[0][0], matPose.m[0][2], matPose.m[0][1], matPose.m[0][3],
    matPose.m[2][0], matPose.m[2][2], matPose.m[2][1], matPose.m[2][3],
    matPose.m[1][0], matPose.m[1][2], matPose.m[1][1], matPose.m[1][3],
    0, 0, 0, 1.0f);

  return mMat;
  // clang-format on
}

xiiVec3 xiiOpenVR::ConvertSteamVRVector(const vr::HmdVector3_t& vector)
{
  return xiiVec3(vector.v[0], vector.v[2], vector.v[1]);
}

XII_STATICLINK_FILE(OpenVRPlugin, OpenVRPlugin_OpenVRSingleton);
