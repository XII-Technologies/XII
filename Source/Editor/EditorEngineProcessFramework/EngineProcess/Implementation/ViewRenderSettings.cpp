/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSceneViewPerspective, 1)
  XII_ENUM_CONSTANTS(xiiSceneViewPerspective::Orthogonal_Front, xiiSceneViewPerspective::Orthogonal_Right, xiiSceneViewPerspective::Orthogonal_Top,
    xiiSceneViewPerspective::Perspective)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEngineViewLightSettings, 1, xiiRTTIDefaultAllocator<xiiEngineViewLightSettings>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight),
      XII_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle),
      XII_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
      XII_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity)
    }
    XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiEngineViewConfig::ApplyPerspectiveSetting(float fFov, float fNearPlane, float fFarPlane)
{
  const float fOrthoRange = 1000.0f;

  switch (m_Perspective)
  {
    case xiiSceneViewPerspective::Perspective:
    {
      m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, fFov == 0.0f ? 70.0f : fFov, fNearPlane, fFarPlane);
    }
    break;

    case xiiSceneViewPerspective::Orthogonal_Front:
    {
      m_Camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, fFov == 0.0f ? 20.0f : fFov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + xiiVec3(-1, 0, 0), xiiVec3(0, 0, 1));
    }
    break;

    case xiiSceneViewPerspective::Orthogonal_Right:
    {
      m_Camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, fFov == 0.0f ? 20.0f : fFov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + xiiVec3(0, -1, 0), xiiVec3(0, 0, 1));
    }
    break;

    case xiiSceneViewPerspective::Orthogonal_Top:
    {
      m_Camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, fFov == 0.0f ? 20.0f : fFov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + xiiVec3(0, 0, -1), xiiVec3(1, 0, 0));
    }
    break;
  }
}

xiiEngineViewLightSettings::xiiEngineViewLightSettings(bool bEnable)
{
  if (!bEnable)
  {
    m_bDirectionalLight = false;
  }
}

xiiEngineViewLightSettings::~xiiEngineViewLightSettings()
{
  if (m_hGameObject.IsInvalidated())
    return;

  XII_LOCK(m_pWorld->GetWriteMarker());
  m_pWorld->DeleteObjectNow(m_hGameObject);
}

bool xiiEngineViewLightSettings::GetDirectionalLight() const
{
  return m_bDirectionalLight;
}

void xiiEngineViewLightSettings::SetDirectionalLight(bool bVal)
{
  m_bDirectionalLight = bVal;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::DirectionalLightChanged);
}

xiiAngle xiiEngineViewLightSettings::GetDirectionalLightAngle() const
{
  return m_DirectionalLightAngle;
}

void xiiEngineViewLightSettings::SetDirectionalLightAngle(xiiAngle val)
{
  m_DirectionalLightAngle = val;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged);
}

bool xiiEngineViewLightSettings::GetDirectionalLightShadows() const
{
  return m_bDirectionalLightShadows;
}

void xiiEngineViewLightSettings::SetDirectionalLightShadows(bool bVal)
{
  m_bDirectionalLightShadows = bVal;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged);
}

float xiiEngineViewLightSettings::GetDirectionalLightIntensity() const
{
  return m_fDirectionalLightIntensity;
}

void xiiEngineViewLightSettings::SetDirectionalLightIntensity(float fVal)
{
  m_fDirectionalLightIntensity = fVal;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged);
}

bool xiiEngineViewLightSettings::SetupForEngine(xiiWorld* pWorld, xiiUInt32 uiNextComponentPickingID)
{
  m_pWorld = pWorld;
  UpdateForEngine(pWorld);
  return false;
}

void xiiEngineViewLightSettings::UpdateForEngine(xiiWorld* pWorld)
{
  XII_ASSERT_DEV(pWorld == m_pWorld, "Preview lighting cannot move between worlds.");
  XII_LOCK(pWorld->GetWriteMarker());
  if (!m_bDirectionalLight)
  {
    pWorld->DeleteObjectNow(m_hGameObject);
    m_hGameObject.Invalidate();
    m_hDirLight.Invalidate();
    return;
  }

  xiiGameObject* pObject = nullptr;
  if (!pWorld->TryGetObject(m_hGameObject, pObject))
  {
    xiiGameObjectDescription description;
    description.m_sName.Assign("ViewLightSettings");
    description.m_bDynamic = true;
    m_hGameObject          = pWorld->CreateObject(description, pObject);
  }

  const xiiQuat rotY = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0.0f, 1.0f, 0.0f), xiiAngle::MakeFromDegree(120.0));
  const xiiQuat rotZ = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0.0f, 0.0f, 1.0f), m_DirectionalLightAngle);
  pObject->SetLocalRotation(rotZ * rotY);

  xiiDirectionalLightComponent* pLight = nullptr;
  if (!pWorld->TryGetComponent(m_hDirLight, pLight))
    m_hDirLight = xiiDirectionalLightComponent::CreateComponent(pObject, pLight);
  pLight->SetCastShadows(m_bDirectionalLightShadows);
  pLight->SetIntensity(m_fDirectionalLightIntensity);
}

void xiiEngineViewLightSettings::SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type type)
{
  SetModified();

  xiiEngineViewLightSettingsEvent e;
  e.m_Type = type;
  m_EngineViewLightSettingsEvents.Broadcast(e);
}
