#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using xiiJoltShapeBoxComponentManager = xiiComponentManager<class xiiJoltShapeBoxComponent, xiiBlockStorageType::FreeList>;

class XII_JOLTPLUGIN_DLL xiiJoltShapeBoxComponent : public xiiJoltShapeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltShapeBoxComponent, xiiJoltShapeComponent, xiiJoltShapeBoxComponentManager);

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
  // xiiJoltShapeBoxComponent

public:
  xiiJoltShapeBoxComponent();
  ~xiiJoltShapeBoxComponent();

  void           SetHalfExtents(const xiiVec3& value);             // [ property ]
  const xiiVec3& GetHalfExtents() const { return m_vHalfExtents; } // [ property ]

  virtual void ExtractGeometry(xiiMsgExtractGeometry& ref_msg) const override;

protected:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const;

  xiiVec3 m_vHalfExtents = xiiVec3(0.5f);
};
