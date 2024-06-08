#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <GraphicsCore/Components/FogComponent.h>
#include <GraphicsCore/Components/SkyBoxComponent.h>
#include <GraphicsCore/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Lights/SkyLightComponent.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSceneViewPerspective, 1)
  XII_ENUM_CONSTANTS(xiiSceneViewPerspective::Orthogonal_Front, xiiSceneViewPerspective::Orthogonal_Right, xiiSceneViewPerspective::Orthogonal_Top,
    xiiSceneViewPerspective::Perspective)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEngineViewLightSettings, 1, xiiRTTIDefaultAllocator<xiiEngineViewLightSettings>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("SkyBox", m_bSkyBox),
      XII_MEMBER_PROPERTY("SkyLight", m_bSkyLight),
      XII_MEMBER_PROPERTY("SkyLightCubeMap", m_sSkyLightCubeMap),
      XII_MEMBER_PROPERTY("SkyLightIntensity", m_fSkyLightIntensity),
      XII_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight),
      XII_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle),
      XII_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
      XII_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity),
      XII_MEMBER_PROPERTY("Fog", m_bFog)
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
    m_bSkyBox           = false;
    m_bSkyLight         = false;
    m_bDirectionalLight = false;
    m_bFog              = false;
  }
}

xiiEngineViewLightSettings::~xiiEngineViewLightSettings()
{
  if (m_hGameObject.IsInvalidated())
    return;

  m_pWorld->DeleteObjectDelayed(m_hGameObject);
}

bool xiiEngineViewLightSettings::GetSkyBox() const
{
  return m_bSkyBox;
}

void xiiEngineViewLightSettings::SetSkyBox(bool bVal)
{
  m_bSkyBox = bVal;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::SkyBoxChanged);
}

bool xiiEngineViewLightSettings::GetSkyLight() const
{
  return m_bSkyLight;
}

void xiiEngineViewLightSettings::SetSkyLight(bool bVal)
{
  m_bSkyLight = bVal;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::SkyLightChanged);
}

const char* xiiEngineViewLightSettings::GetSkyLightCubeMap() const
{
  return m_sSkyLightCubeMap;
}

void xiiEngineViewLightSettings::SetSkyLightCubeMap(const char* szVal)
{
  m_sSkyLightCubeMap = szVal;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged);
}

float xiiEngineViewLightSettings::GetSkyLightIntensity() const
{
  return m_fSkyLightIntensity;
}

void xiiEngineViewLightSettings::SetSkyLightIntensity(float fVal)
{
  m_fSkyLightIntensity = fVal;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged);
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

bool xiiEngineViewLightSettings::GetFog() const
{
  return m_bFog;
}

void xiiEngineViewLightSettings::SetFog(bool bVal)
{
  m_bFog = bVal;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::FogChanged);
}

bool xiiEngineViewLightSettings::SetupForEngine(xiiWorld* pWorld, xiiUInt32 uiNextComponentPickingID)
{
  m_pWorld = pWorld;
  UpdateForEngine(pWorld);
  return false;
}

namespace
{
  template <typename T>
  T* SyncComponent(xiiWorld* pWorld, xiiGameObject* pParent, xiiComponentHandle& inout_hHandle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      T* pComp = nullptr;
      if (inout_hHandle.IsInvalidated() || !pWorld->TryGetComponent(inout_hHandle, pComp))
      {
        inout_hHandle = T::CreateComponent(pParent, pComp);
      }
      return pComp;
    }
    else
    {
      if (!inout_hHandle.IsInvalidated())
      {
        T* pComp = nullptr;
        if (pWorld->TryGetComponent(inout_hHandle, pComp))
        {
          pComp->DeleteComponent();
          inout_hHandle.Invalidate();
        }
      }
      return nullptr;
    }
  }

  xiiGameObject* SyncGameObject(xiiWorld* pWorld, xiiGameObjectHandle& inout_hHandle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      xiiGameObject* pObj = nullptr;
      if (inout_hHandle.IsInvalidated() || !pWorld->TryGetObject(inout_hHandle, pObj))
      {
        xiiGameObjectDesc obj;
        obj.m_sName.Assign("ViewLightSettings");
        inout_hHandle = pWorld->CreateObject(obj, pObj);
        pObj->MakeDynamic();
      }
      return pObj;
    }
    else
    {
      if (!inout_hHandle.IsInvalidated())
      {
        pWorld->DeleteObjectDelayed(inout_hHandle);
      }
      return nullptr;
    }
  }
} // namespace

void xiiEngineViewLightSettings::UpdateForEngine(xiiWorld* pWorld)
{
  if (xiiGameObject* pParent = SyncGameObject(m_pWorld, m_hSkyBoxObject, m_bSkyBox))
  {
    pParent->SetTag(xiiTagRegistry::GetGlobalRegistry().RegisterTag("SkyLight"));

    if (xiiSkyBoxComponent* pSkyBox = SyncComponent<xiiSkyBoxComponent>(m_pWorld, pParent, m_hSkyBox, m_bSkyBox))
    {
      pSkyBox->SetCubeMapFile(m_sSkyLightCubeMap);
    }
  }

  const bool bNeedGameObject = m_bDirectionalLight || m_bSkyLight;
  if (xiiGameObject* pParent = SyncGameObject(m_pWorld, m_hGameObject, bNeedGameObject))
  {
    xiiQuat rot;
    rot.SetFromAxisAndAngle(xiiVec3(0.0f, 1.0f, 0.0f), m_DirectionalLightAngle + xiiAngle::MakeFromDegree(90.0));
    pParent->SetLocalRotation(rot);

    if (xiiDirectionalLightComponent* pDirLight = SyncComponent<xiiDirectionalLightComponent>(m_pWorld, pParent, m_hDirLight, m_bDirectionalLight))
    {
      pDirLight->SetCastShadows(m_bDirectionalLightShadows);
      pDirLight->SetIntensity(m_fDirectionalLightIntensity);
    }

    if (xiiSkyLightComponent* pSkyLight = SyncComponent<xiiSkyLightComponent>(m_pWorld, pParent, m_hSkyLight, m_bSkyLight))
    {
      pSkyLight->SetIntensity(m_fSkyLightIntensity);
      pSkyLight->SetReflectionProbeMode(xiiReflectionProbeMode::Static);
      pSkyLight->SetCubeMapFile(m_sSkyLightCubeMap);
    }

    if (xiiFogComponent* pFog = SyncComponent<xiiFogComponent>(m_pWorld, pParent, m_hFog, m_bFog))
    {
      pFog->SetColor(xiiColor(0.02f, 0.02f, 0.02f));
      pFog->SetDensity(5.0f);
      pFog->SetHeightFalloff(0);
    }
  }
}

void xiiEngineViewLightSettings::SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type type)
{
  SetModified();
  xiiEngineViewLightSettingsEvent e;
  e.m_Type = type;
  m_EngineViewLightSettingsEvents.Broadcast(e);
}
