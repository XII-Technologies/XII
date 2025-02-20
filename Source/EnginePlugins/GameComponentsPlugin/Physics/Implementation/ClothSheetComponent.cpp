#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Physics/ClothSheetComponent.h>
#include <GraphicsCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/DynamicMeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Pipeline/InstanceDataProvider.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Device/Device.h>

/* TODO:
 * cache render category
 */

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClothSheetRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClothSheetRenderer, 1, xiiRTTIDefaultAllocator<xiiClothSheetRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiClothSheetFlags, 1)
  XII_ENUM_CONSTANT(xiiClothSheetFlags::FixedCornerTopLeft),
  XII_ENUM_CONSTANT(xiiClothSheetFlags::FixedCornerTopRight),
  XII_ENUM_CONSTANT(xiiClothSheetFlags::FixedCornerBottomRight),
  XII_ENUM_CONSTANT(xiiClothSheetFlags::FixedCornerBottomLeft),
  XII_ENUM_CONSTANT(xiiClothSheetFlags::FixedEdgeTop),
  XII_ENUM_CONSTANT(xiiClothSheetFlags::FixedEdgeRight),
  XII_ENUM_CONSTANT(xiiClothSheetFlags::FixedEdgeBottom),
  XII_ENUM_CONSTANT(xiiClothSheetFlags::FixedEdgeLeft),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_COMPONENT_TYPE(xiiClothSheetComponent, 1, xiiComponentMode::Static)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(0.5f, 0.5f))),
      XII_ACCESSOR_PROPERTY("Slack", GetSlack, SetSlack)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(0.0f, 0.0f))),
      XII_ACCESSOR_PROPERTY("Segments", GetSegments, SetSegments)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2U32(7, 7)), new xiiClampValueAttribute(xiiVec2U32(1, 1), xiiVec2U32(31, 31))),
      XII_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
      XII_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 10.0f)),
      XII_BITFLAGS_ACCESSOR_PROPERTY("Flags", xiiClothSheetFlags, GetFlags, SetFlags),
      XII_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
      XII_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::White)),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiCategoryAttribute("Effects"),
    }
    XII_END_ATTRIBUTES;
    XII_BEGIN_MESSAGEHANDLERS
    {
      XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    }
    XII_END_MESSAGEHANDLERS;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiClothSheetComponent::xiiClothSheetComponent()  = default;
xiiClothSheetComponent::~xiiClothSheetComponent() = default;

void xiiClothSheetComponent::SetSize(xiiVec2 vVal)
{
  m_vSize = vVal;
  SetupCloth();
}

void xiiClothSheetComponent::SetSlack(xiiVec2 vVal)
{
  m_vSlack = vVal;
  SetupCloth();
}

void xiiClothSheetComponent::SetSegments(xiiVec2U32 vVal)
{
  m_vSegments = vVal;
  SetupCloth();
}

void xiiClothSheetComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vSize;
  s << m_vSegments;
  s << m_vSlack;
  s << m_fWindInfluence;
  s << m_fDamping;
  s << m_Flags;
  s << m_hMaterial;
  s << m_Color;
}

void xiiClothSheetComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_vSize;
  s >> m_vSegments;
  s >> m_vSlack;
  s >> m_fWindInfluence;
  s >> m_fDamping;
  s >> m_Flags;
  s >> m_hMaterial;
  s >> m_Color;
}

void xiiClothSheetComponent::OnActivated()
{
  SUPER::OnActivated();

  SetupCloth();
}

void xiiClothSheetComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  SetupCloth();
}

void xiiClothSheetComponent::SetupCloth()
{
  m_Bbox = xiiBoundingBox::MakeInvalid();

  if (IsActiveAndSimulating())
  {
    m_uiSleepCounter   = 0;
    m_uiVisibleCounter = 5;

    m_Simulator.m_uiWidth  = static_cast<xiiUInt8>(m_vSegments.x + 1);
    m_Simulator.m_uiHeight = static_cast<xiiUInt8>(m_vSegments.y + 1);
    m_Simulator.m_vAcceleration.Set(0, 0, -10);
    m_Simulator.m_vSegmentLength = m_vSize.CompMul(xiiVec2(1.0f) + m_vSlack);
    m_Simulator.m_vSegmentLength.x /= (float)m_vSegments.x;
    m_Simulator.m_vSegmentLength.y /= (float)m_vSegments.y;
    m_Simulator.m_Nodes.Clear();
    m_Simulator.m_Nodes.SetCount(m_Simulator.m_uiWidth * m_Simulator.m_uiHeight);

    const xiiVec3 pos  = xiiVec3(0);
    const xiiVec3 dirX = xiiVec3(1, 0, 0);
    const xiiVec3 dirY = xiiVec3(0, 1, 0);

    xiiVec2 dist = m_vSize;
    dist.x /= (float)m_vSegments.x;
    dist.y /= (float)m_vSegments.y;

    for (xiiUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
    {
      for (xiiUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const xiiUInt32 idx = (y * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_vPosition         = xiiSimdConversion::ToVec3(pos + x * dist.x * dirX + y * dist.y * dirY);
        m_Simulator.m_Nodes[idx].m_vPreviousPosition = m_Simulator.m_Nodes[idx].m_vPosition;
      }
    }

    if (m_Flags.IsSet(xiiClothSheetFlags::FixedCornerTopLeft))
      m_Simulator.m_Nodes[0].m_bFixed = true;

    if (m_Flags.IsSet(xiiClothSheetFlags::FixedCornerTopRight))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth - 1].m_bFixed = true;

    if (m_Flags.IsSet(xiiClothSheetFlags::FixedCornerBottomRight))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth * m_Simulator.m_uiHeight - 1].m_bFixed = true;

    if (m_Flags.IsSet(xiiClothSheetFlags::FixedCornerBottomLeft))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth * (m_Simulator.m_uiHeight - 1)].m_bFixed = true;

    if (m_Flags.IsSet(xiiClothSheetFlags::FixedEdgeTop))
    {
      for (xiiUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const xiiUInt32 idx = (0 * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(xiiClothSheetFlags::FixedEdgeRight))
    {
      for (xiiUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
      {
        const xiiUInt32 idx = (y * m_Simulator.m_uiWidth) + (m_Simulator.m_uiWidth - 1);

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(xiiClothSheetFlags::FixedEdgeBottom))
    {
      for (xiiUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const xiiUInt32 idx = ((m_Simulator.m_uiHeight - 1) * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(xiiClothSheetFlags::FixedEdgeLeft))
    {
      for (xiiUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
      {
        const xiiUInt32 idx = (y * m_Simulator.m_uiWidth) + 0;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }
  }

  TriggerLocalBoundsUpdate();
}

void xiiClothSheetComponent::OnDeactivated()
{
  m_Simulator.m_Nodes.Clear();

  SUPER::OnDeactivated();
}

xiiResult xiiClothSheetComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  if (m_Bbox.IsValid())
  {
    ref_bounds.ExpandToInclude(xiiBoundingBoxSphere::MakeFromBox(m_Bbox));
  }
  else
  {
    xiiBoundingBox box = xiiBoundingBox::MakeInvalid();
    box.ExpandToInclude(xiiVec3::MakeZero());
    box.ExpandToInclude(xiiVec3(m_vSize.x, 0, -0.1f));
    box.ExpandToInclude(xiiVec3(0, m_vSize.y, +0.1f));
    box.ExpandToInclude(xiiVec3(m_vSize.x, m_vSize.y, 0));

    ref_bounds.ExpandToInclude(xiiBoundingBoxSphere::MakeFromBox(box));
  }

  return XII_SUCCESS;
}

void xiiClothSheetComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  auto pRenderData               = xiiCreateRenderDataForThisFrame<xiiClothSheetRenderData>(GetOwner());
  pRenderData->m_uiUniqueID      = GetUniqueIdForRendering();
  pRenderData->m_Color           = m_Color;
  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_uiSortingKey    = xiiHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash());
  pRenderData->m_GlobalBounds    = GetOwner()->GetGlobalBounds();
  pRenderData->m_hMaterial       = m_hMaterial;


  if (m_Simulator.m_Nodes.IsEmpty())
  {
    pRenderData->m_uiVerticesX = 2;
    pRenderData->m_uiVerticesY = 2;

    pRenderData->m_Positions = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiVec3, 4);
    pRenderData->m_Indices   = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiUInt16, 6);

    pRenderData->m_Positions[0] = xiiVec3(0, 0, 0);
    pRenderData->m_Positions[1] = xiiVec3(m_vSize.x, 0, 0);
    pRenderData->m_Positions[2] = xiiVec3(0, m_vSize.y, 0);
    pRenderData->m_Positions[3] = xiiVec3(m_vSize.x, m_vSize.y, 0);

    pRenderData->m_Indices[0] = 0;
    pRenderData->m_Indices[1] = 1;
    pRenderData->m_Indices[2] = 2;

    pRenderData->m_Indices[3] = 1;
    pRenderData->m_Indices[4] = 3;
    pRenderData->m_Indices[5] = 2;
  }
  else
  {
    m_uiVisibleCounter = 3;

    pRenderData->m_uiVerticesX = m_Simulator.m_uiWidth;
    pRenderData->m_uiVerticesY = m_Simulator.m_uiHeight;

    pRenderData->m_Positions = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiVec3, pRenderData->m_uiVerticesX * pRenderData->m_uiVerticesY);
    pRenderData->m_Indices   = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiUInt16, (pRenderData->m_uiVerticesX - 1) * (pRenderData->m_uiVerticesY - 1) * 2 * 3);

    {
      xiiUInt32 vidx = 0;
      for (xiiUInt32 y = 0; y < pRenderData->m_uiVerticesY; ++y)
      {
        for (xiiUInt32 x = 0; x < pRenderData->m_uiVerticesX; ++x, ++vidx)
        {
          pRenderData->m_Positions[vidx] = xiiSimdConversion::ToVec3(m_Simulator.m_Nodes[vidx].m_vPosition);
        }
      }
    }

    {
      xiiUInt32 tidx = 0;
      xiiUInt16 vidx = 0;
      for (xiiUInt16 y = 0; y < pRenderData->m_uiVerticesY - 1; ++y)
      {
        for (xiiUInt16 x = 0; x < pRenderData->m_uiVerticesX - 1; ++x, ++vidx)
        {
          pRenderData->m_Indices[tidx++] = vidx;
          pRenderData->m_Indices[tidx++] = vidx + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX;

          pRenderData->m_Indices[tidx++] = vidx + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX;
        }

        ++vidx;
      }
    }
  }

  xiiRenderData::Category category = xiiDefaultRenderDataCategories::LitOpaque;

  if (m_hMaterial.IsValid())
  {
    xiiResourceLock<xiiMaterialResource> pMaterial(m_hMaterial, xiiResourceAcquireMode::AllowLoadingFallback);
    category = pMaterial->GetRenderDataCategory();
  }

  msg.AddRenderData(pRenderData, category, xiiRenderData::Caching::Never);
}

void xiiClothSheetComponent::SetFlags(xiiBitflags<xiiClothSheetFlags> flags)
{
  m_Flags = flags;
  SetupCloth();
}

void xiiClothSheetComponent::Update()
{
  if (m_Simulator.m_Nodes.IsEmpty() || m_uiVisibleCounter == 0)
    return;

  --m_uiVisibleCounter;

  {
    xiiVec3 acc = -GetOwner()->GetLinearVelocity();

    if (const xiiPhysicsWorldModuleInterface* pModule = GetWorld()->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
    {
      acc += pModule->GetGravity();
    }
    else
    {
      acc += xiiVec3(0, 0, -9.81f);
    }

    if (m_fWindInfluence > 0.0f)
    {
      if (const xiiWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<xiiWindWorldModuleInterface>())
      {
        xiiVec3 ropeDir(0, 0, 1);

        // take the position of the center cloth node to sample the wind
        const xiiVec3 vSampleWindPos = GetOwner()->GetGlobalTransform().TransformPosition(xiiSimdConversion::ToVec3(m_Simulator.m_Nodes[m_Simulator.m_uiWidth * (m_Simulator.m_uiHeight / 2) + m_Simulator.m_uiWidth / 2].m_vPosition));

        const xiiVec3 vWind = pWind->GetWindAt(vSampleWindPos) * m_fWindInfluence;

        acc += vWind;
        acc += pWind->ComputeWindFlutter(vWind, ropeDir, 0.5f, GetOwner()->GetStableRandomSeed());
      }
    }

    // rotate the acceleration vector into the local simulation space
    acc = GetOwner()->GetGlobalRotation().GetInverse() * acc;

    if (m_Simulator.m_vAcceleration != acc)
    {
      m_Simulator.m_vAcceleration = acc;
      m_uiSleepCounter            = 0;
    }
  }

  if (m_uiSleepCounter <= 10)
  {
    m_Simulator.m_fDampingFactor = xiiMath::Lerp(1.0f, 0.97f, m_fDamping);

    m_Simulator.SimulateCloth(GetWorld()->GetClock().GetTimeDiff());

    auto prevBbox = m_Bbox;
    m_Bbox.ExpandToInclude(xiiSimdConversion::ToVec3(m_Simulator.m_Nodes[0].m_vPosition));
    m_Bbox.ExpandToInclude(xiiSimdConversion::ToVec3(m_Simulator.m_Nodes[m_Simulator.m_uiWidth - 1].m_vPosition));
    m_Bbox.ExpandToInclude(xiiSimdConversion::ToVec3(m_Simulator.m_Nodes[((m_Simulator.m_uiHeight - 1) * m_Simulator.m_uiWidth)].m_vPosition));
    m_Bbox.ExpandToInclude(xiiSimdConversion::ToVec3(m_Simulator.m_Nodes.PeekBack().m_vPosition));

    if (prevBbox != m_Bbox)
    {
      SetUserFlag(0, true); // flag 0 => requires local bounds update

      // can't call this here in the async phase
      // TriggerLocalBoundsUpdate();
    }

    ++m_uiCheckEquilibriumCounter;
    if (m_uiCheckEquilibriumCounter > 64)
    {
      m_uiCheckEquilibriumCounter = 0;

      if (m_Simulator.HasEquilibrium(0.01f))
      {
        ++m_uiSleepCounter;
      }
      else
      {
        m_uiSleepCounter = 0;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////

xiiClothSheetRenderer::xiiClothSheetRenderer()
{
  CreateVertexBuffer();
}

xiiClothSheetRenderer::~xiiClothSheetRenderer() = default;

void xiiClothSheetRenderer::GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::Selection);
}

void xiiClothSheetRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(xiiGetStaticRTTI<xiiClothSheetRenderData>());
}

void xiiClothSheetRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  const bool bNeedsNormals = (renderViewContext.m_pViewData->m_CameraUsageHint != xiiCameraUsageHint::Shadow);


  xiiRenderContext*  pRenderContext  = renderViewContext.m_pRenderContext;
  xiiGALCommandList* pGALCommandList = pRenderContext->GetCommandList();

  xiiInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<xiiInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  xiiResourceLock<xiiDynamicMeshBufferResource> pBuffer(m_hDynamicMeshBuffer, xiiResourceAcquireMode::BlockTillLoaded);

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  for (auto it = batch.GetIterator<xiiClothSheetRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const xiiClothSheetRenderData* pRenderData = it;

    XII_ASSERT_DEV(pRenderData->m_uiVerticesX > 1 && pRenderData->m_uiVerticesY > 1, "Invalid cloth render data");

    pRenderContext->BindMaterial(pRenderData->m_hMaterial);

    xiiUInt32                       uiInstanceDataOffset = 0;
    xiiArrayPtr<xiiPerInstanceData> instanceData         = pInstanceData->GetInstanceData(1, uiInstanceDataOffset);

    instanceData[0].ObjectToWorld       = pRenderData->m_GlobalTransform;
    instanceData[0].ObjectToWorldNormal = instanceData[0].ObjectToWorld;
    instanceData[0].GameObjectID        = pRenderData->m_uiUniqueID;
    instanceData[0].Color               = pRenderData->m_Color;

    if (auto pGraphicsOrTransferQueue = pDevice->GetDefaultCommandQueue(xiiGALCommandQueueType::Transfer))
    {
      auto pCommandList = pGraphicsOrTransferQueue->BeginCommandList();

      pCommandList->BeginDebugGroup("xiiInstanceData Update");
      {
        pInstanceData->UpdateInstanceData(pCommandList, 1);
      }
      pCommandList->EndDebugGroup();
      pCommandList->Submit();
    }

    {
      auto pVertexData = pBuffer->AccessVertexData();
      auto pIndexData  = pBuffer->AccessIndex16Data();

      const float fDivU = 1.0f / (pRenderData->m_uiVerticesX - 1);
      const float fDivY = 1.0f / (pRenderData->m_uiVerticesY - 1);

      const xiiUInt16 width = pRenderData->m_uiVerticesX;

      if (bNeedsNormals)
      {
        const xiiUInt16 widthM1  = width - 1;
        const xiiUInt16 heightM1 = pRenderData->m_uiVerticesY - 1;

        xiiUInt16 topIdx = 0;

        xiiUInt32 vidx = 0;
        for (xiiUInt16 y = 0; y < pRenderData->m_uiVerticesY; ++y)
        {
          xiiUInt16       leftIdx   = 0;
          const xiiUInt16 bottomIdx = xiiMath::Min<xiiUInt16>(y + 1, heightM1);

          const xiiUInt32 yOff       = y * width;
          const xiiUInt32 yOffTop    = topIdx * width;
          const xiiUInt32 yOffBottom = bottomIdx * width;

          for (xiiUInt16 x = 0; x < width; ++x, ++vidx)
          {
            const xiiUInt16 rightIdx = xiiMath::Min<xiiUInt16>(x + 1, widthM1);

            const xiiVec3 leftPos   = pRenderData->m_Positions[yOff + leftIdx];
            const xiiVec3 rightPos  = pRenderData->m_Positions[yOff + rightIdx];
            const xiiVec3 topPos    = pRenderData->m_Positions[yOffTop + x];
            const xiiVec3 bottomPos = pRenderData->m_Positions[yOffBottom + x];

            const xiiVec3 leftToRight = rightPos - leftPos;
            const xiiVec3 bottomToTop = topPos - bottomPos;
            xiiVec3       normal      = -leftToRight.CrossRH(bottomToTop);
            normal.NormalizeIfNotZero(xiiVec3(0, 0, 1)).IgnoreResult();

            xiiVec3 tangent = leftToRight;
            tangent.NormalizeIfNotZero(xiiVec3(1, 0, 0)).IgnoreResult();

            pVertexData[vidx].m_vPosition = pRenderData->m_Positions[vidx];
            pVertexData[vidx].m_vTexCoord = xiiVec2(x * fDivU, y * fDivY);
            pVertexData[vidx].EncodeNormal(normal);
            pVertexData[vidx].EncodeTangent(tangent, 1.0f);

            leftIdx = x;
          }

          topIdx = y;
        }
      }
      else
      {
        xiiUInt32 vidx = 0;
        for (xiiUInt16 y = 0; y < pRenderData->m_uiVerticesY; ++y)
        {
          for (xiiUInt16 x = 0; x < width; ++x, ++vidx)
          {
            pVertexData[vidx].m_vPosition = pRenderData->m_Positions[vidx];
            pVertexData[vidx].m_vTexCoord = xiiVec2(x * fDivU, y * fDivY);
            pVertexData[vidx].EncodeNormal(xiiVec3::MakeAxisZ());
            pVertexData[vidx].EncodeTangent(xiiVec3::MakeAxisX(), 1.0f);
          }
        }
      }

      xiiMemoryUtils::Copy<xiiUInt16>(pIndexData.GetPtr(), pRenderData->m_Indices.GetPtr(), pRenderData->m_Indices.GetCount());
    }

    const xiiUInt32 uiNumPrimitives = (pRenderData->m_uiVerticesX - 1) * (pRenderData->m_uiVerticesY - 1) * 2;

    pBuffer->UpdateGpuBuffer(pGALCommandList, 0, pRenderData->m_uiVerticesX * pRenderData->m_uiVerticesY);

    // redo this after the primitive count has changed
    pRenderContext->BindMeshBuffer(m_hDynamicMeshBuffer);

    renderViewContext.m_pRenderContext->DrawMeshBuffer(uiNumPrimitives).IgnoreResult();
  }
}

void xiiClothSheetRenderer::CreateVertexBuffer()
{
  if (m_hDynamicMeshBuffer.IsValid())
    return;

  m_hDynamicMeshBuffer = xiiResourceManager::GetExistingResource<xiiDynamicMeshBufferResource>("ClothSheet");

  if (!m_hDynamicMeshBuffer.IsValid())
  {
    const xiiUInt32 uiMaxVerts = 32;

    xiiDynamicMeshBufferResourceDescriptor desc;
    desc.m_uiMaxVertices   = uiMaxVerts * uiMaxVerts;
    desc.m_IndexType       = xiiGALValueType::UInt16;
    desc.m_uiMaxPrimitives = xiiMath::Square(uiMaxVerts - 1) * 2;

    m_hDynamicMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiDynamicMeshBufferResource>("ClothSheet", std::move(desc), "Cloth Sheet Buffer");
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiClothSheetComponentManager::xiiClothSheetComponentManager(xiiWorld* pWorld) :
  xiiComponentManager(pWorld)
{
}

xiiClothSheetComponentManager::~xiiClothSheetComponentManager() = default;

void xiiClothSheetComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiClothSheetComponentManager::Update, this);
    desc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiClothSheetComponentManager::UpdateBounds, this);
    desc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void xiiClothSheetComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update();
    }
  }
}

void xiiClothSheetComponentManager::UpdateBounds(const xiiWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUserFlag(0))
    {
      it->TriggerLocalBoundsUpdate();

      // reset update bounds flag
      it->SetUserFlag(0, false);
    }
  }
}
