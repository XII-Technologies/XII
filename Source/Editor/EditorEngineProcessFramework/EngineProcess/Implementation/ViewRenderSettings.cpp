#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/SkyLightComponent.h>

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

void xiiEngineViewConfig::ApplyPerspectiveSetting(float fov, float nearPlane, float farPlane)
{
  const float fOrthoRange = 1000.0f;

  switch (m_Perspective)
  {
    case xiiSceneViewPerspective::Perspective:
    {
      m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, fov == 0.0f ? 70.0f : fov, nearPlane, farPlane);
    }
    break;

    case xiiSceneViewPerspective::Orthogonal_Front:
    {
      m_Camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + xiiVec3(-1, 0, 0), xiiVec3(0, 0, 1));
    }
    break;

    case xiiSceneViewPerspective::Orthogonal_Right:
    {
      m_Camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + xiiVec3(0, -1, 0), xiiVec3(0, 0, 1));
    }
    break;

    case xiiSceneViewPerspective::Orthogonal_Top:
    {
      m_Camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
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

void xiiEngineViewLightSettings::SetSkyBox(bool val)
{
  m_bSkyBox = val;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::SkyBoxChanged);
}

bool xiiEngineViewLightSettings::GetSkyLight() const
{
  return m_bSkyLight;
}

void xiiEngineViewLightSettings::SetSkyLight(bool val)
{
  m_bSkyLight = val;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::SkyLightChanged);
}

const char* xiiEngineViewLightSettings::GetSkyLightCubeMap() const
{
  return m_sSkyLightCubeMap;
}

void xiiEngineViewLightSettings::SetSkyLightCubeMap(const char* val)
{
  m_sSkyLightCubeMap = val;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged);
}

float xiiEngineViewLightSettings::GetSkyLightIntensity() const
{
  return m_fSkyLightIntensity;
}

void xiiEngineViewLightSettings::SetSkyLightIntensity(float val)
{
  m_fSkyLightIntensity = val;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged);
}

bool xiiEngineViewLightSettings::GetDirectionalLight() const
{
  return m_bDirectionalLight;
}

void xiiEngineViewLightSettings::SetDirectionalLight(bool val)
{
  m_bDirectionalLight = val;
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

void xiiEngineViewLightSettings::SetDirectionalLightShadows(bool val)
{
  m_bDirectionalLightShadows = val;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged);
}

float xiiEngineViewLightSettings::GetDirectionalLightIntensity() const
{
  return m_fDirectionalLightIntensity;
}

void xiiEngineViewLightSettings::SetDirectionalLightIntensity(float val)
{
  m_fDirectionalLightIntensity = val;
  SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged);
}

bool xiiEngineViewLightSettings::GetFog() const
{
  return m_bFog;
}

void xiiEngineViewLightSettings::SetFog(bool val)
{
  m_bFog = val;
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
  T* SyncComponent(xiiWorld* pWorld, xiiGameObject* pParent, xiiComponentHandle& handle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      T* pComp = nullptr;
      if (handle.IsInvalidated() || !pWorld->TryGetComponent(handle, pComp))
      {
        handle = T::CreateComponent(pParent, pComp);
      }
      return pComp;
    }
    else
    {
      if (!handle.IsInvalidated())
      {
        T* pComp = nullptr;
        if (pWorld->TryGetComponent(handle, pComp))
        {
          pComp->DeleteComponent();
          handle.Invalidate();
        }
      }
      return nullptr;
    }
  }

  xiiGameObject* SyncGameObject(xiiWorld* pWorld, xiiGameObjectHandle& handle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      xiiGameObject* pObj = nullptr;
      if (handle.IsInvalidated() || !pWorld->TryGetObject(handle, pObj))
      {
        xiiGameObjectDesc obj;
        obj.m_sName.Assign("ViewLightSettings");
        handle = pWorld->CreateObject(obj, pObj);
        pObj->MakeDynamic();
      }
      return pObj;
    }
    else
    {
      if (!handle.IsInvalidated())
      {
        pWorld->DeleteObjectDelayed(handle);
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

  const bool bNeedGameObject = m_bDirectionalLight | m_bSkyLight;
  if (xiiGameObject* pParent = SyncGameObject(m_pWorld, m_hGameObject, bNeedGameObject))
  {
    xiiQuat rot;
    rot.SetFromAxisAndAngle(xiiVec3(0.0f, 1.0f, 0.0f), m_DirectionalLightAngle + xiiAngle::Degree(90.0));
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
