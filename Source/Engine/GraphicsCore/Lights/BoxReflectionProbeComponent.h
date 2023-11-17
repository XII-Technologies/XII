#pragma once

#include <GraphicsCore/Lights/ReflectionProbeComponentBase.h>

class XII_GRAPHICSCORE_DLL xiiBoxReflectionProbeComponentManager final : public xiiComponentManager<class xiiBoxReflectionProbeComponent, xiiBlockStorageType::Compact>
{
public:
  xiiBoxReflectionProbeComponentManager(xiiWorld* pWorld);
};

/// \brief Box reflection probe component.
///
/// The generated reflection cube map is projected on a box defined by this component's extents. The influence volume can be smaller than the projection which is defined by a scale and shift parameter. Each side of the influence volume has a separate falloff parameter to smoothly blend the probe into others.
class XII_GRAPHICSCORE_DLL xiiBoxReflectionProbeComponent : public xiiReflectionProbeComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiBoxReflectionProbeComponent, xiiReflectionProbeComponentBase, xiiBoxReflectionProbeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiBoxReflectionProbeComponent

public:
  xiiBoxReflectionProbeComponent();
  ~xiiBoxReflectionProbeComponent();

  const xiiVec3& GetExtents() const;                  // [ property ]
  void           SetExtents(const xiiVec3& vExtents); // [ property ]

  const xiiVec3& GetInfluenceScale() const;                         // [ property ]
  void           SetInfluenceScale(const xiiVec3& vInfluenceScale); // [ property ]
  const xiiVec3& GetInfluenceShift() const;                         // [ property ]
  void           SetInfluenceShift(const xiiVec3& vInfluenceShift); // [ property ]

  void           SetPositiveFalloff(const xiiVec3& vFalloff);              // [ property ]
  const xiiVec3& GetPositiveFalloff() const { return m_vPositiveFalloff; } // [ property ]
  void           SetNegativeFalloff(const xiiVec3& vFalloff);              // [ property ]
  const xiiVec3& GetNegativeFalloff() const { return m_vNegativeFalloff; } // [ property ]

  void SetBoxProjection(bool bBoxProjection);                // [ property ]
  bool GetBoxProjection() const { return m_bBoxProjection; } // [ property ]

protected:
  //////////////////////////////////////////////////////////////////////////
  // Editor
  void OnObjectCreated(const xiiAbstractObjectNode& node);

protected:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;
  void OnTransformChanged(xiiMsgTransformChanged& msg);

protected:
  xiiVec3 m_vExtents         = xiiVec3(5.0f);
  xiiVec3 m_vInfluenceScale  = xiiVec3(1.0f);
  xiiVec3 m_vInfluenceShift  = xiiVec3(0.0f);
  xiiVec3 m_vPositiveFalloff = xiiVec3(0.1f, 0.1f, 0.0f);
  xiiVec3 m_vNegativeFalloff = xiiVec3(0.1f, 0.1f, 0.0f);
  bool    m_bBoxProjection   = true;
};

/// \brief A special visualizer attribute for box reflection probes
class XII_GRAPHICSCORE_DLL xiiBoxReflectionProbeVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBoxReflectionProbeVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiBoxReflectionProbeVisualizerAttribute();

  xiiBoxReflectionProbeVisualizerAttribute(const char* szExtentsProperty, const char* szInfluenceScaleProperty, const char* szInfluenceShiftProperty);

  const xiiUntrackedString& GetExtentsProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetInfluenceScaleProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetInfluenceShiftProperty() const { return m_sProperty3; }
};
