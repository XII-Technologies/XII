/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

xiiCameraComponentManager::xiiCameraComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiCameraComponent, xiiBlockStorageType::Compact>(pWorld)
{
}

xiiCameraComponentManager::~xiiCameraComponentManager() = default;

void xiiCameraComponentManager::Initialize()
{
  {
    auto description    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiCameraComponentManager::Update, this);
    description.m_Phase = xiiWorldUpdatePhase::PostTransform;

    this->RegisterUpdateFunction(description);
  }

  auto pRenderWorldModule = GetWorld()->GetOrCreateModule<xiiRenderWorldModule>();

  pRenderWorldModule->GetViewEvents().AddEventHandler(xiiMakeDelegate(&xiiCameraComponentManager::OnViewCreated, this));
}

void xiiCameraComponentManager::Deinitialize()
{
  auto pRenderWorldModule = GetWorld()->GetOrCreateModule<xiiRenderWorldModule>();

  pRenderWorldModule->GetViewEvents().RemoveEventHandler(xiiMakeDelegate(&xiiCameraComponentManager::OnViewCreated, this));

  SUPER::Deinitialize();
}

void xiiCameraComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  auto pRenderWorldModule = GetWorld()->GetOrCreateModule<xiiRenderWorldModule>();

  for (xiiComponentHandle hCameraComponent : m_ModifiedCameras)
  {
    xiiCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
      continue;

    if (xiiView* pView = pRenderWorldModule->GetViewByUsageHint(pCameraComponent->GetUsageHint(), xiiCameraUsageHint::None))
    {
      pCameraComponent->ApplySettingsToView(pView);
    }
    pCameraComponent->m_bIsModified = false;
  }

  m_ModifiedCameras.Clear();

  for (xiiComponentHandle hCameraComponent : m_RenderTargetCameras)
  {
    xiiCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
      continue;

    pCameraComponent->UpdateRenderTargetCamera();
  }

  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->m_bShowStats && it->GetUsageHint() == xiiCameraUsageHint::MainView)
    {
      if (xiiView* pView = pRenderWorldModule->GetViewByUsageHint(xiiCameraUsageHint::MainView, xiiCameraUsageHint::EditorView))
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

void xiiCameraComponentManager::OnViewCreated(const xiiViewEvent& viewEvent)
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
XII_BEGIN_COMPONENT_TYPE(xiiCameraComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("EditorShortcut", m_iEditorShortcut)->AddAttributes(new xiiDefaultValueAttribute(-1), new xiiClampValueAttribute(-1, 9)),
    XII_ENUM_ACCESSOR_PROPERTY("UsageHint", xiiCameraUsageHint, GetUsageHint, SetUsageHint),
    XII_ENUM_ACCESSOR_PROPERTY("Mode", xiiCameraMode, GetCameraMode, SetCameraMode),
    XII_ACCESSOR_PROPERTY("RenderTargetOffset", GetRenderTargetRectOffset, SetRenderTargetRectOffset)->AddAttributes(new xiiClampValueAttribute(xiiVec2(0.0f), xiiVec2(0.9f))),
    XII_ACCESSOR_PROPERTY("RenderTargetSize", GetRenderTargetRectSize, SetRenderTargetRectSize)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(1.0f)), new xiiClampValueAttribute(xiiVec2(0.1f), xiiVec2(1.0f))),
    XII_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new xiiDefaultValueAttribute(0.25f), new xiiClampValueAttribute(0.01f, 4.0f)),
    XII_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new xiiDefaultValueAttribute(1000.0f), new xiiClampValueAttribute(5.0, 10000.0f)),
    XII_ACCESSOR_PROPERTY("FOV", GetFieldOfView, SetFieldOfView)->AddAttributes(new xiiDefaultValueAttribute(60.0f), new xiiClampValueAttribute(1.0f, 170.0f)),
    XII_ACCESSOR_PROPERTY("Dimensions", GetOrthoDimension, SetOrthoDimension)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.01f, 10000.0f)),
    XII_ACCESSOR_PROPERTY("RenderScale", GetRenderScale, SetRenderScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.1f, 1.0f)),
    XII_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new xiiTagSetWidgetAttribute("Default")),
    XII_SET_MEMBER_PROPERTY("ExcludeTags", m_ExcludeTags)->AddAttributes(new xiiTagSetWidgetAttribute("Default")),
    XII_ACCESSOR_PROPERTY("Aperture", GetAperture, SetAperture)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(1.0f, 32.0f), new xiiSuffixAttribute(" f-stop(s)")),
    XII_ACCESSOR_PROPERTY("ShutterTime", GetShutterTime, SetShutterTime)->AddAttributes(new xiiDefaultValueAttribute(xiiTime::MakeFromSeconds(1.0)), new xiiClampValueAttribute(xiiTime::MakeFromSeconds(1.0f / 100000.0f), xiiTime::MakeFromSeconds(600.0f))),
    XII_ACCESSOR_PROPERTY("ISO", GetISO, SetISO)->AddAttributes(new xiiDefaultValueAttribute(100.0f), new xiiClampValueAttribute(50.0f, 64000.0f)),
    XII_ACCESSOR_PROPERTY("ExposureCompensation", GetExposureCompensation, SetExposureCompensation)->AddAttributes(new xiiClampValueAttribute(-32.0f, 32.0f)),
    XII_MEMBER_PROPERTY("ShowStats", m_bShowStats),
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

  s << m_UsageHint;
  s << m_Mode;
  s << m_fNearPlane;
  s << m_fFarPlane;
  s << m_fPerspectiveFieldOfView;
  s << m_fOrthoDimension;
  s << m_fAperture;
  s << static_cast<float>(m_ShutterTime.GetSeconds());
  s << m_fISO;
  s << m_fExposureCompensation;
  s << m_fRenderScale;

  m_IncludeTags.Save(s);
  m_ExcludeTags.Save(s);

  s << m_vRenderTargetRectOffset;
  s << m_vRenderTargetRectSize;
  s << m_bShowStats;
}

void xiiCameraComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_UsageHint;
  s >> m_Mode;
  s >> m_fNearPlane;
  s >> m_fFarPlane;
  s >> m_fPerspectiveFieldOfView;
  s >> m_fOrthoDimension;
  s >> m_fAperture;
  s >> m_ShutterTime;
  s >> m_fISO;
  s >> m_fExposureCompensation;
  s >> m_fRenderScale;

  m_IncludeTags.Load(s, xiiTagRegistry::GetGlobalRegistry());
  m_ExcludeTags.Load(s, xiiTagRegistry::GetGlobalRegistry());

  s >> m_vRenderTargetRectOffset;
  s >> m_vRenderTargetRectSize;
  s >> m_bShowStats;

  MarkAsModified();
}

void xiiCameraComponent::UpdateRenderTargetCamera()
{
  if (!m_bRenderTargetInitialized)
    return;

  // Recreate everything, if the view got invalidated in between.
  if (m_hRenderTargetView.IsInvalidated())
  {
    DeactivateRenderToTexture();
    ActivateRenderToTexture();
  }

  xiiView* pView = nullptr;
  if (!GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->TryGetView(m_hRenderTargetView, pView))
    return;

  ApplySettingsToView(pView);

  if (m_Mode == xiiCameraMode::PerspectiveFixedFovX || m_Mode == xiiCameraMode::PerspectiveFixedFovY)
  {
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fPerspectiveFieldOfView, m_fNearPlane, m_fFarPlane);
  }
  else
  {
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fOrthoDimension, m_fNearPlane, m_fFarPlane);
  }
  m_RenderTargetCamera.LookAt(GetOwner()->GetGlobalPosition(), GetOwner()->GetGlobalPosition() + GetOwner()->GetGlobalDirForwards(), GetOwner()->GetGlobalDirUp());
}

void xiiCameraComponent::ShowStats(xiiView* pView)
{
  if (!m_bShowStats)
    return;

  // \todo Draw stereo frustum and display stats with the debug renderer.
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

void xiiCameraComponent::SetRenderScale(float fVal)
{
  const float fClamped = xiiMath::Clamp(fVal, 0.1f, 1.0f);
  if (fClamped == m_fRenderScale)
    return;

  m_fRenderScale = fClamped;

  MarkAsModified();
}

xiiViewHandle xiiCameraComponent::GetRenderTargetView() const
{
  return m_hRenderTargetView;
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
  pView->SetRenderScale(m_fRenderScale);

  const xiiTag& tagEditor = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
  pView->m_ExcludeTags.Set(tagEditor);
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
    {
      // Triggers a recreation of the view
      GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->DestroyView(m_hRenderTargetView);
      m_hRenderTargetView.Invalidate();
      break;
    }
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

  if (m_bRenderTargetInitialized || !IsActiveAndInitialized())
    return;

#if 0
  m_bRenderTargetInitialized = true;

  XII_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiStringBuilder sName;
  sName.SetFormat("Camera RT: {0}", GetOwner()->GetName());

  xiiView* pView      = nullptr;
  m_hRenderTargetView = GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView(sName, pView);

  pView->SetCamera(&m_RenderTargetCamera);
  pView->SetRenderTargetView(pRenderTarget->GetGALTexture()->GetDefaultView(xiiGALTextureViewType::RenderTarget));

  const float maxSizeX = 1.0f - m_vRenderTargetRectOffset.x;
  const float maxSizeY = 1.0f - m_vRenderTargetRectOffset.y;

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  const float width  = resX * xiiMath::Min(maxSizeX, m_vRenderTargetRectSize.x);
  const float height = resY * xiiMath::Min(maxSizeY, m_vRenderTargetRectSize.y);

  const float offsetX = m_vRenderTargetRectOffset.x * resX;
  const float offsetY = m_vRenderTargetRectOffset.y * resY;

  pView->SetViewport(xiiRectFloat(offsetX, offsetY, width, height));
#endif

  GetWorld()->GetComponentManager<xiiCameraComponentManager>()->AddRenderTargetCamera(this);
}

void xiiCameraComponent::DeactivateRenderToTexture()
{
  if (!m_bRenderTargetInitialized)
    return;

  m_bRenderTargetInitialized = false;

  if (!m_hRenderTargetView.IsInvalidated())
  {
    GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->DestroyView(m_hRenderTargetView);
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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_CameraComponent);
