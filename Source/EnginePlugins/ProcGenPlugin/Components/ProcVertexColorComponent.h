#pragma once

#include <Core/World/World.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <RendererCore/Meshes/MeshComponent.h>

class XII_PROCGENPLUGIN_DLL xiiProcVertexColorRenderData : public xiiMeshRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcVertexColorRenderData, xiiMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey() override;

  xiiGALBufferHandle m_hVertexColorBuffer;
  xiiUInt32          m_uiBufferAccessData = 0;
};

//////////////////////////////////////////////////////////////////////////

struct xiiRenderWorldExtractionEvent;
struct xiiRenderWorldRenderEvent;
class xiiProcVertexColorComponent;

class XII_PROCGENPLUGIN_DLL xiiProcVertexColorComponentManager : public xiiComponentManager<xiiProcVertexColorComponent, xiiBlockStorageType::Compact>
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiProcVertexColorComponentManager);

public:
  xiiProcVertexColorComponentManager(xiiWorld* pWorld);
  ~xiiProcVertexColorComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class xiiProcVertexColorComponent;

  void UpdateVertexColors(const xiiWorldModule::UpdateContext& context);
  void UpdateComponentVertexColors(xiiProcVertexColorComponent* pComponent);
  void OnExtractionEvent(const xiiRenderWorldExtractionEvent& e);
  void OnRenderEvent(const xiiRenderWorldRenderEvent& e);

  void EnqueueUpdate(xiiProcVertexColorComponent* pComponent);
  void RemoveComponent(xiiProcVertexColorComponent* pComponent);

  void OnResourceEvent(const xiiResourceEvent& resourceEvent);

  void OnAreaInvalidated(const xiiProcGenInternal::InvalidatedArea& area);

  xiiDynamicArray<xiiComponentHandle> m_ComponentsToUpdate;

  xiiDynamicArray<xiiSharedPtr<xiiProcGenInternal::VertexColorTask>> m_UpdateTasks;
  xiiTaskGroupID                                                     m_UpdateTaskGroupID;
  xiiUInt32                                                          m_uiNextTaskIndex = 0;

  xiiGALBufferHandle         m_hVertexColorBuffer;
  xiiDynamicArray<xiiUInt32> m_VertexColorData;
  xiiUInt32                  m_uiCurrentBufferOffset = 0;

  xiiGAL::ModifiedRange m_ModifiedDataRange;

  struct DataCopy
  {
    xiiArrayPtr<xiiUInt32> m_Data;
    xiiUInt32              m_uiStart = 0;
  };
  DataCopy m_DataCopy[2];
};

//////////////////////////////////////////////////////////////////////////

struct xiiProcVertexColorOutputDesc
{
  xiiHashedString           m_sName;
  xiiProcVertexColorMapping m_Mapping;

  void        SetName(const char* szName);
  const char* GetName() const { return m_sName; }

  xiiResult Serialize(xiiStreamWriter& ref_stream) const;
  xiiResult Deserialize(xiiStreamReader& ref_stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PROCGENPLUGIN_DLL, xiiProcVertexColorOutputDesc);

//////////////////////////////////////////////////////////////////////////

struct xiiMsgTransformChanged;

class XII_PROCGENPLUGIN_DLL xiiProcVertexColorComponent : public xiiMeshComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiProcVertexColorComponent, xiiMeshComponent, xiiProcVertexColorComponentManager);

public:
  xiiProcVertexColorComponent();
  ~xiiProcVertexColorComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void        SetResourceFile(const char* szFile);
  const char* GetResourceFile() const;

  void                                 SetResource(const xiiProcGenGraphResourceHandle& hResource);
  const xiiProcGenGraphResourceHandle& GetResource() const { return m_hResource; }

  const xiiProcVertexColorOutputDesc& GetOutputDesc(xiiUInt32 uiIndex) const;
  void                                SetOutputDesc(xiiUInt32 uiIndex, const xiiProcVertexColorOutputDesc& outputDesc);

  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

  void OnTransformChanged(xiiMsgTransformChanged& ref_msg);

protected:
  virtual xiiMeshRenderData* CreateRenderData() const override;

private:
  xiiUInt32 OutputDescs_GetCount() const;
  void      OutputDescs_Insert(xiiUInt32 uiIndex, const xiiProcVertexColorOutputDesc& outputDesc);
  void      OutputDescs_Remove(xiiUInt32 uiIndex);

  bool HasValidOutputs() const;

  xiiProcGenGraphResourceHandle                   m_hResource;
  xiiHybridArray<xiiProcVertexColorOutputDesc, 2> m_OutputDescs;

  xiiHybridArray<xiiSharedPtr<const xiiProcGenInternal::VertexColorOutput>, 2> m_Outputs;

  xiiGALBufferHandle m_hVertexColorBuffer;
  xiiUInt32          m_uiBufferAccessData = 0;
};
