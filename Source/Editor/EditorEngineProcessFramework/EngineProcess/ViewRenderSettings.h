#pragma once

#include <Core/Graphics/Camera.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>

struct XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiSceneViewPerspective
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Orthogonal_Front,
    Orthogonal_Right,
    Orthogonal_Top,
    Perspective,

    Default = Perspective
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORENGINEPROCESSFRAMEWORK_DLL, xiiSceneViewPerspective);

struct XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEngineViewConfig
{
  xiiEngineViewConfig()
  {
    m_RenderMode        = xiiViewRenderMode::Default;
    m_Perspective       = xiiSceneViewPerspective::Default;
    m_CameraUsageHint   = xiiCameraUsageHint::EditorView;
    m_pLinkedViewConfig = nullptr;
  }

  xiiViewRenderMode::Enum       m_RenderMode;
  xiiSceneViewPerspective::Enum m_Perspective;
  xiiCameraUsageHint::Enum      m_CameraUsageHint;
  bool                          m_bUseCameraTransformOnDevice = true;

  xiiCamera            m_Camera;
  xiiEngineViewConfig* m_pLinkedViewConfig; // used to store which other view config this is linked to, for resetting values when switching views

  void ApplyPerspectiveSetting(float fov = 0.0f, float nearPlane = 0.1f, float farPlane = 1000.0f);
};
struct XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEngineViewLightSettingsEvent
{
  enum class Type
  {
    SkyBoxChanged,
    SkyLightChanged,
    SkyLightCubeMapChanged,
    SkyLightIntensityChanged,
    DirectionalLightChanged,
    DirectionalLightAngleChanged,
    DirectionalLightShadowsChanged,
    DirectionalLightIntensityChanged,
    FogChanged,
    DefaultValuesChanged,
  };

  Type m_Type;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEngineViewLightSettings : public xiiEditorEngineSyncObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEngineViewLightSettings, xiiEditorEngineSyncObject);

public:
  xiiEngineViewLightSettings(bool bEnable = true);
  ~xiiEngineViewLightSettings();

  bool GetSkyBox() const;
  void SetSkyBox(bool val);

  bool GetSkyLight() const;
  void SetSkyLight(bool val);

  const char* GetSkyLightCubeMap() const;
  void        SetSkyLightCubeMap(const char* val);

  float GetSkyLightIntensity() const;
  void  SetSkyLightIntensity(float val);

  bool GetDirectionalLight() const;
  void SetDirectionalLight(bool val);

  xiiAngle GetDirectionalLightAngle() const;
  void     SetDirectionalLightAngle(xiiAngle val);

  bool GetDirectionalLightShadows() const;
  void SetDirectionalLightShadows(bool val);

  float GetDirectionalLightIntensity() const;
  void  SetDirectionalLightIntensity(float val);

  bool GetFog() const;
  void SetFog(bool val);

  mutable xiiEvent<const xiiEngineViewLightSettingsEvent&> m_EngineViewLightSettingsEvents;

  virtual bool SetupForEngine(xiiWorld* pWorld, xiiUInt32 uiNextComponentPickingID) override;
  virtual void UpdateForEngine(xiiWorld* pWorld) override;

private:
  void SetModifiedInternal(xiiEngineViewLightSettingsEvent::Type type);

  bool      m_bSkyBox            = true;
  bool      m_bSkyLight          = true;
  xiiString m_sSkyLightCubeMap   = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
  float     m_fSkyLightIntensity = 1.0f;

  bool     m_bDirectionalLight          = true;
  xiiAngle m_DirectionalLightAngle      = xiiAngle::Degree(30.0f);
  bool     m_bDirectionalLightShadows   = false;
  float    m_fDirectionalLightIntensity = 10.0f;

  bool m_bFog = false;

  // Engine side data
  xiiWorld*           m_pWorld = nullptr;
  xiiGameObjectHandle m_hSkyBoxObject;
  xiiComponentHandle  m_hSkyBox;
  xiiGameObjectHandle m_hGameObject;
  xiiComponentHandle  m_hDirLight;
  xiiComponentHandle  m_hSkyLight;
  xiiComponentHandle  m_hFog;
};
