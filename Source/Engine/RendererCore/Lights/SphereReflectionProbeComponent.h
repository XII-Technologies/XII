#pragma once

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

class XII_RENDERERCORE_DLL xiiSphereReflectionProbeComponentManager final : public xiiComponentManager<class xiiSphereReflectionProbeComponent, xiiBlockStorageType::Compact>
{
public:
  xiiSphereReflectionProbeComponentManager(xiiWorld* pWorld);
};

//////////////////////////////////////////////////////////////////////////
// xiiSphereReflectionProbeComponent

/// \brief Sphere reflection probe component.
///
/// The generated reflection cube map is is projected to infinity. So parallax correction takes place.
class XII_RENDERERCORE_DLL xiiSphereReflectionProbeComponent : public xiiReflectionProbeComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiSphereReflectionProbeComponent, xiiReflectionProbeComponentBase, xiiSphereReflectionProbeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiSphereReflectionProbeComponent

public:
  xiiSphereReflectionProbeComponent();
  ~xiiSphereReflectionProbeComponent();

  void  SetRadius(float fRadius); // [ property ]
  float GetRadius() const;        // [ property ]

  void  SetFalloff(float fFalloff);               // [ property ]
  float GetFalloff() const { return m_fFalloff; } // [ property ]

  void SetSphereProjection(bool bSphereProjection);                // [ property ]
  bool GetSphereProjection() const { return m_bSphereProjection; } // [ property ]

protected:
  //////////////////////////////////////////////////////////////////////////
  // Editor
  void OnObjectCreated(const xiiAbstractObjectNode& node);

protected:
  void  OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);
  void  OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;
  void  OnTransformChanged(xiiMsgTransformChanged& msg);
  float m_fRadius           = 5.0f;
  float m_fFalloff          = 0.1f;
  bool  m_bSphereProjection = true;
};
