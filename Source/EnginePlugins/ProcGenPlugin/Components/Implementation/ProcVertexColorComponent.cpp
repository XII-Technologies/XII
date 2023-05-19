#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcVertexColorRenderData, 1, xiiRTTIDefaultAllocator<xiiProcVertexColorRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiProcVertexColorRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_hVertexColorBuffer.GetInternalID().m_Data);
}

//////////////////////////////////////////////////////////////////////////

enum
{
  BUFFER_ACCESS_OFFSET_BITS = 28,
  BUFFER_ACCESS_OFFSET_MASK = (1 << BUFFER_ACCESS_OFFSET_BITS) - 1,

  VERTEX_COLOR_BUFFER_SIZE = 1024 * 1024
};

using namespace xiiProcGenInternal;

xiiProcVertexColorComponentManager::xiiProcVertexColorComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiProcVertexColorComponent, xiiBlockStorageType::Compact>(pWorld)
{
}

xiiProcVertexColorComponentManager::~xiiProcVertexColorComponentManager() = default;

void xiiProcVertexColorComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiStructSize                = sizeof(xiiUInt32);
    desc.m_uiTotalSize                 = desc.m_uiStructSize * VERTEX_COLOR_BUFFER_SIZE;
    desc.m_bAllowShaderResourceView    = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hVertexColorBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);

    m_VertexColorData.SetCountUninitialized(VERTEX_COLOR_BUFFER_SIZE);
  }

  {
    auto desc        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiProcVertexColorComponentManager::UpdateVertexColors, this);
    desc.m_Phase     = xiiWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  xiiRenderWorld::GetRenderEvent().AddEventHandler(xiiMakeDelegate(&xiiProcVertexColorComponentManager::OnRenderEvent, this));
  xiiRenderWorld::GetExtractionEvent().AddEventHandler(xiiMakeDelegate(&xiiProcVertexColorComponentManager::OnExtractionEvent, this));

  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiProcVertexColorComponentManager::OnResourceEvent, this));

  // TODO: also do this in xiiProcPlacementComponentManager
  xiiProcVolumeComponent::GetAreaInvalidatedEvent().AddEventHandler(xiiMakeDelegate(&xiiProcVertexColorComponentManager::OnAreaInvalidated, this));
}

void xiiProcVertexColorComponentManager::Deinitialize()
{
  xiiGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexColorBuffer);
  m_hVertexColorBuffer.Invalidate();

  xiiRenderWorld::GetRenderEvent().RemoveEventHandler(xiiMakeDelegate(&xiiProcVertexColorComponentManager::OnRenderEvent, this));
  xiiRenderWorld::GetExtractionEvent().RemoveEventHandler(xiiMakeDelegate(&xiiProcVertexColorComponentManager::OnExtractionEvent, this));

  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiProcVertexColorComponentManager::OnResourceEvent, this));

  xiiProcVolumeComponent::GetAreaInvalidatedEvent().RemoveEventHandler(xiiMakeDelegate(&xiiProcVertexColorComponentManager::OnAreaInvalidated, this));

  SUPER::Deinitialize();
}

void xiiProcVertexColorComponentManager::UpdateVertexColors(const xiiWorldModule::UpdateContext& context)
{
  m_UpdateTaskGroupID = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::EarlyThisFrame);
  m_ModifiedDataRange.Reset();

  for (const auto& componentToUpdate : m_ComponentsToUpdate)
  {
    xiiProcVertexColorComponent* pComponent = nullptr;
    if (!TryGetComponent(componentToUpdate, pComponent))
      continue;

    UpdateComponentVertexColors(pComponent);

    // Invalidate all cached render data so the new buffer handle and offset are propagated to the render data
    xiiRenderWorld::DeleteCachedRenderData(pComponent->GetOwner()->GetHandle(), pComponent->GetHandle());
  }

  m_ComponentsToUpdate.Clear();

  xiiTaskSystem::StartTaskGroup(m_UpdateTaskGroupID);
}

void xiiProcVertexColorComponentManager::UpdateComponentVertexColors(xiiProcVertexColorComponent* pComponent)
{
  pComponent->m_Outputs.Clear();
  xiiHybridArray<xiiProcVertexColorMapping, 2> outputMappings;

  {
    xiiResourceLock<xiiProcGenGraphResource> pResource(pComponent->m_hResource, xiiResourceAcquireMode::BlockTillLoaded);
    auto                                     outputs = pResource->GetVertexColorOutputs();

    for (auto& outputDesc : pComponent->m_OutputDescs)
    {
      if (!outputDesc.m_sName.IsEmpty())
      {
        bool bOutputFound = false;
        for (auto& pOutput : outputs)
        {
          if (pOutput->m_sName == outputDesc.m_sName)
          {
            pComponent->m_Outputs.PushBack(pOutput);
            bOutputFound = true;
            break;
          }
        }

        if (!bOutputFound)
        {
          pComponent->m_Outputs.PushBack(nullptr);
          xiiLog::Error("Vertex Color Output with name '{}' not found in Proc Gen Graph '{}'", outputDesc.m_sName, pResource->GetResourceID());
        }
      }
      else
      {
        pComponent->m_Outputs.PushBack(nullptr);
      }

      outputMappings.PushBack(outputDesc.m_Mapping);
    }
  }

  if (!pComponent->HasValidOutputs())
    return;

  const char* szMesh = pComponent->GetMeshFile();
  if (xiiStringUtils::IsNullOrEmpty(szMesh))
    return;

  xiiCpuMeshResourceHandle            hCpuMesh = xiiResourceManager::LoadResource<xiiCpuMeshResource>(szMesh);
  xiiResourceLock<xiiCpuMeshResource> pCpuMesh(hCpuMesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCpuMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
  {
    xiiLog::Warning("Failed to retrieve CPU mesh '{}'", szMesh);
    return;
  }

  const auto&     mbDesc             = pCpuMesh->GetDescriptor().MeshBufferDesc();
  const xiiUInt32 uiNumOutputs       = pComponent->m_Outputs.GetCount();
  const xiiUInt32 uiVertexColorCount = mbDesc.GetVertexCount() * uiNumOutputs;

  pComponent->m_hVertexColorBuffer = m_hVertexColorBuffer;

  if (pComponent->m_uiBufferAccessData == 0)
  {
    pComponent->m_uiBufferAccessData = (uiNumOutputs << BUFFER_ACCESS_OFFSET_BITS) | m_uiCurrentBufferOffset;
    m_uiCurrentBufferOffset += uiVertexColorCount;
  }

  const xiiUInt32 uiBufferOffset = pComponent->m_uiBufferAccessData & BUFFER_ACCESS_OFFSET_MASK;
  m_ModifiedDataRange.SetToIncludeRange(uiBufferOffset, uiBufferOffset + uiVertexColorCount - 1);

  if (m_uiNextTaskIndex >= m_UpdateTasks.GetCount())
  {
    m_UpdateTasks.PushBack(XII_DEFAULT_NEW(xiiProcGenInternal::VertexColorTask));
  }

  auto& pUpdateTask = m_UpdateTasks[m_uiNextTaskIndex];

  xiiStringBuilder taskName = "VertexColor ";
  taskName.Append(pCpuMesh->GetResourceDescription().GetView());
  pUpdateTask->ConfigureTask(taskName, xiiTaskNesting::Never);

  pUpdateTask->Prepare(*GetWorld(), mbDesc, pComponent->GetOwner()->GetGlobalTransform(), pComponent->m_Outputs, outputMappings, m_VertexColorData.GetArrayPtr().GetSubArray(uiBufferOffset, uiVertexColorCount));

  xiiTaskSystem::AddTaskToGroup(m_UpdateTaskGroupID, pUpdateTask);

  ++m_uiNextTaskIndex;
}

void xiiProcVertexColorComponentManager::OnExtractionEvent(const xiiRenderWorldExtractionEvent& e)
{
  if (e.m_Type != xiiRenderWorldExtractionEvent::Type::EndExtraction)
    return;

  xiiTaskSystem::WaitForGroup(m_UpdateTaskGroupID);
  m_UpdateTaskGroupID.Invalidate();
  m_uiNextTaskIndex = 0;

  if (m_ModifiedDataRange.IsValid())
  {
    auto& dataCopy  = m_DataCopy[xiiRenderWorld::GetDataIndexForExtraction()];
    dataCopy.m_Data = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiUInt32, m_ModifiedDataRange.GetCount());
    dataCopy.m_Data.CopyFrom(m_VertexColorData.GetArrayPtr().GetSubArray(m_ModifiedDataRange.m_uiMin, m_ModifiedDataRange.GetCount()));
    dataCopy.m_uiStart = m_ModifiedDataRange.m_uiMin;
  }
}

void xiiProcVertexColorComponentManager::OnRenderEvent(const xiiRenderWorldRenderEvent& e)
{
  if (e.m_Type != xiiRenderWorldRenderEvent::Type::BeginRender)
    return;

  auto& dataCopy = m_DataCopy[xiiRenderWorld::GetDataIndexForRendering()];
  if (!dataCopy.m_Data.IsEmpty())
  {
    xiiGALDevice*                pGALDevice         = xiiGALDevice::GetDefaultDevice();
    xiiGALPass*                  pGALPass           = pGALDevice->BeginPass("ProcVertexUpdate");
    xiiGALComputeCommandEncoder* pGALCommandEncoder = pGALPass->BeginCompute();

    xiiUInt32 uiByteOffset = dataCopy.m_uiStart * sizeof(xiiUInt32);
    pGALCommandEncoder->UpdateBuffer(m_hVertexColorBuffer, uiByteOffset, dataCopy.m_Data.ToByteArray(), xiiGALUpdateMode::Discard);

    dataCopy = DataCopy();

    pGALPass->EndCompute(pGALCommandEncoder);
    pGALDevice->EndPass(pGALPass);
  }
}

void xiiProcVertexColorComponentManager::EnqueueUpdate(xiiProcVertexColorComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  if (!m_ComponentsToUpdate.Contains(pComponent->GetHandle()))
  {
    m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
  }
}

void xiiProcVertexColorComponentManager::RemoveComponent(xiiProcVertexColorComponent* pComponent)
{
  m_ComponentsToUpdate.RemoveAndSwap(pComponent->GetHandle());

  if (pComponent->m_uiBufferAccessData != 0)
  {
    /// \todo compact buffer somehow?

    pComponent->m_uiBufferAccessData = 0;
  }
}

void xiiProcVertexColorComponentManager::OnResourceEvent(const xiiResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != xiiResourceEvent::Type::ResourceContentUnloading)
    return;

  if (auto pResource = xiiDynamicCast<const xiiProcGenGraphResource*>(resourceEvent.m_pResource))
  {
    xiiProcGenGraphResourceHandle hResource = pResource->GetResourceHandle();

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource)
      {
        EnqueueUpdate(it);
      }
    }
  }
}

void xiiProcVertexColorComponentManager::OnAreaInvalidated(const xiiProcGenInternal::InvalidatedArea& area)
{
  if (area.m_pWorld != GetWorld())
    return;

  xiiSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = xiiDefaultSpatialDataCategories::RenderStatic.GetBitmask() | xiiDefaultSpatialDataCategories::RenderDynamic.GetBitmask();

  GetWorld()->GetSpatialSystem()->FindObjectsInBox(area.m_Box, queryParams, [this](xiiGameObject* pObject) {
    xiiHybridArray<xiiProcVertexColorComponent*, 8> components;
    pObject->TryGetComponentsOfBaseType(components);

    for (auto pComponent : components)
    {
      EnqueueUpdate(pComponent);
    }

    return xiiVisitorExecution::Continue;
  });
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiProcVertexColorOutputDesc, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiProcVertexColorOutputDesc>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetName, SetName),
    XII_MEMBER_PROPERTY("Mapping", m_Mapping),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiProcVertexColorOutputDesc::SetName(const char* szName)
{
  m_sName.Assign(szName);
}

static xiiTypeVersion s_ProcVertexColorOutputDescVersion = 1;
xiiResult             xiiProcVertexColorOutputDesc::Serialize(xiiStreamWriter& stream) const
{
  stream.WriteVersion(s_ProcVertexColorOutputDescVersion);
  stream << m_sName;
  XII_SUCCEED_OR_RETURN(m_Mapping.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiProcVertexColorOutputDesc::Deserialize(xiiStreamReader& stream)
{
  /*xiiTypeVersion version =*/stream.ReadVersion(s_ProcVertexColorOutputDescVersion);
  stream >> m_sName;
  XII_SUCCEED_OR_RETURN(m_Mapping.Deserialize(stream));

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiProcVertexColorComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_ProcGen_Graph")),
    XII_ARRAY_ACCESSOR_PROPERTY("OutputDescs", OutputDescs_GetCount, GetOutputDesc, SetOutputDesc, OutputDescs_Insert, OutputDescs_Remove),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgTransformChanged, OnTransformChanged)
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

xiiProcVertexColorComponent::xiiProcVertexColorComponent()  = default;
xiiProcVertexColorComponent::~xiiProcVertexColorComponent() = default;

void xiiProcVertexColorComponent::OnActivated()
{
  SUPER::OnActivated();

  auto pManager = static_cast<xiiProcVertexColorComponentManager*>(GetOwningManager());
  pManager->EnqueueUpdate(this);

  GetOwner()->EnableStaticTransformChangesNotifications();
}

void xiiProcVertexColorComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  auto pManager = static_cast<xiiProcVertexColorComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(this);

  // Don't disable notifications as other components attached to the owner game object might need them too.
  // GetOwner()->DisableStaticTransformChangesNotifications();
}

void xiiProcVertexColorComponent::SetResourceFile(const char* szFile)
{
  xiiProcGenGraphResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiProcGenGraphResource>(szFile);
    xiiResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

const char* xiiProcVertexColorComponent::GetResourceFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void xiiProcVertexColorComponent::SetResource(const xiiProcGenGraphResourceHandle& hResource)
{
  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<xiiProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

const xiiProcVertexColorOutputDesc& xiiProcVertexColorComponent::GetOutputDesc(xiiUInt32 uiIndex) const
{
  return m_OutputDescs[uiIndex];
}

void xiiProcVertexColorComponent::SetOutputDesc(xiiUInt32 uiIndex, const xiiProcVertexColorOutputDesc& outputDesc)
{
  m_OutputDescs.EnsureCount(uiIndex + 1);
  m_OutputDescs[uiIndex] = outputDesc;

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<xiiProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

void xiiProcVertexColorComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  xiiStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_OutputDescs).IgnoreResult();
}

void xiiProcVertexColorComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32  uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = stream.GetStream();

  s >> m_hResource;
  if (uiVersion >= 2)
  {
    s.ReadArray(m_OutputDescs).IgnoreResult();
  }
  else
  {
    xiiHybridArray<xiiHashedString, 2> outputNames;
    s.ReadArray(outputNames).IgnoreResult();

    for (auto& outputName : outputNames)
    {
      auto& outputDesc   = m_OutputDescs.ExpandAndGetRef();
      outputDesc.m_sName = outputName;
    }
  }
}

void xiiProcVertexColorComponent::OnTransformChanged(xiiMsgTransformChanged& msg)
{
  auto pManager = static_cast<xiiProcVertexColorComponentManager*>(GetOwningManager());
  pManager->EnqueueUpdate(this);
}

xiiMeshRenderData* xiiProcVertexColorComponent::CreateRenderData() const
{
  auto pRenderData = xiiCreateRenderDataForThisFrame<xiiProcVertexColorRenderData>(GetOwner());

  if (HasValidOutputs() && m_uiBufferAccessData != 0)
  {
    pRenderData->m_hVertexColorBuffer = m_hVertexColorBuffer;
    pRenderData->m_uiBufferAccessData = m_uiBufferAccessData;
  }

  return pRenderData;
}

xiiUInt32 xiiProcVertexColorComponent::OutputDescs_GetCount() const
{
  return m_OutputDescs.GetCount();
}

void xiiProcVertexColorComponent::OutputDescs_Insert(xiiUInt32 uiIndex, const xiiProcVertexColorOutputDesc& outputDesc)
{
  m_OutputDescs.Insert(outputDesc, uiIndex);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<xiiProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

void xiiProcVertexColorComponent::OutputDescs_Remove(xiiUInt32 uiIndex)
{
  m_OutputDescs.RemoveAtAndCopy(uiIndex);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<xiiProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

bool xiiProcVertexColorComponent::HasValidOutputs() const
{
  for (auto& pOutput : m_Outputs)
  {
    if (pOutput != nullptr && pOutput->m_pByteCode != nullptr)
    {
      return true;
    }
  }

  return false;
}
