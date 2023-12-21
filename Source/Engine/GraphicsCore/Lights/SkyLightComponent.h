#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <GraphicsCore/Lights/Implementation/ReflectionProbeData.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>

struct xiiMsgUpdateLocalBounds;
struct xiiMsgExtractRenderData;
struct xiiMsgTransformChanged;

using xiiSkyLightComponentManager = xiiSettingsComponentManager<class xiiSkyLightComponent>;

class XII_GRAPHICSCORE_DLL xiiSkyLightComponent : public xiiSettingsComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkyLightComponent, xiiSettingsComponent, xiiSkyLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiSkyLightComponent

public:
  xiiSkyLightComponent();
  ~xiiSkyLightComponent();

  void                            SetReflectionProbeMode(xiiEnum<xiiReflectionProbeMode> mode); // [ property ]
  xiiEnum<xiiReflectionProbeMode> GetReflectionProbeMode() const;                               // [ property ]

  void  SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;           // [ property ]

  void  SetSaturation(float fSaturation); // [ property ]
  float GetSaturation() const;            // [ property ]

  const xiiTagSet& GetIncludeTags() const;              // [ property ]
  void             InsertIncludeTag(const char* szTag); // [ property ]
  void             RemoveIncludeTag(const char* szTag); // [ property ]

  const xiiTagSet& GetExcludeTags() const;              // [ property ]
  void             InsertExcludeTag(const char* szTag); // [ property ]
  void             RemoveExcludeTag(const char* szTag); // [ property ]

  void SetShowDebugInfo(bool bShowDebugInfo); // [ property ]
  bool GetShowDebugInfo() const;              // [ property ]

  void SetShowMipMaps(bool bShowMipMaps); // [ property ]
  bool GetShowMipMaps() const;            // [ property ]

  void                         SetCubeMapFile(const char* szFile); // [ property ]
  const char*                  GetCubeMapFile() const;             // [ property ]
  xiiTextureCubeResourceHandle GetCubeMap() const
  {
    return m_hCubeMap;
  }

  float GetNearPlane() const { return m_Desc.m_fNearPlane; } // [ property ]
  void  SetNearPlane(float fNearPlane);                      // [ property ]

  float GetFarPlane() const { return m_Desc.m_fFarPlane; } // [ property ]
  void  SetFarPlane(float fFarPlane);                      // [ property ]

protected:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;
  void OnTransformChanged(xiiMsgTransformChanged& msg);

  xiiReflectionProbeDesc       m_Desc;
  xiiTextureCubeResourceHandle m_hCubeMap;

  xiiReflectionProbeId m_Id;

  mutable bool m_bStatesDirty = true;
};
