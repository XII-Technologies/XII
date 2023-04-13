#pragma once

#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using xiiJoltShapeConvexHullComponentManager = xiiComponentManager<class xiiJoltShapeConvexHullComponent, xiiBlockStorageType::FreeList>;

class XII_JOLTPLUGIN_DLL xiiJoltShapeConvexHullComponent : public xiiJoltShapeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltShapeConvexHullComponent, xiiJoltShapeComponent, xiiJoltShapeConvexHullComponentManager);

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
  // xiiConvexShapeConvexComponent

public:
  xiiJoltShapeConvexHullComponent();
  ~xiiJoltShapeConvexHullComponent();

  virtual void ExtractGeometry(xiiMsgExtractGeometry& ref_msg) const override;

  void        SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;             // [ property ]

  xiiJoltMeshResourceHandle GetMesh() const { return m_hCollisionMesh; }

protected:
  xiiJoltMeshResourceHandle m_hCollisionMesh;
};
