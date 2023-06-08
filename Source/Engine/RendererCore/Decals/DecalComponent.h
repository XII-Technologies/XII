#pragma once

#include <Foundation/Math/Color16f.h>
#include <Foundation/Types/VarianceTypes.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

class xiiAbstractObjectNode;
struct xiiMsgComponentInternalTrigger;
struct xiiMsgOnlyApplyToObject;
struct xiiMsgSetColor;

class XII_RENDERERCORE_DLL xiiDecalComponentManager final : public xiiComponentManager<class xiiDecalComponent, xiiBlockStorageType::Compact>
{
public:
  xiiDecalComponentManager(xiiWorld* pWorld);

  virtual void Initialize() override;

private:
  friend class xiiDecalComponent;
  xiiDecalAtlasResourceHandle m_hDecalAtlas;
};

class XII_RENDERERCORE_DLL xiiDecalRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalRenderData, xiiRenderData);

public:
  xiiUInt32 m_uiApplyOnlyToId;
  xiiUInt32 m_uiFlags;
  xiiUInt32 m_uiAngleFadeParams;

  xiiColorLinearUB  m_BaseColor;
  xiiColorLinear16f m_EmissiveColor;

  xiiUInt32 m_uiBaseColorAtlasScale;
  xiiUInt32 m_uiBaseColorAtlasOffset;

  xiiUInt32 m_uiNormalAtlasScale;
  xiiUInt32 m_uiNormalAtlasOffset;

  xiiUInt32 m_uiORMAtlasScale;
  xiiUInt32 m_uiORMAtlasOffset;
};

class XII_RENDERERCORE_DLL xiiDecalComponent final : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDecalComponent, xiiRenderComponent, xiiDecalComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

protected:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;
  void              OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;


  //////////////////////////////////////////////////////////////////////////
  // xiiDecalComponent

public:
  xiiDecalComponent();
  ~xiiDecalComponent();

  void           SetExtents(const xiiVec3& value); // [ property ]
  const xiiVec3& GetExtents() const;               // [ property ]

  void  SetSizeVariance(float fVariance); // [ property ]
  float GetSizeVariance() const;          // [ property ]

  void            SetColor(xiiColorGammaUB color); // [ property ]
  xiiColorGammaUB GetColor() const;                // [ property ]

  void     SetEmissiveColor(xiiColor color); // [ property ]
  xiiColor GetEmissiveColor() const;         // [ property ]

  void     SetInnerFadeAngle(xiiAngle fadeAngle); // [ property ]
  xiiAngle GetInnerFadeAngle() const;             // [ property ]

  void     SetOuterFadeAngle(xiiAngle fadeAngle); // [ property ]
  xiiAngle GetOuterFadeAngle() const;             // [ property ]

  void  SetSortOrder(float fOrder); // [ property ]
  float GetSortOrder() const;       // [ property ]

  void SetWrapAround(bool bWrapAround); // [ property ]
  bool GetWrapAround() const;           // [ property ]

  void SetMapNormalToGeometry(bool bMapNormal); // [ property ]
  bool GetMapNormalToGeometry() const;          // [ property ]

  void                          SetDecal(xiiUInt32 uiIndex, const xiiDecalResourceHandle& hResource); // [ property ]
  const xiiDecalResourceHandle& GetDecal(xiiUInt32 uiIndex) const;                                    // [ property ]

  xiiVarianceTypeTime                   m_FadeOutDelay;     // [ property ]
  xiiTime                               m_FadeOutDuration;  // [ property ]
  xiiEnum<xiiOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

  void                  SetProjectionAxis(xiiEnum<xiiBasisAxis> projectionAxis); // [ property ]
  xiiEnum<xiiBasisAxis> GetProjectionAxis() const;                               // [ property ]

  void                SetApplyOnlyTo(xiiGameObjectHandle hObject);
  xiiGameObjectHandle GetApplyOnlyTo() const;

  xiiUInt32   DecalFile_GetCount() const;                              // [ property ]
  const char* DecalFile_Get(xiiUInt32 uiIndex) const;                  // [ property ]
  void        DecalFile_Set(xiiUInt32 uiIndex, const char* szFile);    // [ property ]
  void        DecalFile_Insert(xiiUInt32 uiIndex, const char* szFile); // [ property ]
  void        DecalFile_Remove(xiiUInt32 uiIndex);                     // [ property ]


protected:
  void SetApplyToRef(const char* szReference); // [ property ]
  void UpdateApplyTo();

  void OnTriggered(xiiMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(xiiMsgDeleteGameObject& msg);
  void OnMsgOnlyApplyToObject(xiiMsgOnlyApplyToObject& msg);
  void OnMsgSetColor(xiiMsgSetColor& msg);

  xiiVec3                                   m_vExtents             = xiiVec3(1.0f);
  float                                     m_fSizeVariance        = 0;
  xiiColorGammaUB                           m_Color                = xiiColor::White;
  xiiColor                                  m_EmissiveColor        = xiiColor::Black;
  xiiAngle                                  m_InnerFadeAngle       = xiiAngle::Degree(50.0f);
  xiiAngle                                  m_OuterFadeAngle       = xiiAngle::Degree(80.0f);
  float                                     m_fSortOrder           = 0;
  bool                                      m_bWrapAround          = false;
  bool                                      m_bMapNormalToGeometry = false;
  xiiUInt8                                  m_uiRandomDecalIdx     = 0xFF;
  xiiEnum<xiiBasisAxis>                     m_ProjectionAxis;
  xiiHybridArray<xiiDecalResourceHandle, 1> m_Decals;

  xiiGameObjectHandle m_hApplyOnlyToObject;
  xiiUInt32           m_uiApplyOnlyToId = 0;

  xiiTime   m_StartFadeOutTime;
  xiiUInt32 m_uiInternalSortKey = 0;

private:
  const char* DummyGetter() const { return nullptr; }
};
