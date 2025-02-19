#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Meshes/InstancedMeshComponent.h>
#include <GraphicsCore/Pipeline/InstanceDataProvider.h>
#include <GraphicsCore/Utils/WorldGeoExtractionUtil.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMeshInstanceData, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMeshInstanceData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    XII_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1.0f, 1.0f, 1.0f))),

    XII_MEMBER_PROPERTY("Color", m_color)
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiTransformManipulatorAttribute("LocalPosition", "LocalRotation", "LocalScaling"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE
// clang-format on

void xiiMeshInstanceData::SetLocalPosition(xiiVec3 vPosition)
{
  m_transform.m_vPosition = vPosition;
}
xiiVec3 xiiMeshInstanceData::GetLocalPosition() const
{
  return m_transform.m_vPosition;
}

void xiiMeshInstanceData::SetLocalRotation(xiiQuat qRotation)
{
  m_transform.m_qRotation = qRotation;
}

xiiQuat xiiMeshInstanceData::GetLocalRotation() const
{
  return m_transform.m_qRotation;
}

void xiiMeshInstanceData::SetLocalScaling(xiiVec3 vScaling)
{
  m_transform.m_vScale = vScaling;
}

xiiVec3 xiiMeshInstanceData::GetLocalScaling() const
{
  return m_transform.m_vScale;
}

static const xiiTypeVersion s_MeshInstanceDataVersion = 1;
xiiResult                   xiiMeshInstanceData::Serialize(xiiStreamWriter& ref_writer) const
{
  ref_writer.WriteVersion(s_MeshInstanceDataVersion);

  ref_writer << m_transform;
  ref_writer << m_color;

  return XII_SUCCESS;
}

xiiResult xiiMeshInstanceData::Deserialize(xiiStreamReader& ref_reader)
{
  /*auto version = */ ref_reader.ReadVersion(s_MeshInstanceDataVersion);

  ref_reader >> m_transform;
  ref_reader >> m_color;

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInstancedMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiInstancedMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////////////////

xiiInstancedMeshComponentManager::xiiInstancedMeshComponentManager(xiiWorld* pWorld) :
  SUPER(pWorld)
{
}

void xiiInstancedMeshComponentManager::EnqueueUpdate(const xiiInstancedMeshComponent* pComponent) const
{
  xiiUInt64 uiCurrentFrame = xiiRenderWorld::GetFrameCounter();
  if (pComponent->m_uiEnqueuedFrame == uiCurrentFrame)
    return;

  XII_LOCK(m_Mutex);
  if (pComponent->m_uiEnqueuedFrame == uiCurrentFrame)
    return;

  auto instanceData = pComponent->GetInstanceData();
  if (instanceData.IsEmpty())
    return;

  m_RequireUpdate.PushBack({pComponent->GetHandle(), instanceData});
  pComponent->m_uiEnqueuedFrame = uiCurrentFrame;
}

void xiiInstancedMeshComponentManager::OnRenderEvent(const xiiRenderWorldRenderEvent& e)
{
  if (e.m_Type != xiiRenderWorldRenderEvent::Type::BeginRender)
    return;

  XII_LOCK(m_Mutex);

  if (m_RequireUpdate.IsEmpty())
    return;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (auto pGraphicsOrTransferQueue = pDevice->GetDefaultCommandQueue(xiiGALCommandQueueType::Transfer))
  {
    auto pCommandList = pGraphicsOrTransferQueue->BeginCommandList();

    pCommandList->BeginDebugGroup("xiiInstanceData Update");
    {
      for (const auto& componentToUpdate : m_RequireUpdate)
      {
        xiiInstancedMeshComponent* pInstancedMeshComponent = nullptr;
        if (!TryGetComponent(componentToUpdate.m_hComponent, pInstancedMeshComponent))
          continue;

        if (pInstancedMeshComponent->m_pExplicitInstanceData)
        {
          xiiUInt32 uiOffset     = 0;
          auto      instanceData = pInstancedMeshComponent->m_pExplicitInstanceData->GetInstanceData(componentToUpdate.m_InstanceData.GetCount(), uiOffset);
          instanceData.CopyFrom(componentToUpdate.m_InstanceData);

          pInstancedMeshComponent->m_pExplicitInstanceData->UpdateInstanceData(pCommandList, instanceData.GetCount());
        }
      }
    }
    pCommandList->EndDebugGroup();
    pCommandList->Submit();
  }

  m_RequireUpdate.Clear();
}

void xiiInstancedMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  xiiRenderWorld::GetRenderEvent().AddEventHandler(xiiMakeDelegate(&xiiInstancedMeshComponentManager::OnRenderEvent, this));
}

void xiiInstancedMeshComponentManager::Deinitialize()
{
  xiiRenderWorld::GetRenderEvent().RemoveEventHandler(xiiMakeDelegate(&xiiInstancedMeshComponentManager::OnRenderEvent, this));

  SUPER::Deinitialize();
}

//////////////////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiInstancedMeshComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    XII_ACCESSOR_PROPERTY("MainColor", GetColor, SetColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),

    XII_ARRAY_ACCESSOR_PROPERTY("InstanceData", Instances_GetCount, Instances_GetValue, Instances_SetValue, Instances_Insert, Instances_Remove),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractGeometry, OnMsgExtractGeometry),
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiInstancedMeshComponent::xiiInstancedMeshComponent()  = default;
xiiInstancedMeshComponent::~xiiInstancedMeshComponent() = default;

void xiiInstancedMeshComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  inout_stream.GetStream().WriteArray(m_RawInstancedData).IgnoreResult();
}

void xiiInstancedMeshComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  inout_stream.GetStream().ReadArray(m_RawInstancedData).IgnoreResult();
}

void xiiInstancedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  XII_ASSERT_DEV(m_pExplicitInstanceData == nullptr, "Instance data must not be initialized at this point");
  m_pExplicitInstanceData = XII_DEFAULT_NEW(xiiInstanceData);
}

void xiiInstancedMeshComponent::OnDeactivated()
{
  XII_DEFAULT_DELETE(m_pExplicitInstanceData);
  m_pExplicitInstanceData = nullptr;

  SUPER::OnDeactivated();
}

void xiiInstancedMeshComponent::OnMsgExtractGeometry(xiiMsgExtractGeometry& ref_msg) {}

xiiResult xiiInstancedMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  xiiBoundingBoxSphere singleBounds;
  if (m_hMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    singleBounds = pMesh->GetBounds();

    for (const auto& instance : m_RawInstancedData)
    {
      auto instanceBounds = singleBounds;
      instanceBounds.Transform(instance.m_transform.GetAsMat4());

      ref_bounds.ExpandToInclude(instanceBounds);
    }

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiInstancedMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  SUPER::OnMsgExtractRenderData(msg);

  static_cast<const xiiInstancedMeshComponentManager*>(GetOwningManager())->EnqueueUpdate(this);
}

xiiMeshRenderData* xiiInstancedMeshComponent::CreateRenderData() const
{
  auto pRenderData = xiiCreateRenderDataForThisFrame<xiiInstancedMeshRenderData>(GetOwner());

  if (m_pExplicitInstanceData)
  {
    pRenderData->m_pExplicitInstanceData   = m_pExplicitInstanceData;
    pRenderData->m_uiExplicitInstanceCount = m_RawInstancedData.GetCount();
  }

  return pRenderData;
}

xiiUInt32 xiiInstancedMeshComponent::Instances_GetCount() const
{
  return m_RawInstancedData.GetCount();
}

xiiMeshInstanceData xiiInstancedMeshComponent::Instances_GetValue(xiiUInt32 uiIndex) const
{
  return m_RawInstancedData[uiIndex];
}

void xiiInstancedMeshComponent::Instances_SetValue(xiiUInt32 uiIndex, xiiMeshInstanceData value)
{
  m_RawInstancedData[uiIndex] = value;

  TriggerLocalBoundsUpdate();
}

void xiiInstancedMeshComponent::Instances_Insert(xiiUInt32 uiIndex, xiiMeshInstanceData value)
{
  m_RawInstancedData.InsertAt(uiIndex, value);

  TriggerLocalBoundsUpdate();
}

void xiiInstancedMeshComponent::Instances_Remove(xiiUInt32 uiIndex)
{
  m_RawInstancedData.RemoveAtAndCopy(uiIndex);

  TriggerLocalBoundsUpdate();
}

xiiArrayPtr<xiiPerInstanceData> xiiInstancedMeshComponent::GetInstanceData() const
{
  if (!m_pExplicitInstanceData || m_RawInstancedData.IsEmpty())
    return xiiArrayPtr<xiiPerInstanceData>();

  auto instanceData = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiPerInstanceData, m_RawInstancedData.GetCount());

  const xiiTransform ownerTransform = GetOwner()->GetGlobalTransform();

  float fBoundingSphereRadius = 1.0f;

  if (m_hMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    fBoundingSphereRadius = pMesh->GetBounds().GetSphere().m_fRadius;
  }

  for (xiiUInt32 i = 0; i < m_RawInstancedData.GetCount(); ++i)
  {
    const xiiTransform globalTransform = ownerTransform * m_RawInstancedData[i].m_transform;
    const xiiMat4      objectToWorld   = globalTransform.GetAsMat4();

    instanceData[i].ObjectToWorld = objectToWorld;

    if (m_RawInstancedData[i].m_transform.ContainsUniformScale())
    {
      instanceData[i].ObjectToWorldNormal = objectToWorld;
    }
    else
    {
      xiiMat3 mInverse = objectToWorld.GetRotationalPart();
      mInverse.Invert(0.0f).IgnoreResult();
      // we explicitly ignore the return value here (success / failure)
      // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

      xiiShaderTransform shaderT;
      shaderT                             = mInverse.GetTranspose();
      instanceData[i].ObjectToWorldNormal = shaderT;
    }

    instanceData[i].GameObjectID         = GetUniqueIdForRendering();
    instanceData[i].BoundingSphereRadius = fBoundingSphereRadius * m_RawInstancedData[i].m_transform.GetMaxScale();

    instanceData[i].Color = m_Color * m_RawInstancedData[i].m_color;
  }

  return instanceData;
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_InstancedMeshComponent);
