#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/CameraComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Textures/Texture2DResource.h>


xiiCameraComponentManager::xiiCameraComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiCameraComponent, xiiBlockStorageType::Compact>(pWorld)
{
  xiiRenderWorld::s_CameraConfigsModifiedEvent.AddEventHandler(xiiMakeDelegate(&xiiCameraComponentManager::OnCameraConfigsChanged, this));
}

xiiCameraComponentManager::~xiiCameraComponentManager()
{
  xiiRenderWorld::s_CameraConfigsModifiedEvent.RemoveEventHandler(xiiMakeDelegate(&xiiCameraComponentManager::OnCameraConfigsChanged, this));
}

void xiiCameraComponentManager::Initialize()
{
  auto desc    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiCameraComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PostTransform;

  this->RegisterUpdateFunction(desc);

  xiiRenderWorld::s_ViewCreatedEvent.AddEventHandler(xiiMakeDelegate(&xiiCameraComponentManager::OnViewCreated, this));
}

void xiiCameraComponentManager::Deinitialize()
{
  xiiRenderWorld::s_ViewCreatedEvent.RemoveEventHandler(xiiMakeDelegate(&xiiCameraComponentManager::OnViewCreated, this));

  SUPER::Deinitialize();
}

void xiiCameraComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (auto hCameraComponent : m_ModifiedCameras)
  {
    xiiCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
    {
      continue;
    }

    if (xiiView* pView = xiiRenderWorld::GetViewByUsageHint(pCameraComponent->GetUsageHint(), xiiCameraUsageHint::None, GetWorld()))
    {
      pCameraComponent->ApplySettingsToView(pView);
    }

    pCameraComponent->m_bIsModified = false;
  }

  m_ModifiedCameras.Clear();

  for (auto hCameraComponent : m_RenderTargetCameras)
  {
    xiiCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
    {
      continue;
    }

    pCameraComponent->UpdateRenderTargetCamera();
  }

  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->m_bShowStats && it->GetUsageHint() == xiiCameraUsageHint::MainView)
    {
      if (xiiView* pView = xiiRenderWorld::GetViewByUsageHint(xiiCameraUsageHint::MainView, xiiCameraUsageHint::EditorView, GetWorld()))
      {
        it->ShowStats(pView);
      }
    }
  }
}

void xiiCameraComponentManager::ReinitializeAllRenderTargetCameras()
{
  XII_LOCK(GetWorld()->GetWriteMarker());

  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->DeactivateRenderToTexture();
      it->ActivateRenderToTexture();
    }
  }
}

const xiiCameraComponent* xiiCameraComponentManager::GetCameraByUsageHint(xiiCameraUsageHint::Enum usageHint) const
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUsageHint() == usageHint)
    {
      return it;
    }
  }

  return nullptr;
}

xiiCameraComponent* xiiCameraComponentManager::GetCameraByUsageHint(xiiCameraUsageHint::Enum usageHint)
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUsageHint() == usageHint)
    {
      return it;
    }
  }

  return nullptr;
}

void xiiCameraComponentManager::AddRenderTargetCamera(xiiCameraComponent* pComponent)
{
  m_RenderTargetCameras.PushBack(pComponent->GetHandle());
}

void xiiCameraComponentManager::RemoveRenderTargetCamera(xiiCameraComponent* pComponent)
{
  m_RenderTargetCameras.RemoveAndSwap(pComponent->GetHandle());
}

void xiiCameraComponentManager::OnViewCreated(xiiView* pView)
{
  // Mark all cameras as modified so the new view gets the proper settings
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    it->MarkAsModified(this);
  }
}

void xiiCameraComponentManager::OnCameraConfigsChanged(void* dummy)
{
  ReinitializeAllRenderTargetCameras();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiCameraComponent, 10, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("EditorShortcut", m_iEditorShortcut)->AddAttributes(new xiiDefaultValueAttribute(-1), new xiiClampValueAttribute(-1, 9)),
    XII_ENUM_ACCESSOR_PROPERTY("UsageHint", xiiCameraUsageHint, GetUsageHint, SetUsageHint),
    XII_ENUM_ACCESSOR_PROPERTY("Mode", xiiCameraMode, GetCameraMode, SetCameraMode),
    XII_ACCESSOR_PROPERTY("RenderTarget", GetRenderTargetFile, SetRenderTargetFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Target", xiiDependencyFlags::Package)),
    XII_ACCESSOR_PROPERTY("RenderTargetOffset", GetRenderTargetRectOffset, SetRenderTargetRectOffset)->AddAttributes(new xiiClampValueAttribute(xiiVec2(0.0f), xiiVec2(0.9f))),
    XII_ACCESSOR_PROPERTY("RenderTargetSize", GetRenderTargetRectSize, SetRenderTargetRectSize)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(1.0f)), new xiiClampValueAttribute(xiiVec2(0.1f), xiiVec2(1.0f))),
    XII_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new xiiDefaultValueAttribute(0.25f), new xiiClampValueAttribute(0.01f, 4.0f)),
    XII_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new xiiDefaultValueAttribute(1000.0f), new xiiClampValueAttribute(5.0, 10000.0f)),
    XII_ACCESSOR_PROPERTY("FOV", GetFieldOfView, SetFieldOfView)->AddAttributes(new xiiDefaultValueAttribute(60.0f), new xiiClampValueAttribute(1.0f, 170.0f)),
    XII_ACCESSOR_PROPERTY("Dimensions", GetOrthoDimension, SetOrthoDimension)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.01f, 10000.0f)),
    XII_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new xiiTagSetWidgetAttribute("Default")),
    XII_SET_MEMBER_PROPERTY("ExcludeTags", m_ExcludeTags)->AddAttributes(new xiiTagSetWidgetAttribute("Default")),
    XII_ACCESSOR_PROPERTY("CameraRenderPipeline", GetRenderPipelineEnum, SetRenderPipelineEnum)->AddAttributes(new xiiDynamicStringEnumAttribute("CameraPipelines")),
    XII_ACCESSOR_PROPERTY("Aperture", GetAperture, SetAperture)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(1.0f, 32.0f), new xiiSuffixAttribute(" f-stop(s)")),
    XII_ACCESSOR_PROPERTY("ShutterTime", GetShutterTime, SetShutterTime)->AddAttributes(new xiiDefaultValueAttribute(xiiTime::Seconds(1.0)), new xiiClampValueAttribute(xiiTime::Seconds(1.0f / 100000.0f), xiiTime::Seconds(600.0f))),
    XII_ACCESSOR_PROPERTY("ISO", GetISO, SetISO)->AddAttributes(new xiiDefaultValueAttribute(100.0f), new xiiClampValueAttribute(50.0f, 64000.0f)),
    XII_ACCESSOR_PROPERTY("ExposureCompensation", GetExposureCompensation, SetExposureCompensation)->AddAttributes(new xiiClampValueAttribute(-32.0f, 32.0f)),
    XII_MEMBER_PROPERTY("ShowStats", m_bShowStats),
    //XII_ACCESSOR_PROPERTY_READ_ONLY("EV100", GetEV100),
    //XII_ACCESSOR_PROPERTY_READ_ONLY("FinalExposure", GetExposure),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 1.0f, xiiColor::DarkSlateBlue),
    new xiiCameraVisualizerAttribute("Mode", "FOV", "Dimensions", "NearPlane", "FarPlane"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCameraComponent::xiiCameraComponent()  = default;
xiiCameraComponent::~xiiCameraComponent() = default;

void xiiCameraComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_UsageHint.GetValue();
  s << m_Mode.GetValue();
  s << m_fNearPlane;
  s << m_fFarPlane;
  s << m_fPerspectiveFieldOfView;
  s << m_fOrthoDimension;

  // Version 2 till 7
  // s << m_hRenderPipeline;

  // Version 3
  s << m_fAperture;
  s << static_cast<float>(m_ShutterTime.GetSeconds());
  s << m_fISO;
  s << m_fExposureCompensation;

  // Version 4
  m_IncludeTags.Save(s);
  m_ExcludeTags.Save(s);

  // Version 6
  s << m_hRenderTarget;

  // Version 7
  s << m_vRenderTargetRectOffset;
  s << m_vRenderTargetRectSize;

  // Version 8
  s << m_sRenderPipeline;

  // Version 10
  s << m_bShowStats;
}

void xiiCameraComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = inout_stream.GetStream();

  xiiCameraUsageHint::StorageType usage;
  s >> usage;
  if (uiVersion == 1 && usage > xiiCameraUsageHint::MainView)
    usage = xiiCameraUsageHint::None;
  m_UsageHint.SetValue(usage);

  xiiCameraMode::StorageType cam;
  s >> cam;
  m_Mode.SetValue(cam);

  s >> m_fNearPlane;
  s >> m_fFarPlane;
  s >> m_fPerspectiveFieldOfView;
  s >> m_fOrthoDimension;

  if (uiVersion >= 2 && uiVersion <= 7)
  {
    xiiRenderPipelineResourceHandle m_hRenderPipeline;
    s >> m_hRenderPipeline;
  }

  if (uiVersion >= 3)
  {
    s >> m_fAperture;
    float shutterTime;
    s >> shutterTime;
    m_ShutterTime = xiiTime::Seconds(shutterTime);
    s >> m_fISO;
    s >> m_fExposureCompensation;
  }

  if (uiVersion >= 4)
  {
    m_IncludeTags.Load(s, xiiTagRegistry::GetGlobalRegistry());
    m_ExcludeTags.Load(s, xiiTagRegistry::GetGlobalRegistry());
  }

  if (uiVersion >= 6)
  {
    s >> m_hRenderTarget;
  }

  if (uiVersion >= 7)
  {
    s >> m_vRenderTargetRectOffset;
    s >> m_vRenderTargetRectSize;
  }

  if (uiVersion >= 8)
  {
    s >> m_sRenderPipeline;
  }

  if (uiVersion >= 10)
  {
    s >> m_bShowStats;
  }

  MarkAsModified();
}

void xiiCameraComponent::UpdateRenderTargetCamera()
{
  if (!m_bRenderTargetInitialized)
    return;

  // recreate everything, if the view got invalidated in between
  if (m_hRenderTargetView.IsInvalidated())
  {
    DeactivateRenderToTexture();
    ActivateRenderToTexture();
  }

  xiiView* pView = nullptr;
  if (!xiiRenderWorld::TryGetView(m_hRenderTargetView, pView))
    return;

  ApplySettingsToView(pView);

  if (m_Mode == xiiCameraMode::PerspectiveFixedFovX || m_Mode == xiiCameraMode::PerspectiveFixedFovY)
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fPerspectiveFieldOfView, m_fNearPlane, m_fFarPlane);
  else
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fOrthoDimension, m_fNearPlane, m_fFarPlane);

  m_RenderTargetCamera.LookAt(
    GetOwner()->GetGlobalPosition(), GetOwner()->GetGlobalPosition() + GetOwner()->GetGlobalDirForwards(), GetOwner()->GetGlobalDirUp());
}

void xiiCameraComponent::ShowStats(xiiView* pView)
{
  if (!m_bShowStats)
    return;

  // draw stats
  {
    const xiiStringView sName = GetOwner()->GetName();

    xiiStringBuilder sb;
    sb.Format("Camera '{0}':\nEV100: {1}, Exposure: {2}", sName.IsEmpty() ? pView->GetName() : sName, GetEV100(), GetExposure());
    xiiDebugRenderer::DrawInfoText(pView->GetHandle(), xiiDebugTextPlacement::TopLeft, "CamStats", sb, xiiColor::White);
  }

  // draw frustum
  {
    const xiiGameObject* pOwner    = GetOwner();
    xiiVec3              vPosition = pOwner->GetGlobalPosition();
    xiiVec3              vForward  = pOwner->GetGlobalDirForwards();
    xiiVec3              vUp       = pOwner->GetGlobalDirUp();

    const xiiMat4 viewMatrix = xiiGraphicsUtils::CreateLookAtViewMatrix(vPosition, vPosition + vForward, vUp);

    xiiMat4 projectionMatrix     = pView->GetProjectionMatrix(xiiCameraEye::Left); // todo: Stereo support
    xiiMat4 viewProjectionMatrix = projectionMatrix * viewMatrix;

    xiiFrustum frustum;
    frustum.SetFrustum(viewProjectionMatrix);

    // TODO: limit far plane to 10 meters

    xiiDebugRenderer::DrawLineFrustum(GetWorld(), frustum, xiiColor::LimeGreen);
  }
}

void xiiCameraComponent::SetUsageHint(xiiEnum<xiiCameraUsageHint> val)
{
  if (val == m_UsageHint)
    return;

  DeactivateRenderToTexture();

  m_UsageHint = val;

  ActivateRenderToTexture();

  MarkAsModified();
}

void xiiCameraComponent::SetRenderTargetFile(const char* szFile)
{
  DeactivateRenderToTexture();

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hRenderTarget = xiiResourceManager::LoadResource<xiiRenderToTexture2DResource>(szFile);
  }
  else
  {
    m_hRenderTarget.Invalidate();
  }

  ActivateRenderToTexture();

  MarkAsModified();
}

const char* xiiCameraComponent::GetRenderTargetFile() const
{
  if (!m_hRenderTarget.IsValid())
    return "";

  return m_hRenderTarget.GetResourceID();
}

void xiiCameraComponent::SetRenderTargetRectOffset(xiiVec2 value)
{
  DeactivateRenderToTexture();

  m_vRenderTargetRectOffset.x = xiiMath::Clamp(value.x, 0.0f, 0.9f);
  m_vRenderTargetRectOffset.y = xiiMath::Clamp(value.y, 0.0f, 0.9f);

  ActivateRenderToTexture();
}

void xiiCameraComponent::SetRenderTargetRectSize(xiiVec2 value)
{
  DeactivateRenderToTexture();

  m_vRenderTargetRectSize.x = xiiMath::Clamp(value.x, 0.1f, 1.0f);
  m_vRenderTargetRectSize.y = xiiMath::Clamp(value.y, 0.1f, 1.0f);

  ActivateRenderToTexture();
}

void xiiCameraComponent::SetCameraMode(xiiEnum<xiiCameraMode> val)
{
  if (val == m_Mode)
    return;
  m_Mode = val;

  MarkAsModified();
}


void xiiCameraComponent::SetNearPlane(float fVal)
{
  if (fVal == m_fNearPlane)
    return;
  m_fNearPlane = fVal;

  MarkAsModified();
}


void xiiCameraComponent::SetFarPlane(float fVal)
{
  if (fVal == m_fFarPlane)
    return;
  m_fFarPlane = fVal;

  MarkAsModified();
}


void xiiCameraComponent::SetFieldOfView(float fVal)
{
  if (fVal == m_fPerspectiveFieldOfView)
    return;
  m_fPerspectiveFieldOfView = fVal;

  MarkAsModified();
}


void xiiCameraComponent::SetOrthoDimension(float fVal)
{
  if (fVal == m_fOrthoDimension)
    return;
  m_fOrthoDimension = fVal;

  MarkAsModified();
}

xiiRenderPipelineResourceHandle xiiCameraComponent::GetRenderPipeline() const
{
  return m_hCachedRenderPipeline;
}

xiiViewHandle xiiCameraComponent::GetRenderTargetView() const
{
  return m_hRenderTargetView;
}

const char* xiiCameraComponent::GetRenderPipelineEnum() const
{
  return m_sRenderPipeline.GetData();
}

void xiiCameraComponent::SetRenderPipelineEnum(const char* szFile)
{
  DeactivateRenderToTexture();

  m_sRenderPipeline.Assign(szFile);

  ActivateRenderToTexture();

  MarkAsModified();
}

void xiiCameraComponent::SetAperture(float fAperture)
{
  if (m_fAperture == fAperture)
    return;
  m_fAperture = fAperture;

  MarkAsModified();
}

void xiiCameraComponent::SetShutterTime(xiiTime shutterTime)
{
  if (m_ShutterTime == shutterTime)
    return;
  m_ShutterTime = shutterTime;

  MarkAsModified();
}

void xiiCameraComponent::SetISO(float fISO)
{
  if (m_fISO == fISO)
    return;
  m_fISO = fISO;

  MarkAsModified();
}

void xiiCameraComponent::SetExposureCompensation(float fEC)
{
  if (m_fExposureCompensation == fEC)
    return;
  m_fExposureCompensation = fEC;

  MarkAsModified();
}

float xiiCameraComponent::GetEV100() const
{
  // From: course_notes_moving_frostbite_to_pbr.pdf
  // EV number is defined as:
  // 2^ EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
  // This gives
  // EV_s = log2 (N^2 / t)
  // EV_100 + log2 (S /100) = log2 (N^2 / t)
  // EV_100 = log2 (N^2 / t) - log2 (S /100)
  // EV_100 = log2 (N^2 / t . 100 / S)
  return xiiMath::Log2((m_fAperture * m_fAperture) / m_ShutterTime.AsFloatInSeconds() * 100.0f / m_fISO) - m_fExposureCompensation;
}

float xiiCameraComponent::GetExposure() const
{
  // Compute the maximum luminance possible with H_sbs sensitivity
  // maxLum = 78 / ( S * q ) * N^2 / t
  // = 78 / ( S * q ) * 2^ EV_100
  // = 78 / (100 * 0.65) * 2^ EV_100
  // = 1.2 * 2^ EV
  // Reference : http://en.wikipedia.org/wiki/Film_speed
  float maxLuminance = 1.2f * xiiMath::Pow2(GetEV100());
  return 1.0f / maxLuminance;
}

void xiiCameraComponent::ApplySettingsToView(xiiView* pView) const
{
  if (m_UsageHint == xiiCameraUsageHint::None)
    return;

  float fFovOrDim = m_fPerspectiveFieldOfView;
  if (m_Mode == xiiCameraMode::OrthoFixedWidth || m_Mode == xiiCameraMode::OrthoFixedHeight)
  {
    fFovOrDim = m_fOrthoDimension;
  }

  xiiCamera* pCamera = pView->GetCamera();
  pCamera->SetCameraMode(m_Mode, fFovOrDim, m_fNearPlane, xiiMath::Max(m_fNearPlane + 0.00001f, m_fFarPlane));
  pCamera->SetExposure(GetExposure());

  pView->m_IncludeTags = m_IncludeTags;
  pView->m_ExcludeTags = m_ExcludeTags;

  const xiiTag& tagEditor = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
  pView->m_ExcludeTags.Set(tagEditor);

  if (m_hCachedRenderPipeline.IsValid())
  {
    pView->SetRenderPipelineResource(m_hCachedRenderPipeline);
  }
}

void xiiCameraComponent::ResourceChangeEventHandler(const xiiResourceEvent& e)
{
  switch (e.m_Type)
  {
    case xiiResourceEvent::Type::ResourceExists:
    case xiiResourceEvent::Type::ResourceCreated:
      return;

    case xiiResourceEvent::Type::ResourceDeleted:
    case xiiResourceEvent::Type::ResourceContentUnloading:
    case xiiResourceEvent::Type::ResourceContentUpdated:
      // triggers a recreation of the view
      xiiRenderWorld::DeleteView(m_hRenderTargetView);
      m_hRenderTargetView.Invalidate();
      break;

    default:
      break;
  }
}

void xiiCameraComponent::MarkAsModified()
{
  if (!m_bIsModified)
  {
    GetWorld()->GetComponentManager<xiiCameraComponentManager>()->m_ModifiedCameras.PushBack(GetHandle());
    m_bIsModified = true;
  }
}


void xiiCameraComponent::MarkAsModified(xiiCameraComponentManager* pCameraManager)
{
  if (!m_bIsModified)
  {
    pCameraManager->m_ModifiedCameras.PushBack(GetHandle());
    m_bIsModified = true;
  }
}

void xiiCameraComponent::ActivateRenderToTexture()
{
  if (m_UsageHint != xiiCameraUsageHint::RenderTarget)
    return;

  if (m_bRenderTargetInitialized || !m_hRenderTarget.IsValid() || m_sRenderPipeline.IsEmpty() || !IsActiveAndInitialized())
    return;

  xiiResourceLock<xiiRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pRenderTarget.GetAcquireResult() != xiiResourceAcquireResult::Final)
  {
    return;
  }

  // query the render pipeline to use
  if (const auto* pConfig = xiiRenderWorld::FindCameraConfig(m_sRenderPipeline))
  {
    m_hCachedRenderPipeline = pConfig->m_hRenderPipeline;
  }

  if (!m_hCachedRenderPipeline.IsValid())
    return;

  m_bRenderTargetInitialized = true;

  XII_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  xiiStringBuilder name;
  name.Format("Camera RT: {0}", GetOwner()->GetName());

  xiiView* pRenderTargetView = nullptr;
  m_hRenderTargetView        = xiiRenderWorld::CreateView(name, pRenderTargetView);

  pRenderTargetView->SetRenderPipelineResource(m_hCachedRenderPipeline);

  pRenderTargetView->SetWorld(GetWorld());
  pRenderTargetView->SetCamera(&m_RenderTargetCamera);

  pRenderTarget->m_ResourceEvents.AddEventHandler(xiiMakeDelegate(&xiiCameraComponent::ResourceChangeEventHandler, this));

  xiiGALRenderTargets renderTargets;
  renderTargets.m_hRTs[0] = pRenderTarget->GetGALTexture();
  pRenderTargetView->SetRenderTargets(renderTargets);

  const float maxSizeX = 1.0f - m_vRenderTargetRectOffset.x;
  const float maxSizeY = 1.0f - m_vRenderTargetRectOffset.y;

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  const float width  = resX * xiiMath::Min(maxSizeX, m_vRenderTargetRectSize.x);
  const float height = resY * xiiMath::Min(maxSizeY, m_vRenderTargetRectSize.y);

  const float offsetX = m_vRenderTargetRectOffset.x * resX;
  const float offsetY = m_vRenderTargetRectOffset.y * resY;

  pRenderTargetView->SetViewport(xiiRectFloat(offsetX, offsetY, width, height));

  pRenderTarget->AddRenderView(m_hRenderTargetView);

  GetWorld()->GetComponentManager<xiiCameraComponentManager>()->AddRenderTargetCamera(this);
}

void xiiCameraComponent::DeactivateRenderToTexture()
{
  if (!m_bRenderTargetInitialized)
    return;

  m_bRenderTargetInitialized = false;
  m_hCachedRenderPipeline.Invalidate();

  XII_ASSERT_DEBUG(m_hRenderTarget.IsValid(), "Render Target should be valid");

  if (m_hRenderTarget.IsValid())
  {
    xiiResourceLock<xiiRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, xiiResourceAcquireMode::BlockTillLoaded);
    pRenderTarget->RemoveRenderView(m_hRenderTargetView);

    pRenderTarget->m_ResourceEvents.RemoveEventHandler(xiiMakeDelegate(&xiiCameraComponent::ResourceChangeEventHandler, this));
  }

  if (!m_hRenderTargetView.IsInvalidated())
  {
    xiiRenderWorld::DeleteView(m_hRenderTargetView);
    m_hRenderTargetView.Invalidate();
  }

  GetWorld()->GetComponentManager<xiiCameraComponentManager>()->RemoveRenderTargetCamera(this);
}

void xiiCameraComponent::OnActivated()
{
  SUPER::OnActivated();

  ActivateRenderToTexture();
}

void xiiCameraComponent::OnDeactivated()
{
  DeactivateRenderToTexture();

  SUPER::OnDeactivated();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class xiiCameraComponentPatch_4_5 : public xiiGraphPatch
{
public:
  xiiCameraComponentPatch_4_5() :
    xiiGraphPatch("xiiCameraComponent", 5)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Usage Hint", "UsageHint");
    pNode->RenameProperty("Near Plane", "NearPlane");
    pNode->RenameProperty("Far Plane", "FarPlane");
    pNode->RenameProperty("Include Tags", "IncludeTags");
    pNode->RenameProperty("Exclude Tags", "ExcludeTags");
    pNode->RenameProperty("Render Pipeline", "RenderPipeline");
    pNode->RenameProperty("Shutter Time", "ShutterTime");
    pNode->RenameProperty("Exposure Compensation", "ExposureCompensation");
  }
};

xiiCameraComponentPatch_4_5 g_xiiCameraComponentPatch_4_5;

//////////////////////////////////////////////////////////////////////////

class xiiCameraComponentPatch_8_9 : public xiiGraphPatch
{
public:
  xiiCameraComponentPatch_8_9() :
    xiiGraphPatch("xiiCameraComponent", 9)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    // convert the "ShutterTime" property from float to xiiTime
    if (auto pProp = pNode->FindProperty("ShutterTime"))
    {
      if (pProp->m_Value.IsA<float>())
      {
        const float shutterTime = pProp->m_Value.Get<float>();
        pProp->m_Value          = xiiTime::Seconds(shutterTime);
      }
    }
  }
};

xiiCameraComponentPatch_8_9 g_xiiCameraComponentPatch_8_9;


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_CameraComponent);
