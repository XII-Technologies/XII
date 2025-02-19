#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GraphicsCore/Meshes/MeshComponentBase.h>

struct xiiPerInstanceData;
struct xiiRenderWorldRenderEvent;
class xiiInstancedMeshComponent;
struct xiiMsgExtractGeometry;
class xiiStreamWriter;
class xiiStreamReader;

struct XII_GRAPHICSCORE_DLL xiiMeshInstanceData
{
  void    SetLocalPosition(xiiVec3 vPosition);
  xiiVec3 GetLocalPosition() const;

  void    SetLocalRotation(xiiQuat qRotation);
  xiiQuat GetLocalRotation() const;

  void    SetLocalScaling(xiiVec3 vScaling);
  xiiVec3 GetLocalScaling() const;

  xiiResult Serialize(xiiStreamWriter& ref_writer) const;
  xiiResult Deserialize(xiiStreamReader& ref_reader);

  xiiTransform m_transform;

  xiiColor m_color;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMeshInstanceData);

//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiInstancedMeshRenderData : public xiiMeshRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInstancedMeshRenderData, xiiMeshRenderData);

public:
  virtual bool CanBatch(const xiiRenderData& other) const override { return false; }

  xiiInstanceData* m_pExplicitInstanceData   = nullptr;
  xiiUInt32        m_uiExplicitInstanceCount = 0;
};

//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiInstancedMeshComponentManager : public xiiComponentManager<class xiiInstancedMeshComponent, xiiBlockStorageType::Compact>
{
public:
  using SUPER = xiiComponentManager<xiiInstancedMeshComponent, xiiBlockStorageType::Compact>;

  xiiInstancedMeshComponentManager(xiiWorld* pWorld);

  void EnqueueUpdate(const xiiInstancedMeshComponent* pComponent) const;

private:
  struct ComponentToUpdate
  {
    xiiComponentHandle              m_hComponent;
    xiiArrayPtr<xiiPerInstanceData> m_InstanceData;
  };

  mutable xiiMutex                    m_Mutex;
  mutable xiiDeque<ComponentToUpdate> m_RequireUpdate;

protected:
  void OnRenderEvent(const xiiRenderWorldRenderEvent& e);

  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

class XII_GRAPHICSCORE_DLL xiiInstancedMeshComponent : public xiiMeshComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiInstancedMeshComponent, xiiMeshComponentBase, xiiInstancedMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiMeshComponentBase

protected:
  virtual xiiMeshRenderData* CreateRenderData() const override;


  //////////////////////////////////////////////////////////////////////////
  // xiiInstancedMeshComponent

public:
  xiiInstancedMeshComponent();
  ~xiiInstancedMeshComponent();

  /// \brief Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(xiiMsgExtractGeometry& ref_msg); // [ msg handler ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  xiiUInt32           Instances_GetCount() const;                                       // [ property ]
  xiiMeshInstanceData Instances_GetValue(xiiUInt32 uiIndex) const;                      // [ property ]
  void                Instances_SetValue(xiiUInt32 uiIndex, xiiMeshInstanceData value); // [ property ]
  void                Instances_Insert(xiiUInt32 uiIndex, xiiMeshInstanceData value);   // [ property ]
  void                Instances_Remove(xiiUInt32 uiIndex);                              // [ property ]

  xiiArrayPtr<xiiPerInstanceData> GetInstanceData() const;

  // Unpacked, reflected instance data for editing and ease of access
  xiiDynamicArray<xiiMeshInstanceData> m_RawInstancedData;

  xiiInstanceData* m_pExplicitInstanceData = nullptr;

  mutable xiiUInt64 m_uiEnqueuedFrame = xiiUInt64(-1);
};
