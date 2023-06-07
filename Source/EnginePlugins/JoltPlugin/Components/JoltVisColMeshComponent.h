#pragma once

#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

class xiiJoltVisColMeshComponentManager : public xiiComponentManager<class xiiJoltVisColMeshComponent, xiiBlockStorageType::Compact>
{
public:
  using SUPER = xiiComponentManager<xiiJoltVisColMeshComponent, xiiBlockStorageType::Compact>;

  xiiJoltVisColMeshComponentManager(xiiWorld* pWorld) :
    SUPER(pWorld)
  {
  }

  void Update(const xiiWorldModule::UpdateContext& context);
  void EnqueueUpdate(xiiComponentHandle hComponent);

private:
  void ResourceEventHandler(const xiiResourceEvent& e);

  mutable xiiMutex             m_Mutex;
  xiiDeque<xiiComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

class XII_JOLTPLUGIN_DLL xiiJoltVisColMeshComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltVisColMeshComponent, xiiRenderComponent, xiiJoltVisColMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltVisColMeshComponent

public:
  xiiJoltVisColMeshComponent();
  ~xiiJoltVisColMeshComponent();

  void        SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;             // [ property ]

  void                    SetMesh(const xiiJoltMeshResourceHandle& hMesh);
  XII_ALWAYS_INLINE const xiiJoltMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;
  void CreateCollisionRenderMesh();

  xiiJoltMeshResourceHandle     m_hCollisionMesh;
  mutable xiiMeshResourceHandle m_hMesh;
};
