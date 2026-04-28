/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/World.h>
#include <GameEngine/Volumes/VolumeSampler.h>
#include <GraphicsCore/Pipeline/Declarations.h>

class XII_GAMEENGINE_DLL xiiPostProcessingComponentManager : public xiiComponentManager<class xiiPostProcessingComponent, xiiBlockStorageType::Compact>
{
public:
  xiiPostProcessingComponentManager(xiiWorld* pWorld);

  virtual void Initialize() override;

private:
  void UpdateComponents(const UpdateContext& context);
};

struct xiiPostProcessingValueMapping
{
  xiiHashedString m_sRenderPassName;
  xiiHashedString m_sPropertyName;
  xiiHashedString m_sVolumeValueName;
  xiiVariant      m_DefaultValue;
  xiiTime         m_InterpolationDuration;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiPostProcessingValueMapping);

/// \brief A component that sets the configured values on a render pipeline and optionally samples those values from volumes at the corresponding camera position.
///
/// If there is a render target camera component attached to the owner object it will affect the render pipeline of this camera,
/// otherwise the render pipeline of the main camera is affected.
class XII_GAMEENGINE_DLL xiiPostProcessingComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPostProcessingComponent, xiiComponent, xiiPostProcessingComponentManager);

public:
  xiiPostProcessingComponent();
  xiiPostProcessingComponent(xiiPostProcessingComponent&& other);
  ~xiiPostProcessingComponent();
  xiiPostProcessingComponent& operator=(xiiPostProcessingComponent&& other);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void        SetVolumeType(const char* szType); // [ property ]
  const char* GetVolumeType() const;             // [ property ]

private:
  xiiUInt32                            Mappings_GetCount() const { return m_Mappings.GetCount(); }                       // [ property ]
  const xiiPostProcessingValueMapping& Mappings_GetMapping(xiiUInt32 i) const { return m_Mappings[i]; }                  // [ property ]
  void                                 Mappings_SetMapping(xiiUInt32 i, const xiiPostProcessingValueMapping& mapping);   // [ property ]
  void                                 Mappings_Insert(xiiUInt32 uiIndex, const xiiPostProcessingValueMapping& mapping); // [ property ]
  void                                 Mappings_Remove(xiiUInt32 uiIndex);                                               // [ property ]

  xiiView* FindView() const;
  void     RegisterSamplerValues();
  void     ResetViewProperties();
  void     SampleAndSetViewProperties();

  xiiComponentHandle                             m_hCameraComponent;
  xiiDynamicArray<xiiPostProcessingValueMapping> m_Mappings;
  xiiUniquePtr<xiiVolumeSampler>                 m_pSampler;
  xiiSpatialData::Category                       m_SpatialCategory = xiiInvalidSpatialDataCategory;
};
