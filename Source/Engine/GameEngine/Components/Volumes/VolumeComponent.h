/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>

struct xiiMsgUpdateLocalBounds;

using xiiBlackboardTemplateResourceHandle = xiiTypedResourceHandle<class xiiBlackboardTemplateResource>;

/// A volume component can hold generic values either from a blackboard template or set directly on the component.
///
/// The values can be sampled with a xiiVolumeSampler and then used for things like e.g. post-processing, reverb etc.
/// They can also be used to represent knowledge in a scene, like e.g. smell or threat, and can be detected by a xiiSensorComponent and then processed by AI.
class XII_GAMEENGINE_DLL xiiVolumeComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiVolumeComponent, xiiComponent);

public:
  xiiVolumeComponent();
  ~xiiVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void                                       SetTemplate(const xiiBlackboardTemplateResourceHandle& hResource); // [ property ]
  const xiiBlackboardTemplateResourceHandle& GetTemplate() const { return m_hTemplateResource; }                // [ property ]

  /// @brief In case two volumes overlap, the one with a higher sort order value has precedence.
  void  SetSortOrder(float fOrder);                   // [ property ]
  float GetSortOrder() const { return m_fSortOrder; } // [ property ]

  /// @brief
  void        SetVolumeType(const char* szType); // [ property ]
  const char* GetVolumeType() const;             // [ property ]

  void       SetValue(const xiiHashedString& sName, const xiiVariant& value); // [ scriptable ]
  xiiVariant GetValue(xiiTempHashedString sName) const                        // [ scriptable ]
  {
    xiiVariant v;
    m_Values.TryGetValue(sName, v);
    return v;
  }

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  const xiiRangeView<const xiiString&, xiiUInt32> Reflection_GetKeys() const;
  bool                                            Reflection_GetValue(xiiStringView sName, xiiVariant& value) const;
  void                                            Reflection_InsertValue(xiiStringView sName, const xiiVariant& value);
  void                                            Reflection_RemoveValue(xiiStringView sName);

  void InitializeFromTemplate();
  void ReloadTemplate();
  void RemoveReloadFunction();

  xiiBlackboardTemplateResourceHandle       m_hTemplateResource;
  xiiHashTable<xiiHashedString, xiiVariant> m_Values;
  xiiSmallArray<xiiHashedString, 1>         m_OverwrittenValues; // only used in editor
  float                                     m_fSortOrder           = 0.0f;
  xiiSpatialData::Category                  m_SpatialCategory      = xiiInvalidSpatialDataCategory;
  bool                                      m_bReloadFunctionAdded = false;
};

//////////////////////////////////////////////////////////////////////////

using xiiVolumeSphereComponentManager = xiiComponentManager<class xiiVolumeSphereComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiVolumeSphereComponent : public xiiVolumeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiVolumeSphereComponent, xiiVolumeComponent, xiiVolumeSphereComponentManager);

public:
  xiiVolumeSphereComponent();
  ~xiiVolumeSphereComponent();

  float GetRadius() const { return m_fRadius; }
  void  SetRadius(float fRadius);

  float GetFalloff() const { return m_fFalloff; }
  void  SetFalloff(float fFalloff);

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const;

protected:
  float m_fRadius  = 5.0f;
  float m_fFalloff = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

using xiiVolumeBoxComponentManager = xiiComponentManager<class xiiVolumeBoxComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiVolumeBoxComponent : public xiiVolumeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiVolumeBoxComponent, xiiVolumeComponent, xiiVolumeBoxComponentManager);

public:
  xiiVolumeBoxComponent();
  ~xiiVolumeBoxComponent();

  const xiiVec3& GetExtents() const { return m_vExtents; }
  void           SetExtents(const xiiVec3& vExtents);

  const xiiVec3& GetFalloff() const { return m_vFalloff; }
  void           SetFalloff(const xiiVec3& vFalloff);

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const;

protected:
  xiiVec3 m_vExtents = xiiVec3(10.0f);
  xiiVec3 m_vFalloff = xiiVec3(0.5f);
};
