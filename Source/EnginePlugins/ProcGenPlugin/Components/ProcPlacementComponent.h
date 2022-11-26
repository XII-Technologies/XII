#pragma once

#include <Core/World/World.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>

class xiiProcPlacementComponent;
struct xiiMsgUpdateLocalBounds;
struct xiiMsgExtractRenderData;

//////////////////////////////////////////////////////////////////////////

class XII_PROCGENPLUGIN_DLL xiiProcPlacementComponentManager : public xiiComponentManager<xiiProcPlacementComponent, xiiBlockStorageType::Compact>
{
public:
  xiiProcPlacementComponentManager(xiiWorld* pWorld);
  ~xiiProcPlacementComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class xiiProcPlacementComponent;

  void FindTiles(const xiiWorldModule::UpdateContext& context);
  void PreparePlace(const xiiWorldModule::UpdateContext& context);
  void PlaceObjects(const xiiWorldModule::UpdateContext& context);

  void DebugDrawTile(const xiiProcGenInternal::PlacementTileDesc& desc, const xiiColor& color, xiiUInt32 uiQueueIndex = xiiInvalidIndex);

  void AddComponent(xiiProcPlacementComponent* pComponent);
  void RemoveComponent(xiiProcPlacementComponent* pComponent);

  xiiUInt32 AllocateTile(const xiiProcGenInternal::PlacementTileDesc& desc, xiiSharedPtr<const xiiProcGenInternal::PlacementOutput>& pOutput);
  void      DeallocateTile(xiiUInt32 uiTileIndex);

  xiiUInt32 AllocateProcessingTask(xiiUInt32 uiTileIndex);
  void      DeallocateProcessingTask(xiiUInt32 uiTaskIndex);
  xiiUInt32 GetNumAllocatedProcessingTasks() const;

  void RemoveTilesForComponent(xiiProcPlacementComponent* pComponent, bool* out_bAnyObjectsRemoved = nullptr);
  void OnResourceEvent(const xiiResourceEvent& resourceEvent);

  void AddVisibleComponent(const xiiComponentHandle& hComponent, const xiiVec3& cameraPosition, const xiiVec3& cameraDirection) const;
  void ClearVisibleComponents();

  struct VisibleComponent
  {
    xiiComponentHandle m_hComponent;
    xiiVec3            m_vCameraPosition;
    xiiVec3            m_vCameraDirection;
  };

  mutable xiiMutex                          m_VisibleComponentsMutex;
  mutable xiiDynamicArray<VisibleComponent> m_VisibleComponents;

  xiiDynamicArray<xiiComponentHandle> m_ComponentsToUpdate;

  xiiDynamicArray<xiiProcGenInternal::PlacementTile, xiiAlignedAllocatorWrapper> m_ActiveTiles;
  xiiDynamicArray<xiiUInt32>                                                     m_FreeTiles;

  struct ProcessingTask
  {
    XII_ALWAYS_INLINE bool IsValid() const { return m_uiTileIndex != xiiInvalidIndex; }
    XII_ALWAYS_INLINE bool IsScheduled() const { return m_PlacementTaskGroupID.IsValid(); }
    XII_ALWAYS_INLINE void Invalidate()
    {
      m_uiScheduledFrame = -1;
      m_PlacementTaskGroupID.Invalidate();
      m_uiTileIndex = xiiInvalidIndex;
    }

    xiiUInt64                                              m_uiScheduledFrame;
    xiiUniquePtr<xiiProcGenInternal::PlacementData>        m_pData;
    xiiSharedPtr<xiiProcGenInternal::PreparePlacementTask> m_pPrepareTask;
    xiiSharedPtr<xiiProcGenInternal::PlacementTask>        m_pPlacementTask;
    xiiTaskGroupID                                         m_PlacementTaskGroupID;
    xiiUInt32                                              m_uiTileIndex;
  };

  xiiDynamicArray<ProcessingTask> m_ProcessingTasks;
  xiiDynamicArray<xiiUInt32>      m_FreeProcessingTasks;

  struct SortedProcessingTask
  {
    xiiUInt64 m_uiScheduledFrame = 0;
    xiiUInt32 m_uiTaskIndex      = 0;
  };

  xiiDynamicArray<SortedProcessingTask> m_SortedProcessingTasks;

  xiiDynamicArray<xiiProcGenInternal::PlacementTileDesc, xiiAlignedAllocatorWrapper> m_NewTiles;
  xiiTaskGroupID                                                                     m_UpdateTilesTaskGroupID;
};

//////////////////////////////////////////////////////////////////////////

struct xiiProcGenBoxExtents
{
  xiiVec3 m_vOffset  = xiiVec3::ZeroVector();
  xiiQuat m_Rotation = xiiQuat::IdentityQuaternion();
  xiiVec3 m_vExtents = xiiVec3(10);

  xiiResult Serialize(xiiStreamWriter& stream) const;
  xiiResult Deserialize(xiiStreamReader& stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PROCGENPLUGIN_DLL, xiiProcGenBoxExtents);

class XII_PROCGENPLUGIN_DLL xiiProcPlacementComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiProcPlacementComponent, xiiComponent, xiiProcPlacementComponentManager);

public:
  xiiProcPlacementComponent();
  ~xiiProcPlacementComponent();

  xiiProcPlacementComponent& operator=(xiiProcPlacementComponent&& other);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void        SetResourceFile(const char* szFile);
  const char* GetResourceFile() const;

  void                                 SetResource(const xiiProcGenGraphResourceHandle& hResource);
  const xiiProcGenGraphResourceHandle& GetResource() const { return m_hResource; }

  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

private:
  xiiUInt32                   BoxExtents_GetCount() const;
  const xiiProcGenBoxExtents& BoxExtents_GetValue(xiiUInt32 uiIndex) const;
  void                        BoxExtents_SetValue(xiiUInt32 uiIndex, const xiiProcGenBoxExtents& value);
  void                        BoxExtents_Insert(xiiUInt32 uiIndex, const xiiProcGenBoxExtents& value);
  void                        BoxExtents_Remove(xiiUInt32 uiIndex);

  void UpdateBoundsAndTiles();

  xiiProcGenGraphResourceHandle m_hResource;

  xiiDynamicArray<xiiProcGenBoxExtents> m_BoxExtents;

  // runtime data
  friend class xiiProcGenInternal::FindPlacementTilesTask;

  struct Bounds
  {
    XII_DECLARE_POD_TYPE();

    xiiSimdBBox  m_GlobalBoundingBox;
    xiiSimdMat4f m_GlobalToLocalBoxTransform;
  };

  xiiDynamicArray<Bounds, xiiAlignedAllocatorWrapper> m_Bounds;

  struct OutputContext
  {
    xiiSharedPtr<const xiiProcGenInternal::PlacementOutput> m_pOutput;

    struct TileIndexAndAge
    {
      XII_DECLARE_POD_TYPE();

      xiiUInt32 m_uiIndex;
      xiiUInt64 m_uiLastSeenFrame;
    };

    xiiHashTable<xiiUInt64, TileIndexAndAge> m_TileIndices;

    xiiSharedPtr<xiiProcGenInternal::FindPlacementTilesTask> m_pUpdateTilesTask;
  };

  xiiDynamicArray<OutputContext> m_OutputContexts;
};
