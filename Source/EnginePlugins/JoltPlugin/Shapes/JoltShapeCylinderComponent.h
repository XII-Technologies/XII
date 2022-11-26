#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using xiiJoltShapeCylinderComponentManager = xiiComponentManager<class xiiJoltShapeCylinderComponent, xiiBlockStorageType::FreeList>;

class XII_JOLTPLUGIN_DLL xiiJoltShapeCylinderComponent : public xiiJoltShapeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltShapeCylinderComponent, xiiJoltShapeComponent, xiiJoltShapeCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltShapeComponent

protected:
  virtual void CreateShapes(xiiDynamicArray<xiiJoltSubShape>& out_Shapes, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltShapeCylinderComponent

public:
  xiiJoltShapeCylinderComponent();
  ~xiiJoltShapeCylinderComponent();

  void  SetRadius(float f);                     // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

  void  SetHeight(float f);                     // [ property ]
  float GetHeight() const { return m_fHeight; } // [ property ]

protected:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
  float m_fHeight = 0.5f;
};
