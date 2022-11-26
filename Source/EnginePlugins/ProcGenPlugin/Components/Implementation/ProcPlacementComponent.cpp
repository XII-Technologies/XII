#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/Implementation/PlacementTile.h>
#include <ProcGenPlugin/Components/ProcPlacementComponent.h>
#include <ProcGenPlugin/Tasks/FindPlacementTilesTask.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PlacementTask.h>
#include <ProcGenPlugin/Tasks/PreparePlacementTask.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

using namespace xiiProcGenInternal;

xiiCVarInt  cvar_ProcGenProcessingMaxTiles("ProcGen.Processing.MaxTiles", 8, xiiCVarFlags::Default, "Maximum number of tiles in process");
xiiCVarInt  cvar_ProcGenProcessingMaxNewObjectsPerFrame("ProcGen.Processing.MaxNewObjectsPerFrame", 128, xiiCVarFlags::Default, "Maximum number of objects placed per frame");
xiiCVarBool cvar_ProcGenVisTiles("ProcGen.VisTiles", false, xiiCVarFlags::Default, "Enables debug visualization of procedural placement tiles");

xiiProcPlacementComponentManager::xiiProcPlacementComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiProcPlacementComponent, xiiBlockStorageType::Compact>(pWorld)
{
}

xiiProcPlacementComponentManager::~xiiProcPlacementComponentManager() {}

void xiiProcPlacementComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiProcPlacementComponentManager::FindTiles, this);
    desc.m_Phase     = xiiWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiProcPlacementComponentManager::PreparePlace, this);
    desc.m_Phase = xiiWorldModule::UpdateFunctionDesc::Phase::Async;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiProcPlacementComponentManager::PlaceObjects, this);
    desc.m_Phase = xiiWorldModule::UpdateFunctionDesc::Phase::PostAsync;

    this->RegisterUpdateFunction(desc);
  }

  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiProcPlacementComponentManager::OnResourceEvent, this));
}

void xiiProcPlacementComponentManager::Deinitialize()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiProcPlacementComponentManager::OnResourceEvent, this));

  for (auto& activeTile : m_ActiveTiles)
  {
    activeTile.Deinitialize(*GetWorld());
  }
  m_ActiveTiles.Clear();

  SUPER::Deinitialize();
}

void xiiProcPlacementComponentManager::FindTiles(const xiiWorldModule::UpdateContext& context)
{
  // Update resource data
  bool bAnyObjectsRemoved = false;

  for (auto& hComponent : m_ComponentsToUpdate)
  {
    xiiProcPlacementComponent* pComponent = nullptr;
    if (!TryGetComponent(hComponent, pComponent))
    {
      continue;
    }

    RemoveTilesForComponent(pComponent, &bAnyObjectsRemoved);

    xiiResourceLock<xiiProcGenGraphResource> pResource(pComponent->m_hResource, xiiResourceAcquireMode::BlockTillLoaded);
    auto                                     outputs = pResource->GetPlacementOutputs();

    pComponent->m_OutputContexts.Clear();
    for (xiiUInt32 uiIndex = 0; uiIndex < outputs.GetCount(); ++uiIndex)
    {
      const auto& pOutput = outputs[uiIndex];
      if (pOutput->IsValid())
      {
        auto& outputContext              = pComponent->m_OutputContexts.ExpandAndGetRef();
        outputContext.m_pOutput          = pOutput;
        outputContext.m_pUpdateTilesTask = XII_DEFAULT_NEW(FindPlacementTilesTask, pComponent, uiIndex);
      }
    }
  }
  m_ComponentsToUpdate.Clear();

  // If we removed any objects during resource update do nothing else this frame so objects are actually deleted before we place new ones.
  if (bAnyObjectsRemoved)
  {
    return;
  }

  // Schedule find tiles tasks
  m_UpdateTilesTaskGroupID = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::EarlyThisFrame);

  for (auto& visibleComponent : m_VisibleComponents)
  {
    xiiProcPlacementComponent* pComponent = nullptr;
    if (!TryGetComponent(visibleComponent.m_hComponent, pComponent))
    {
      continue;
    }

    auto& outputContexts = pComponent->m_OutputContexts;
    for (auto& outputContext : outputContexts)
    {
      outputContext.m_pUpdateTilesTask->AddCameraPosition(visibleComponent.m_vCameraPosition);

      if (outputContext.m_pUpdateTilesTask->IsTaskFinished())
      {
        xiiTaskSystem::AddTaskToGroup(m_UpdateTilesTaskGroupID, outputContext.m_pUpdateTilesTask);
      }
    }
  }

  xiiTaskSystem::StartTaskGroup(m_UpdateTilesTaskGroupID);
}

void xiiProcPlacementComponentManager::PreparePlace(const xiiWorldModule::UpdateContext& context)
{
  // Find new active tiles and remove old ones
  {
    XII_PROFILE_SCOPE("Add new/remove old tiles");

    xiiTaskSystem::WaitForGroup(m_UpdateTilesTaskGroupID);
    m_UpdateTilesTaskGroupID.Invalidate();

    for (auto& visibleComponent : m_VisibleComponents)
    {
      xiiProcPlacementComponent* pComponent = nullptr;
      if (!TryGetComponent(visibleComponent.m_hComponent, pComponent))
      {
        continue;
      }

      auto& outputContexts = pComponent->m_OutputContexts;
      for (auto& outputContext : outputContexts)
      {
        auto oldTiles = outputContext.m_pUpdateTilesTask->GetOldTiles();
        for (xiiUInt64 uiOldTileKey : oldTiles)
        {
          xiiProcPlacementComponent::OutputContext::TileIndexAndAge tileIndex;
          if (outputContext.m_TileIndices.Remove(uiOldTileKey, &tileIndex))
          {
            if (tileIndex.m_uiIndex != NewTileIndex)
            {
              DeallocateTile(tileIndex.m_uiIndex);
            }
          }

          // Also remove from new tiles list
          for (xiiUInt32 i = 0; i < m_NewTiles.GetCount(); ++i)
          {
            auto&     newTile   = m_NewTiles[i];
            xiiUInt64 uiTileKey = GetTileKey(newTile.m_iPosX, newTile.m_iPosY);
            if (uiTileKey == uiOldTileKey)
            {
              m_NewTiles.RemoveAtAndSwap(i);
              break;
            }
          }
        }

        m_NewTiles.PushBackRange(outputContext.m_pUpdateTilesTask->GetNewTiles());
      }
    }

    // Sort new tiles
    {
      XII_PROFILE_SCOPE("Sort new tiles");

      // Update distance to camera
      for (auto& newTile : m_NewTiles)
      {
        xiiVec2 tilePos             = xiiVec2((float)newTile.m_iPosX, (float)newTile.m_iPosY);
        newTile.m_fDistanceToCamera = xiiMath::MaxValue<float>();

        for (auto& visibleComponent : m_VisibleComponents)
        {
          xiiVec2 cameraPos = visibleComponent.m_vCameraPosition.GetAsVec2() / newTile.m_fTileSize;

          float fDistance             = (tilePos - cameraPos).GetLengthSquared();
          newTile.m_fDistanceToCamera = xiiMath::Min(newTile.m_fDistanceToCamera, fDistance);
        }
      }

      // Sort by distance, larger distances come first since new tiles are processed in reverse order.
      m_NewTiles.Sort([](auto& tileA, auto& tileB) { return tileA.m_fDistanceToCamera > tileB.m_fDistanceToCamera; });
    }

    ClearVisibleComponents();
  }

  // Debug draw tiles
  if (cvar_ProcGenVisTiles)
  {
    xiiStringBuilder sb;
    sb.Format("Procedural Placement Stats:\nNum Tiles to process: {}", m_NewTiles.GetCount());

    xiiColor textColor = xiiColorScheme::LightUI(xiiColorScheme::Grape);
    xiiDebugRenderer::DrawInfoText(GetWorld(), xiiDebugRenderer::ScreenPlacement::TopLeft, "ProcPlaceStats", sb, textColor);

    for (xiiUInt32 i = 0; i < m_NewTiles.GetCount(); ++i)
    {
      DebugDrawTile(m_NewTiles[i], textColor, m_NewTiles.GetCount() - i - 1);
    }

    for (auto& activeTile : m_ActiveTiles)
    {
      if (!activeTile.IsValid())
        continue;

      DebugDrawTile(activeTile.GetDesc(), activeTile.GetDebugColor());
    }
  }

  // Allocate new tiles and placement tasks
  {
    XII_PROFILE_SCOPE("Allocate new tiles");

    while (!m_NewTiles.IsEmpty() && GetNumAllocatedProcessingTasks() < (xiiUInt32)cvar_ProcGenProcessingMaxTiles)
    {
      const PlacementTileDesc& newTile = m_NewTiles.PeekBack();

      xiiProcPlacementComponent* pComponent = nullptr;
      if (TryGetComponent(newTile.m_hComponent, pComponent))
      {
        auto&     pOutput        = pComponent->m_OutputContexts[newTile.m_uiOutputIndex].m_pOutput;
        xiiUInt32 uiNewTileIndex = AllocateTile(newTile, pOutput);

        AllocateProcessingTask(uiNewTileIndex);
      }

      m_NewTiles.PopBack();
    }
  }

  const xiiWorld* pWorld = GetWorld();

  // Update processing tasks
  if (GetWorldSimulationEnabled())
  {
    {
      XII_PROFILE_SCOPE("Prepare processing tasks");

      xiiTaskGroupID prepareTaskGroupID = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::EarlyThisFrame);

      for (auto& processingTask : m_ProcessingTasks)
      {
        if (!processingTask.IsValid() || processingTask.IsScheduled())
          continue;

        auto& activeTile = m_ActiveTiles[processingTask.m_uiTileIndex];
        activeTile.PreparePlacementData(pWorld, pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>(), *processingTask.m_pData);

        xiiTaskSystem::AddTaskToGroup(prepareTaskGroupID, processingTask.m_pPrepareTask);
      }

      xiiTaskSystem::StartTaskGroup(prepareTaskGroupID);
      xiiTaskSystem::WaitForGroup(prepareTaskGroupID);
    }

    {
      XII_PROFILE_SCOPE("Kickoff placement tasks");

      for (auto& processingTask : m_ProcessingTasks)
      {
        if (!processingTask.IsValid() || processingTask.IsScheduled())
          continue;

        processingTask.m_uiScheduledFrame     = xiiRenderWorld::GetFrameCounter();
        processingTask.m_PlacementTaskGroupID = xiiTaskSystem::StartSingleTask(processingTask.m_pPlacementTask, xiiTaskPriority::LongRunningHighPriority);
      }
    }
  }
}

void xiiProcPlacementComponentManager::PlaceObjects(const xiiWorldModule::UpdateContext& context)
{
  m_SortedProcessingTasks.Clear();
  for (xiiUInt32 i = 0; i < m_ProcessingTasks.GetCount(); ++i)
  {
    auto& sortedTask              = m_SortedProcessingTasks.ExpandAndGetRef();
    sortedTask.m_uiScheduledFrame = m_ProcessingTasks[i].m_uiScheduledFrame;
    sortedTask.m_uiTaskIndex      = i;
  }

  m_SortedProcessingTasks.Sort([](auto& taskA, auto& taskB) { return taskA.m_uiScheduledFrame < taskB.m_uiScheduledFrame; });

  xiiUInt32 uiTotalNumPlacedObjects = 0;

  for (auto& sortedTask : m_SortedProcessingTasks)
  {
    auto& task = m_ProcessingTasks[sortedTask.m_uiTaskIndex];
    if (!task.IsValid() || !task.IsScheduled())
      continue;

    if (task.m_pPlacementTask->IsTaskFinished())
    {
      xiiUInt32 uiPlacedObjects = 0;

      xiiUInt32 uiTileIndex = task.m_uiTileIndex;
      auto&     activeTile  = m_ActiveTiles[uiTileIndex];

      auto&                      tileDesc   = activeTile.GetDesc();
      xiiProcPlacementComponent* pComponent = nullptr;
      if (TryGetComponent(tileDesc.m_hComponent, pComponent))
      {
        auto& outputContext = pComponent->m_OutputContexts[tileDesc.m_uiOutputIndex];

        xiiUInt64 uiTileKey = GetTileKey(tileDesc.m_iPosX, tileDesc.m_iPosY);
        if (auto pTile = outputContext.m_TileIndices.GetValue(uiTileKey))
        {
          uiPlacedObjects = activeTile.PlaceObjects(*GetWorld(), task.m_pPlacementTask->GetOutputTransforms());

          pTile->m_uiIndex         = uiPlacedObjects > 0 ? uiTileIndex : EmptyTileIndex;
          pTile->m_uiLastSeenFrame = xiiRenderWorld::GetFrameCounter();
        }
      }

      if (uiPlacedObjects == 0)
      {
        // mark tile for re-use
        DeallocateTile(uiTileIndex);
      }

      // mark task for re-use
      DeallocateProcessingTask(sortedTask.m_uiTaskIndex);

      uiTotalNumPlacedObjects += uiPlacedObjects;
    }

    if (uiTotalNumPlacedObjects >= (xiiUInt32)cvar_ProcGenProcessingMaxNewObjectsPerFrame)
    {
      break;
    }
  }
}

void xiiProcPlacementComponentManager::DebugDrawTile(const xiiProcGenInternal::PlacementTileDesc& desc, const xiiColor& color, xiiUInt32 uiQueueIndex)
{
  const xiiProcPlacementComponent* pComponent = nullptr;
  if (!TryGetComponent(desc.m_hComponent, pComponent))
    return;

  xiiBoundingBox bbox = desc.GetBoundingBox();
  xiiDebugRenderer::DrawLineBox(GetWorld(), bbox, color);

  xiiUInt64 uiAge         = -1;
  auto&     outputContext = pComponent->m_OutputContexts[desc.m_uiOutputIndex];
  if (auto pTile = outputContext.m_TileIndices.GetValue(GetTileKey(desc.m_iPosX, desc.m_iPosY)))
  {
    uiAge = xiiRenderWorld::GetFrameCounter() - pTile->m_uiLastSeenFrame;
  }

  xiiStringBuilder sb;
  if (uiQueueIndex != xiiInvalidIndex)
  {
    sb.Format("Queue Index: {}\n", uiQueueIndex);
  }
  sb.AppendFormat("Age: {}\nDistance: {}", uiAge, desc.m_fDistanceToCamera);
  xiiDebugRenderer::Draw3DText(GetWorld(), sb, bbox.GetCenter(), color);
}

void xiiProcPlacementComponentManager::AddComponent(xiiProcPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
}

void xiiProcPlacementComponentManager::RemoveComponent(xiiProcPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  RemoveTilesForComponent(pComponent);
}

xiiUInt32 xiiProcPlacementComponentManager::AllocateTile(const PlacementTileDesc& desc, xiiSharedPtr<const PlacementOutput>& pOutput)
{
  xiiUInt32 uiNewTileIndex = xiiInvalidIndex;
  if (!m_FreeTiles.IsEmpty())
  {
    uiNewTileIndex = m_FreeTiles.PeekBack();
    m_FreeTiles.PopBack();
  }
  else
  {
    uiNewTileIndex = m_ActiveTiles.GetCount();
    m_ActiveTiles.ExpandAndGetRef();
  }

  m_ActiveTiles[uiNewTileIndex].Initialize(desc, pOutput);
  return uiNewTileIndex;
}

void xiiProcPlacementComponentManager::DeallocateTile(xiiUInt32 uiTileIndex)
{
  m_ActiveTiles[uiTileIndex].Deinitialize(*GetWorld());
  m_FreeTiles.PushBack(uiTileIndex);
}

xiiUInt32 xiiProcPlacementComponentManager::AllocateProcessingTask(xiiUInt32 uiTileIndex)
{
  xiiUInt32 uiNewTaskIndex = xiiInvalidIndex;
  if (!m_FreeProcessingTasks.IsEmpty())
  {
    uiNewTaskIndex = m_FreeProcessingTasks.PeekBack();
    m_FreeProcessingTasks.PopBack();
  }
  else
  {
    uiNewTaskIndex = m_ProcessingTasks.GetCount();
    auto& newTask  = m_ProcessingTasks.ExpandAndGetRef();

    newTask.m_pData = XII_DEFAULT_NEW(PlacementData);

    xiiStringBuilder sName;
    sName.Format("Prepare Task {}", uiNewTaskIndex);
    newTask.m_pPrepareTask = XII_DEFAULT_NEW(PreparePlacementTask, newTask.m_pData.Borrow(), sName);

    sName.Format("Placement Task {}", uiNewTaskIndex);
    newTask.m_pPlacementTask = XII_DEFAULT_NEW(PlacementTask, newTask.m_pData.Borrow(), sName);
  }

  m_ProcessingTasks[uiNewTaskIndex].m_uiTileIndex = uiTileIndex;
  return uiNewTaskIndex;
}

void xiiProcPlacementComponentManager::DeallocateProcessingTask(xiiUInt32 uiTaskIndex)
{
  auto& task = m_ProcessingTasks[uiTaskIndex];
  if (task.IsScheduled())
  {
    xiiTaskSystem::WaitForGroup(task.m_PlacementTaskGroupID);
  }

  task.m_pData->Clear();
  task.m_pPrepareTask->Clear();
  task.m_pPlacementTask->Clear();
  task.Invalidate();

  m_FreeProcessingTasks.PushBack(uiTaskIndex);
}

xiiUInt32 xiiProcPlacementComponentManager::GetNumAllocatedProcessingTasks() const
{
  return m_ProcessingTasks.GetCount() - m_FreeProcessingTasks.GetCount();
}

void xiiProcPlacementComponentManager::RemoveTilesForComponent(xiiProcPlacementComponent* pComponent, bool* out_bAnyObjectsRemoved /*= nullptr*/)
{
  xiiComponentHandle hComponent = pComponent->GetHandle();

  for (xiiUInt32 uiNewTileIndex = 0; uiNewTileIndex < m_NewTiles.GetCount(); ++uiNewTileIndex)
  {
    if (m_NewTiles[uiNewTileIndex].m_hComponent == hComponent)
    {
      m_NewTiles.RemoveAtAndSwap(uiNewTileIndex);
      --uiNewTileIndex;
    }
  }

  for (xiiUInt32 uiTileIndex = 0; uiTileIndex < m_ActiveTiles.GetCount(); ++uiTileIndex)
  {
    auto& activeTile = m_ActiveTiles[uiTileIndex];
    if (!activeTile.IsValid())
      continue;

    auto& tileDesc = activeTile.GetDesc();
    if (tileDesc.m_hComponent == hComponent)
    {
      if (out_bAnyObjectsRemoved != nullptr && !m_ActiveTiles[uiTileIndex].GetPlacedObjects().IsEmpty())
      {
        *out_bAnyObjectsRemoved = true;
      }

      DeallocateTile(uiTileIndex);

      for (xiiUInt32 i = 0; i < m_ProcessingTasks.GetCount(); ++i)
      {
        auto& taskInfo = m_ProcessingTasks[i];
        if (taskInfo.m_uiTileIndex == uiTileIndex)
        {
          DeallocateProcessingTask(i);
        }
      }
    }
  }
}

void xiiProcPlacementComponentManager::OnResourceEvent(const xiiResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != xiiResourceEvent::Type::ResourceContentUnloading)
    return;

  if (auto pResource = xiiDynamicCast<const xiiProcGenGraphResource*>(resourceEvent.m_pResource))
  {
    xiiProcGenGraphResourceHandle hResource = pResource->GetResourceHandle();

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource && !m_ComponentsToUpdate.Contains(it->GetHandle()))
      {
        m_ComponentsToUpdate.PushBack(it->GetHandle());
      }
    }
  }
}

void xiiProcPlacementComponentManager::AddVisibleComponent(const xiiComponentHandle& hComponent, const xiiVec3& cameraPosition, const xiiVec3& cameraDirection) const
{
  XII_LOCK(m_VisibleComponentsMutex);

  for (auto& visibleComponent : m_VisibleComponents)
  {
    if (visibleComponent.m_hComponent == hComponent && visibleComponent.m_vCameraPosition == cameraPosition && visibleComponent.m_vCameraDirection == cameraDirection)
    {
      return;
    }
  }

  auto& visibleComponent              = m_VisibleComponents.ExpandAndGetRef();
  visibleComponent.m_hComponent       = hComponent;
  visibleComponent.m_vCameraPosition  = cameraPosition;
  visibleComponent.m_vCameraDirection = cameraDirection;
}

void xiiProcPlacementComponentManager::ClearVisibleComponents()
{
  m_VisibleComponents.Clear();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiProcGenBoxExtents, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiProcGenBoxExtents>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Offset", m_vOffset),
    XII_MEMBER_PROPERTY("Rotation", m_Rotation),
    XII_MEMBER_PROPERTY("Extents", m_vExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(10.0f)), new xiiClampValueAttribute(xiiVec3(0), xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiBoxManipulatorAttribute("Extents", 1.0f, false, "Offset", "Rotation"),
    new xiiBoxVisualizerAttribute("Extents", 1.0f, xiiColorScheme::LightUI(xiiColorScheme::Blue), nullptr, xiiVisualizerAnchor::Center, xiiVec3::OneVector(), "Offset", "Rotation"),
    new xiiTransformManipulatorAttribute("Offset", "Rotation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiProcPlacementComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_ProcGen_Graph")),
    XII_ARRAY_ACCESSOR_PROPERTY("BoxExtents", BoxExtents_GetCount, BoxExtents_GetValue, BoxExtents_SetValue, BoxExtents_Insert, BoxExtents_Remove),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Procedural Generation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiProcPlacementComponent::xiiProcPlacementComponent()        = default;
xiiProcPlacementComponent::~xiiProcPlacementComponent()       = default;
xiiProcPlacementComponent& xiiProcPlacementComponent::operator=(xiiProcPlacementComponent&& other) = default;

void xiiProcPlacementComponent::OnActivated()
{
  UpdateBoundsAndTiles();
}

void xiiProcPlacementComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  m_Bounds.Clear();
  m_OutputContexts.Clear();

  auto pManager = static_cast<xiiProcPlacementComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(this);
}

void xiiProcPlacementComponent::SetResourceFile(const char* szFile)
{
  xiiProcGenGraphResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiProcGenGraphResource>(szFile);
    xiiResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

const char* xiiProcPlacementComponent::GetResourceFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void xiiProcPlacementComponent::SetResource(const xiiProcGenGraphResourceHandle& hResource)
{
  auto pManager = static_cast<xiiProcPlacementComponentManager*>(GetOwningManager());

  if (IsActiveAndInitialized())
  {
    pManager->RemoveComponent(this);
  }

  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    pManager->AddComponent(this);
  }
}

void xiiProcPlacementComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  if (m_BoxExtents.IsEmpty())
    return;

  xiiBoundingBoxSphere bounds;
  bounds.SetInvalid();

  for (auto& boxExtent : m_BoxExtents)
  {
    xiiBoundingBoxSphere localBox = xiiBoundingBox(-boxExtent.m_vExtents * 0.5f, boxExtent.m_vExtents * 0.5f);
    localBox.Transform(xiiTransform(boxExtent.m_vOffset, boxExtent.m_Rotation).GetAsMat4());

    bounds.ExpandToInclude(localBox);
  }

  msg.AddBounds(bounds, GetOwner()->IsDynamic() ? xiiDefaultSpatialDataCategories::RenderDynamic : xiiDefaultSpatialDataCategories::RenderStatic);
}

void xiiProcPlacementComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // Don't extract render data for selection or in shadow views.
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory)
    return;

  if (msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::MainView || msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::EditorView)
  {
    const xiiCamera* pCamera = msg.m_pView->GetCullingCamera();

    xiiVec3 cameraPosition  = pCamera->GetCenterPosition();
    xiiVec3 cameraDirection = pCamera->GetCenterDirForwards();

    if (m_hResource.IsValid())
    {
      auto pManager = static_cast<const xiiProcPlacementComponentManager*>(GetOwningManager());
      pManager->AddVisibleComponent(GetHandle(), cameraPosition, cameraDirection);
    }
  }
}

void xiiProcPlacementComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  xiiStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_BoxExtents).IgnoreResult();
}

void xiiProcPlacementComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = stream.GetStream();

  s >> m_hResource;
  s.ReadArray(m_BoxExtents).IgnoreResult();
}

xiiUInt32 xiiProcPlacementComponent::BoxExtents_GetCount() const
{
  return m_BoxExtents.GetCount();
}

const xiiProcGenBoxExtents& xiiProcPlacementComponent::BoxExtents_GetValue(xiiUInt32 uiIndex) const
{
  return m_BoxExtents[uiIndex];
}

void xiiProcPlacementComponent::BoxExtents_SetValue(xiiUInt32 uiIndex, const xiiProcGenBoxExtents& value)
{
  m_BoxExtents.EnsureCount(uiIndex + 1);
  m_BoxExtents[uiIndex] = value;

  UpdateBoundsAndTiles();
}

void xiiProcPlacementComponent::BoxExtents_Insert(xiiUInt32 uiIndex, const xiiProcGenBoxExtents& value)
{
  m_BoxExtents.Insert(value, uiIndex);

  UpdateBoundsAndTiles();
}

void xiiProcPlacementComponent::BoxExtents_Remove(xiiUInt32 uiIndex)
{
  m_BoxExtents.RemoveAtAndCopy(uiIndex);

  UpdateBoundsAndTiles();
}

void xiiProcPlacementComponent::UpdateBoundsAndTiles()
{
  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<xiiProcPlacementComponentManager*>(GetOwningManager());

    pManager->RemoveComponent(this);

    GetOwner()->UpdateLocalBounds();

    m_Bounds.Clear();
    m_OutputContexts.Clear();

    xiiSimdTransform ownerTransform = GetOwner()->GetGlobalTransformSimd();
    for (auto& boxExtent : m_BoxExtents)
    {
      xiiSimdTransform localBoxTransform;
      localBoxTransform.m_Position = xiiSimdConversion::ToVec3(boxExtent.m_vOffset);
      localBoxTransform.m_Rotation = xiiSimdConversion::ToQuat(boxExtent.m_Rotation);
      localBoxTransform.m_Scale    = xiiSimdConversion::ToVec3(boxExtent.m_vExtents * 0.5f);

      xiiSimdTransform finalBoxTransform;
      finalBoxTransform.SetGlobalTransform(ownerTransform, localBoxTransform);

      xiiSimdMat4f finalBoxMat = finalBoxTransform.GetAsMat4();

      xiiSimdBBox globalBox(xiiSimdVec4f(-1.0f), xiiSimdVec4f(1.0f));
      globalBox.Transform(finalBoxMat);

      auto& bounds                       = m_Bounds.ExpandAndGetRef();
      bounds.m_GlobalBoundingBox         = globalBox;
      bounds.m_GlobalToLocalBoxTransform = finalBoxMat.GetInverse();
    }

    pManager->AddComponent(this);
  }
}

//////////////////////////////////////////////////////////////////////////

xiiResult xiiProcGenBoxExtents::Serialize(xiiStreamWriter& stream) const
{
  stream << m_vOffset;
  stream << m_Rotation;
  stream << m_vExtents;

  return XII_SUCCESS;
}

xiiResult xiiProcGenBoxExtents::Deserialize(xiiStreamReader& stream)
{
  stream >> m_vOffset;
  stream >> m_Rotation;
  stream >> m_vExtents;

  return XII_SUCCESS;
}
