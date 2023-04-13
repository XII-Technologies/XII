#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using xiiJoltShapeSphereComponentManager = xiiComponentManager<class xiiJoltShapeSphereComponent, xiiBlockStorageType::FreeList>;

class XII_JOLTPLUGIN_DLL xiiJoltShapeSphereComponent : public xiiJoltShapeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltShapeSphereComponent, xiiJoltShapeComponent, xiiJoltShapeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltShapeComponent

protected:
  virtual void CreateShapes(xiiDynamicArray<xiiJoltSubShape>& out_Shapes, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltShapeSphereComponent

public:
  xiiJoltShapeSphereComponent();
  ~xiiJoltShapeSphereComponent();

  void  SetRadius(float f);                     // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

protected:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
};
