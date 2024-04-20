#include <GraphicsCore/GraphicsCorePCH.h>

#include <../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Meshes/CustomMeshComponent.h>
#include <GraphicsCore/Meshes/DynamicMeshBufferResource.h>
#include <GraphicsCore/Pipeline/InstanceDataProvider.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiCustomMeshComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    XII_MESSAGE_HANDLER(xiiMsgSetColor, OnMsgSetColor),
  } XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiAtomicInteger32 s_iCustomMeshResources;

xiiCustomMeshComponent::xiiCustomMeshComponent()
{
  m_Bounds.SetInvalid();
}

xiiCustomMeshComponent::~xiiCustomMeshComponent() = default;

void xiiCustomMeshComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_Color;
  s << m_hMaterial;
}

void xiiCustomMeshComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_hMaterial;

  if (uiVersion < 2)
  {
    xiiUInt32 uiCategory = 0;
    s >> uiCategory;
  }
}

xiiResult xiiCustomMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  if (m_Bounds.IsValid())
  {
    ref_bounds = m_Bounds;
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiDynamicMeshBufferResourceHandle xiiCustomMeshComponent::CreateMeshResource(xiiGALPrimitiveTopology::Enum topology, xiiUInt32 uiMaxVertices, xiiUInt32 uiMaxPrimitives, xiiGALValueType::Enum indexType)
{
  xiiDynamicMeshBufferResourceDescriptor desc;
  desc.m_Topology        = topology;
  desc.m_uiMaxVertices   = uiMaxVertices;
  desc.m_uiMaxPrimitives = uiMaxPrimitives;
  desc.m_IndexType       = indexType;
  desc.m_bColorStream    = true;

  xiiStringBuilder sGuid;
  sGuid.SetFormat("CustomMesh_{}", s_iCustomMeshResources.Increment());

  m_hDynamicMesh = xiiResourceManager::CreateResource<xiiDynamicMeshBufferResource>(sGuid, std::move(desc));

  InvalidateCachedRenderData();

  return m_hDynamicMesh;
}

void xiiCustomMeshComponent::SetMeshResource(const xiiDynamicMeshBufferResourceHandle& hMesh)
{
  m_hDynamicMesh = hMesh;
  InvalidateCachedRenderData();
}

void xiiCustomMeshComponent::SetBounds(const xiiBoundingBoxSphere& bounds)
{
  m_Bounds = bounds;
  TriggerLocalBoundsUpdate();
}

void xiiCustomMeshComponent::SetMaterial(const xiiMaterialResourceHandle& hMaterial)
{
  m_hMaterial = hMaterial;
  InvalidateCachedRenderData();
}

xiiMaterialResourceHandle xiiCustomMeshComponent::GetMaterial() const
{
  return m_hMaterial;
}

void xiiCustomMeshComponent::SetMaterialFile(const char* szMaterial)
{
  xiiMaterialResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szMaterial))
  {
    hResource = xiiResourceManager::LoadResource<xiiMaterialResource>(szMaterial);
  }

  m_hMaterial = hResource;
}

const char* xiiCustomMeshComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void xiiCustomMeshComponent::SetColor(const xiiColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const xiiColor& xiiCustomMeshComponent::GetColor() const
{
  return m_Color;
}

void xiiCustomMeshComponent::OnMsgSetMeshMaterial(xiiMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_hMaterial);
}

void xiiCustomMeshComponent::OnMsgSetColor(xiiMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

void xiiCustomMeshComponent::SetUsePrimitiveRange(xiiUInt32 uiFirstPrimitive /*= 0*/, xiiUInt32 uiNumPrimitives /*= xiiMath::MaxValue<xiiUInt32>()*/)
{
  m_uiFirstPrimitive = uiFirstPrimitive;
  m_uiNumPrimitives  = uiNumPrimitives;
}

void xiiCustomMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (!m_hDynamicMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  xiiResourceLock<xiiDynamicMeshBufferResource> pMesh(m_hDynamicMesh, xiiResourceAcquireMode::BlockTillLoaded);

  xiiCustomMeshRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiCustomMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform  = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds     = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh            = m_hDynamicMesh;
    pRenderData->m_hMaterial        = m_hMaterial;
    pRenderData->m_Color            = m_Color;
    pRenderData->m_uiUniqueID       = GetUniqueIdForRendering();
    pRenderData->m_uiFirstPrimitive = xiiMath::Min(m_uiFirstPrimitive, pMesh->GetDescriptor().m_uiMaxPrimitives);
    pRenderData->m_uiNumPrimitives  = xiiMath::Min(m_uiNumPrimitives, pMesh->GetDescriptor().m_uiMaxPrimitives - pRenderData->m_uiFirstPrimitive);

    pRenderData->FillBatchIdAndSortingKey();
  }

  xiiResourceLock<xiiMaterialResource> pMaterial(m_hMaterial, xiiResourceAcquireMode::AllowLoadingFallback);
  xiiRenderData::Category              category      = pMaterial->GetRenderDataCategory();
  bool                                 bDontCacheYet = pMaterial.GetAcquireResult() == xiiResourceAcquireResult::LoadingFallback;

  msg.AddRenderData(pRenderData, category, bDontCacheYet ? xiiRenderData::Caching::Never : xiiRenderData::Caching::IfStatic);
}

void xiiCustomMeshComponent::OnActivated()
{
  if (false)
  {
    xiiGeometry geo;
    geo.AddTorus(1.0f, 1.5f, 32, 16, false);
    geo.TriangulatePolygons();
    geo.ComputeTangents();

    auto hMesh = CreateMeshResource(xiiGALPrimitiveTopology::TriangleList, geo.GetVertices().GetCount(), geo.GetPolygons().GetCount(), xiiGALValueType::UInt32);

    xiiResourceLock<xiiDynamicMeshBufferResource> pMesh(hMesh, xiiResourceAcquireMode::BlockTillLoaded);

    auto verts = pMesh->AccessVertexData();
    auto cols  = pMesh->AccessColorData();

    for (xiiUInt32 v = 0; v < verts.GetCount(); ++v)
    {
      verts[v].m_vPosition = geo.GetVertices()[v].m_vPosition;
      verts[v].m_vTexCoord.SetZero();
      verts[v].EncodeNormal(geo.GetVertices()[v].m_vNormal);
      verts[v].EncodeTangent(geo.GetVertices()[v].m_vTangent, 1.0f);

      cols[v] = xiiColor::CornflowerBlue;
    }

    auto ind = pMesh->AccessIndex32Data();

    for (xiiUInt32 i = 0; i < geo.GetPolygons().GetCount(); ++i)
    {
      ind[i * 3 + 0] = geo.GetPolygons()[i].m_Vertices[0];
      ind[i * 3 + 1] = geo.GetPolygons()[i].m_Vertices[1];
      ind[i * 3 + 2] = geo.GetPolygons()[i].m_Vertices[2];
    }

    SetBounds(xiiBoundingSphere(xiiVec3::ZeroVector(), 1.5f));
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCustomMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiCustomMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


void xiiCustomMeshRenderData::FillBatchIdAndSortingKey()
{
  const xiiUInt32 uiAdditionalBatchData = 0;

  m_uiFlipWinding  = m_GlobalTransform.ContainsNegativeScale() ? 1 : 0;
  m_uiUniformScale = m_GlobalTransform.ContainsUniformScale() ? 1 : 0;

  const xiiUInt32 uiMeshIDHash     = xiiHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
  const xiiUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? xiiHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;

  // Generate batch id from mesh, material and part index.
  xiiUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, 0 /*m_uiSubMeshIndex*/, m_uiFlipWinding, uiAdditionalBatchData};
  m_uiBatchId      = xiiHashingUtils::xxHash32(data, sizeof(data));

  // Sort by material and then by mesh
  m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + 0 /*m_uiSubMeshIndex*/) & 0xFFFE) | m_uiFlipWinding;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCustomMeshRenderer, 1, xiiRTTIDefaultAllocator<xiiCustomMeshRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCustomMeshRenderer::xiiCustomMeshRenderer()  = default;
xiiCustomMeshRenderer::~xiiCustomMeshRenderer() = default;

void xiiCustomMeshRenderer::GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::Selection);
}

void xiiCustomMeshRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(xiiGetStaticRTTI<xiiCustomMeshRenderData>());
}

void xiiCustomMeshRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  xiiRenderContext*     pRenderContext     = renderViewContext.m_pRenderContext;
  xiiGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  xiiInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<xiiInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  const xiiCustomMeshRenderData* pRenderData1st = batch.GetFirstData<xiiCustomMeshRenderData>();

  if (pRenderData1st->m_uiFlipWinding)
  {
    pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "TRUE");
  }
  else
  {
    pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  }

  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  for (auto it = batch.GetIterator<xiiCustomMeshRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const xiiCustomMeshRenderData* pRenderData = it;

    xiiResourceLock<xiiDynamicMeshBufferResource> pBuffer(pRenderData->m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);

    pRenderContext->BindMaterial(pRenderData->m_hMaterial);

    xiiUInt32                       uiInstanceDataOffset = 0;
    xiiArrayPtr<xiiPerInstanceData> instanceData         = pInstanceData->GetInstanceData(1, uiInstanceDataOffset);

    instanceData[0].GameObjectID  = pRenderData->m_uiUniqueID;
    instanceData[0].Color         = pRenderData->m_Color;
    instanceData[0].ObjectToWorld = pRenderData->m_GlobalTransform;

    if (pRenderData->m_uiUniformScale)
    {
      instanceData[0].ObjectToWorldNormal = instanceData[0].ObjectToWorld;
    }
    else
    {
      xiiMat4 objectToWorld = pRenderData->m_GlobalTransform.GetAsMat4();

      xiiMat3 mInverse = objectToWorld.GetRotationalPart();
      mInverse.Invert(0.0f).IgnoreResult();
      // we explicitly ignore the return value here (success / failure)
      // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying
      instanceData[0].ObjectToWorldNormal = mInverse.GetTranspose();
    }

    pInstanceData->UpdateInstanceData(pRenderContext, 1);

    const auto& desc = pBuffer->GetDescriptor();
    pBuffer->UpdateGpuBuffer(pGALCommandEncoder);

    // redo this after the primitive count has changed
    pRenderContext->BindMeshBuffer(pRenderData->m_hMesh);

    renderViewContext.m_pRenderContext->DrawMeshBuffer(pRenderData->m_uiNumPrimitives, pRenderData->m_uiFirstPrimitive).IgnoreResult();
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_CustomMeshComponent);
