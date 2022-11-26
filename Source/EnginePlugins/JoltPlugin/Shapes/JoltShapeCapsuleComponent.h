#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using xiiJoltShapeCapsuleComponentManager = xiiComponentManager<class xiiJoltShapeCapsuleComponent, xiiBlockStorageType::FreeList>;

class XII_JOLTPLUGIN_DLL xiiJoltShapeCapsuleComponent : public xiiJoltShapeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltShapeCapsuleComponent, xiiJoltShapeComponent, xiiJoltShapeCapsuleComponentManager);

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
  // xiiJoltShapeCapsuleComponent

public:
  xiiJoltShapeCapsuleComponent();
  ~xiiJoltShapeCapsuleComponent();

  void  SetRadius(float f);                     // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

  void  SetHeight(float f);                     // [ property ]
  float GetHeight() const { return m_fHeight; } // [ property ]

protected:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
  float m_fHeight = 0.5f;
};
