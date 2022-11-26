#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

struct xiiMsgExtractGeometry;

using xiiJoltStaticActorComponentManager = xiiComponentManager<class xiiJoltStaticActorComponent, xiiBlockStorageType::FreeList>;

class XII_JOLTPLUGIN_DLL xiiJoltStaticActorComponent : public xiiJoltActorComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltStaticActorComponent, xiiJoltActorComponent, xiiJoltStaticActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  void PullSurfacesFromGraphicsMesh(xiiDynamicArray<const xiiJoltMaterial*>& materials);

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltActorComponent
protected:
  virtual void CreateShapes(xiiDynamicArray<xiiJoltSubShape>& out_Shapes, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltStaticActorComponent

public:
  xiiJoltStaticActorComponent();
  ~xiiJoltStaticActorComponent();

  void        SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;             // [ property ]

  void                    SetMesh(const xiiJoltMeshResourceHandle& hMesh);
  XII_ALWAYS_INLINE const xiiJoltMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

  void        SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;             // [ property ]

  bool                     m_bIncludeInNavmesh             = true;  // [ property ]
  bool                     m_bPullSurfacesFromGraphicsMesh = false; // [ property ]
  xiiSurfaceResourceHandle m_hSurface;                              // [ property ]

protected:
  void                   OnMsgExtractGeometry(xiiMsgExtractGeometry& msg) const;
  const xiiJoltMaterial* GetJoltMaterial() const;

  xiiJoltMeshResourceHandle m_hCollisionMesh;

  // array to keep surfaces alive, in case they are pulled from the materials of the render mesh
  xiiDynamicArray<xiiSurfaceResourceHandle> m_UsedSurfaces;
};
