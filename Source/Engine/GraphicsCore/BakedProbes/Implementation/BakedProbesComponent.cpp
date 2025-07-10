#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Utilities/Progress.h>
#include <GraphicsCore/BakedProbes/BakedProbesComponent.h>
#include <GraphicsCore/BakedProbes/BakedProbesWorldModule.h>
#include <GraphicsCore/BakedProbes/ProbeTreeSectorResource.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Meshes/MeshComponentBase.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

struct xiiBakedProbesComponent::RenderDebugViewTask : public xiiTask
{
  RenderDebugViewTask()
  {
    ConfigureTask("BakingDebugView", xiiTaskNesting::Never);
  }

  virtual void Execute() override
  {
    XII_ASSERT_DEV(m_PixelData.GetCount() == m_uiWidth * m_uiHeight, "Pixel data must be pre-allocated");

    xiiProgress progress;
    progress.m_Events.AddEventHandler([this](const xiiProgressEvent& e) {
      if (e.m_Type != xiiProgressEvent::Type::CancelClicked)
      {
        if (HasBeenCanceled())
        {
          e.m_pProgressbar->UserClickedCancel();
        }
        m_bHasNewData = true;
      }
    });

    if (m_pBakingInterface->RenderDebugView(*m_pWorld, m_InverseViewProjection, m_uiWidth, m_uiHeight, m_PixelData, progress).Succeeded())
    {
      m_bHasNewData = true;
    }
  }

  xiiBakingInterface* m_pBakingInterface = nullptr;

  const xiiWorld*                  m_pWorld                = nullptr;
  xiiMat4                          m_InverseViewProjection = xiiMat4::MakeIdentity();
  xiiUInt32                        m_uiWidth               = 0;
  xiiUInt32                        m_uiHeight              = 0;
  xiiDynamicArray<xiiColorGammaUB> m_PixelData;

  bool m_bHasNewData = false;
};

//////////////////////////////////////////////////////////////////////////


xiiBakedProbesComponentManager::xiiBakedProbesComponentManager(xiiWorld* pWorld) :
  xiiSettingsComponentManager<xiiBakedProbesComponent>(pWorld)
{
}

xiiBakedProbesComponentManager::~xiiBakedProbesComponentManager() = default;

void xiiBakedProbesComponentManager::Initialize()
{
  {
    auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiBakedProbesComponentManager::RenderDebug, this);

    this->RegisterUpdateFunction(desc);
  }

  xiiRenderWorld::GetRenderEvent().AddEventHandler(xiiMakeDelegate(&xiiBakedProbesComponentManager::OnRenderEvent, this));

  CreateDebugResources();
}

void xiiBakedProbesComponentManager::Deinitialize()
{
  xiiRenderWorld::GetRenderEvent().RemoveEventHandler(xiiMakeDelegate(&xiiBakedProbesComponentManager::OnRenderEvent, this));
}

void xiiBakedProbesComponentManager::RenderDebug(const xiiWorldModule::UpdateContext& updateContext)
{
  if (xiiBakedProbesComponent* pComponent = GetSingletonComponent())
  {
    if (pComponent->GetShowDebugOverlay())
    {
      pComponent->RenderDebugOverlay();
    }
  }
}

void xiiBakedProbesComponentManager::OnRenderEvent(const xiiRenderWorldRenderEvent& e)
{
  if (e.m_Type != xiiRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (xiiBakedProbesComponent* pComponent = GetSingletonComponent())
  {
    auto& task = pComponent->m_pRenderDebugViewTask;
    if (task != nullptr && task->m_bHasNewData)
    {
      task->m_bHasNewData = false;

      xiiSharedPtr<xiiGALDevice> pGALDevice       = xiiGALDevice::GetDefaultDevice();
      xiiGALCommandQueue*        pGALCommandQueue = pGALDevice->GetDefaultCommandQueue();

      if (xiiSharedPtr<xiiGALCommandList> pGALCommandList = pGALCommandQueue->BeginCommandList())
      {
        {
          xiiGALScopedDebugGroup group(pGALCommandList, "BakingDebugView");

          xiiBoundingBoxU32 destBox;
          destBox.m_vMin.SetZero();
          destBox.m_vMax = xiiVec3U32(task->m_uiWidth, task->m_uiHeight, 1);

          xiiGALTextureSubResourceData sourceData;
          sourceData.m_pData    = task->m_PixelData.GetByteArrayPtr();
          sourceData.m_uiStride = task->m_uiWidth * sizeof(xiiColorGammaUB);

          pGALCommandList->UpdateTexture(pComponent->m_pDebugViewTexture, xiiGALTextureMipLevelData(), destBox, sourceData);
        }
        pGALCommandList->Submit();
      }
    }
  }
}

void xiiBakedProbesComponentManager::CreateDebugResources()
{
  if (!m_hDebugSphere.IsValid())
  {
    xiiGeometry geom;
    geom.AddStackedSphere(0.3f, 32, 16);

    const char*                 szBufferResourceName = "IrradianceProbeDebugSphereBuffer";
    xiiMeshBufferResourceHandle hMeshBuffer          = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szBufferResourceName);
    if (!hMeshBuffer.IsValid())
    {
      xiiMeshBufferResourceDescriptor desc;
      desc.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALResourceFormat::RGB32Float);
      desc.AddStream(xiiGALInputLayoutSemantic::Normal, xiiGALResourceFormat::RGB32Float);
      desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList);

      hMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
    }

    const char* szMeshResourceName = "IrradianceProbeDebugSphere";
    m_hDebugSphere                 = xiiResourceManager::GetExistingResource<xiiMeshResource>(szMeshResourceName);
    if (!m_hDebugSphere.IsValid())
    {
      xiiMeshResourceDescriptor desc;
      desc.UseExistingMeshBuffer(hMeshBuffer);
      desc.AddSubMesh(geom.CalculateTriangleCount(), 0, 0);
      desc.ComputeBounds();

      m_hDebugSphere = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
    }
  }

  if (!m_hDebugMaterial.IsValid())
  {
    m_hDebugMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>("{ 4d15c716-a8e9-43d4-9424-43174403fb94 }"); // IrradianceProbeVisualization.xiiMaterialAsset
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiBakedProbesComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Settings", m_Settings),
    XII_ACCESSOR_PROPERTY("ShowDebugOverlay", GetShowDebugOverlay, SetShowDebugOverlay)->AddAttributes(new xiiGroupAttribute("Debug")),
    XII_ACCESSOR_PROPERTY("ShowDebugProbes", GetShowDebugProbes, SetShowDebugProbes),
    XII_ACCESSOR_PROPERTY("UseTestPosition", GetUseTestPosition, SetUseTestPosition),
    XII_ACCESSOR_PROPERTY("TestPosition", GetTestPosition, SetTestPosition)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_FUNCTIONS
  {
    XII_FUNCTION_PROPERTY(OnObjectCreated),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Lighting/Baking"),
    new xiiLongOpAttribute("xiiLongOpProxy_BakeScene"),
    new xiiTransformManipulatorAttribute("TestPosition"),
    new xiiInDevelopmentAttribute(xiiInDevelopmentAttribute::Phase::Beta),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiBakedProbesComponent::xiiBakedProbesComponent()  = default;
xiiBakedProbesComponent::~xiiBakedProbesComponent() = default;

void xiiBakedProbesComponent::OnActivated()
{
  auto pModule = GetWorld()->GetOrCreateModule<xiiBakedProbesWorldModule>();
  pModule->SetProbeTreeResourcePrefix(m_sProbeTreeResourcePrefix);

  GetOwner()->UpdateLocalBounds();

  SUPER::OnActivated();
}

void xiiBakedProbesComponent::OnDeactivated()
{
  if (m_pRenderDebugViewTask != nullptr)
  {
    xiiTaskSystem::CancelTask(m_pRenderDebugViewTask).IgnoreResult();
  }

  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void xiiBakedProbesComponent::SetShowDebugOverlay(bool bShow)
{
  m_bShowDebugOverlay = bShow;

  if (bShow && m_pRenderDebugViewTask == nullptr)
  {
    m_pRenderDebugViewTask = XII_DEFAULT_NEW(RenderDebugViewTask);
  }
}

void xiiBakedProbesComponent::SetShowDebugProbes(bool bShow)
{
  if (m_bShowDebugProbes != bShow)
  {
    m_bShowDebugProbes = bShow;

    if (IsActiveAndInitialized())
    {
      xiiRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
    }
  }
}

void xiiBakedProbesComponent::SetUseTestPosition(bool bUse)
{
  if (m_bUseTestPosition != bUse)
  {
    m_bUseTestPosition = bUse;

    if (IsActiveAndInitialized())
    {
      xiiRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
    }
  }
}

void xiiBakedProbesComponent::SetTestPosition(const xiiVec3& vPos)
{
  m_vTestPosition = vPos;

  if (IsActiveAndInitialized())
  {
    xiiRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void xiiBakedProbesComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg)
{
  ref_msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? xiiDefaultSpatialDataCategories::RenderDynamic : xiiDefaultSpatialDataCategories::RenderStatic);
}

void xiiBakedProbesComponent::OnExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!m_bShowDebugProbes)
    return;

  // Don't trigger probe rendering in shadow or reflection views.
  if (ref_msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Shadow || ref_msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Reflection)
    return;

  auto pModule = GetWorld()->GetModule<xiiBakedProbesWorldModule>();
  if (!pModule->HasProbeData())
    return;

  const xiiGameObject* pOwner   = GetOwner();
  auto                 pManager = static_cast<const xiiBakedProbesComponentManager*>(GetOwningManager());

  auto addProbeRenderData = [&](const xiiVec3& vPosition, xiiCompressedSkyVisibility skyVisibility, xiiRenderData::Caching::Enum caching) {
    xiiTransform transform = xiiTransform::MakeIdentity();
    transform.m_vPosition  = vPosition;

    xiiColor encodedSkyVisibility = xiiColor::Black;
    encodedSkyVisibility.r        = *reinterpret_cast<const float*>(&skyVisibility);

    xiiMeshRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiMeshRenderData>(pOwner);
    {
      pRenderData->m_GlobalTransform = transform;
      pRenderData->m_hMesh           = pManager->m_hDebugSphere;
      pRenderData->m_hMaterial       = pManager->m_hDebugMaterial;
      pRenderData->m_Color           = encodedSkyVisibility;
      pRenderData->m_uiSubMeshIndex  = 0;
      pRenderData->m_uiUniqueID      = xiiRenderComponent::GetUniqueIdForRendering(*this, 0);
      pRenderData->m_GlobalBounds    = xiiBoundingBoxSphere::MakeInvalid();

      pRenderData->FillSortingKey();
    }

    ref_msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::SimpleOpaque, caching);
  };

  if (m_bUseTestPosition)
  {
    xiiBakedProbesWorldModule::ProbeIndexData indexData;
    if (pModule->GetProbeIndexData(m_vTestPosition, xiiVec3::MakeAxisZ(), indexData).Failed())
      return;

    if (true)
    {
      xiiResourceLock<xiiProbeTreeSectorResource> pProbeTree(pModule->m_hProbeTree, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pProbeTree.GetAcquireResult() != xiiResourceAcquireResult::Final)
        return;

      for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(indexData.m_probeIndices); ++i)
      {
        xiiVec3 pos = pProbeTree->GetProbePositions()[indexData.m_probeIndices[i]];
        xiiDebugRenderer::DrawCross(ref_msg.m_pView->GetHandle(), pos, 0.5f, xiiColor::Yellow);

        pos.z += 0.5f;
        xiiDebugRenderer::Draw3DText(ref_msg.m_pView->GetHandle(), xiiFmt("Weight: {}", indexData.m_probeWeights[i]), pos, xiiColor::Yellow);
      }
    }

    xiiCompressedSkyVisibility skyVisibility = xiiBakingUtils::CompressSkyVisibility(pModule->GetSkyVisibility(indexData));

    addProbeRenderData(m_vTestPosition, skyVisibility, xiiRenderData::Caching::Never);
  }
  else
  {
    xiiResourceLock<xiiProbeTreeSectorResource> pProbeTree(pModule->m_hProbeTree, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pProbeTree.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return;

    auto probePositions = pProbeTree->GetProbePositions();
    auto skyVisibility  = pProbeTree->GetSkyVisibility();

    for (xiiUInt32 uiProbeIndex = 0; uiProbeIndex < probePositions.GetCount(); ++uiProbeIndex)
    {
      addProbeRenderData(probePositions[uiProbeIndex], skyVisibility[uiProbeIndex], xiiRenderData::Caching::IfStatic);
    }
  }
}

void xiiBakedProbesComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  if (m_Settings.Serialize(s).Failed())
    return;

  s << m_sProbeTreeResourcePrefix;
  s << m_bShowDebugOverlay;
  s << m_bShowDebugProbes;
  s << m_bUseTestPosition;
  s << m_vTestPosition;
}

void xiiBakedProbesComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = inout_stream.GetStream();

  if (m_Settings.Deserialize(s).Failed())
    return;

  s >> m_sProbeTreeResourcePrefix;
  s >> m_bShowDebugOverlay;
  s >> m_bShowDebugProbes;
  s >> m_bUseTestPosition;
  s >> m_vTestPosition;
}

void xiiBakedProbesComponent::RenderDebugOverlay()
{
  xiiView* pView = xiiRenderWorld::GetViewByUsageHint(xiiCameraUsageHint::MainView, xiiCameraUsageHint::EditorView);
  if (pView == nullptr)
    return;

  xiiBakingInterface* pBakingInterface = xiiSingletonRegistry::GetSingletonInstance<xiiBakingInterface>();
  if (pBakingInterface == nullptr)
  {
    xiiDebugRenderer::Draw2DText(pView->GetHandle(), "Baking Plugin not loaded", xiiVec2I32(10, 10), xiiColor::OrangeRed);
    return;
  }

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiRectFloat viewport = pView->GetViewport();
  xiiUInt32    uiWidth  = static_cast<xiiUInt32>(xiiMath::Ceil(viewport.width / 3.0f));
  xiiUInt32    uiHeight = static_cast<xiiUInt32>(xiiMath::Ceil(viewport.height / 3.0f));

  xiiMat4 inverseViewProjection = pView->GetInverseViewProjectionMatrix(xiiCameraEye::Left);

  if (m_pRenderDebugViewTask->m_InverseViewProjection != inverseViewProjection ||
      m_pRenderDebugViewTask->m_uiWidth != uiWidth || m_pRenderDebugViewTask->m_uiHeight != uiHeight)
  {
    xiiTaskSystem::CancelTask(m_pRenderDebugViewTask).IgnoreResult();

    m_pRenderDebugViewTask->m_pBakingInterface      = pBakingInterface;
    m_pRenderDebugViewTask->m_pWorld                = GetWorld();
    m_pRenderDebugViewTask->m_InverseViewProjection = inverseViewProjection;
    m_pRenderDebugViewTask->m_uiWidth               = uiWidth;
    m_pRenderDebugViewTask->m_uiHeight              = uiHeight;
    m_pRenderDebugViewTask->m_PixelData.SetCount(uiWidth * uiHeight, xiiColor::Red);
    m_pRenderDebugViewTask->m_bHasNewData = false;

    xiiTaskSystem::StartSingleTask(m_pRenderDebugViewTask, xiiTaskPriority::LongRunning);
  }

  xiiUInt32 uiTextureWidth  = 0;
  xiiUInt32 uiTextureHeight = 0;
  if (m_pDebugViewTexture)
  {
    uiTextureWidth  = m_pDebugViewTexture->GetDescription().m_Size.width;
    uiTextureHeight = m_pDebugViewTexture->GetDescription().m_Size.height;
  }

  if (uiTextureWidth != uiWidth || uiTextureHeight != uiHeight)
  {
    m_pDebugViewTexture.Clear();

    xiiGALTextureCreationDescription textureDescription;
    textureDescription.m_Type        = xiiGALResourceDimension::Texture2D;
    textureDescription.m_Size.width  = uiWidth;
    textureDescription.m_Size.height = uiHeight;
    textureDescription.m_Format      = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    textureDescription.m_BindFlags   = xiiGALBindFlags::ShaderResource;
    textureDescription.m_Usage       = xiiGALResourceUsage::Mutable;

    m_pDebugViewTexture = pDevice->CreateTexture(textureDescription);
  }

  xiiRectFloat rectInPixel = xiiRectFloat(10.0f, 10.0f, static_cast<float>(uiWidth), static_cast<float>(uiHeight));

  xiiDebugRenderer::Draw2DRectangle(pView->GetHandle(), rectInPixel, 0.0f, xiiColor::White, m_pDebugViewTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource));
}

void xiiBakedProbesComponent::OnObjectCreated(const xiiAbstractObjectNode& node)
{
  xiiStringBuilder sPrefix;
  sPrefix.SetFormat(":project/AssetCache/Generated/{0}", node.GetGuid());

  m_sProbeTreeResourcePrefix.Assign(sPrefix);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_BakedProbes_Implementation_BakedProbesComponent);
