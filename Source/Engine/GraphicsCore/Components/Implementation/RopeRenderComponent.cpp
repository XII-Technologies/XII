#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/AnimationSystem/Declarations.h>
#include <GraphicsCore/Components/RopeRenderComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Meshes/SkinnedMeshComponent.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Shader/Types.h>

xiiCVarBool cvar_FeatureRopesVisBones("Feature.Ropes.VisBones", false, xiiCVarFlags::Default, "Enables debug visualization of rope bones");

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRopeRenderComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::White), new xiiExposeColorAlphaAttribute()),
    XII_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new xiiDefaultValueAttribute(0.05f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Detail", GetDetail, SetDetail)->AddAttributes(new xiiDefaultValueAttribute(6), new xiiClampValueAttribute(3, 16)),
    XII_ACCESSOR_PROPERTY("Subdivide", GetSubdivide, SetSubdivide),
    XII_ACCESSOR_PROPERTY("UScale", GetUScale, SetUScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgRopePoseUpdated, OnRopePoseUpdated),
    XII_MESSAGE_HANDLER(xiiMsgSetColor, OnMsgSetColor),
    XII_MESSAGE_HANDLER(xiiMsgSetMeshMaterial, OnMsgSetMeshMaterial),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Effects/Ropes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRopeRenderComponent::xiiRopeRenderComponent()  = default;
xiiRopeRenderComponent::~xiiRopeRenderComponent() = default;

void xiiRopeRenderComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Color;
  s << m_hMaterial;
  s << m_fThickness;
  s << m_uiDetail;
  s << m_bSubdivide;
  s << m_fUScale;
}

void xiiRopeRenderComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_hMaterial;
  s >> m_fThickness;
  s >> m_uiDetail;
  s >> m_bSubdivide;
  s >> m_fUScale;
}

void xiiRopeRenderComponent::OnActivated()
{
  SUPER::OnActivated();

  m_LocalBounds = xiiBoundingBoxSphere::MakeInvalid();
}

void xiiRopeRenderComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  SUPER::OnDeactivated();
}

xiiResult xiiRopeRenderComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  bounds = m_LocalBounds;
  return XII_SUCCESS;
}

void xiiRopeRenderComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const xiiUInt32 uiFlipWinding  = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const xiiUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
  xiiMaterialResourceHandle        hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[0];

  xiiSkinnedMeshRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiSkinnedMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds    = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh           = m_hMesh;
    pRenderData->m_hMaterial       = hMaterial;
    pRenderData->m_Color           = m_Color;

    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiFlipWinding  = uiFlipWinding;
    pRenderData->m_uiUniformScale = uiUniformScale;

    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    m_SkinningState.FillSkinnedMeshRenderData(*pRenderData);

    pRenderData->FillBatchIdAndSortingKey();
  }

  // Determine render data category.
  xiiRenderData::Category category = xiiDefaultRenderDataCategories::LitOpaque;

  if (hMaterial.IsValid())
  {
    xiiResourceLock<xiiMaterialResource> pMaterial(hMaterial, xiiResourceAcquireMode::AllowLoadingFallback);
    category = pMaterial->GetRenderDataCategory();
  }

  msg.AddRenderData(pRenderData, category, xiiRenderData::Caching::Never);

  if (cvar_FeatureRopesVisBones)
  {
    xiiHybridArray<xiiDebugRendererLine, 128> lines(xiiFrameAllocator::GetCurrentAllocator());
    lines.Reserve(m_SkinningState.m_Transforms.GetCount() * 3);

    xiiMat4 offsetMat;
    offsetMat.SetIdentity();

    for (xiiUInt32 i = 0; i < m_SkinningState.m_Transforms.GetCount(); ++i)
    {
      offsetMat.SetTranslationVector(xiiVec3(static_cast<float>(i), 0, 0));
      xiiMat4 skinningMat = m_SkinningState.m_Transforms[i].GetAsMat4() * offsetMat;

      xiiVec3 pos = skinningMat.GetTranslationVector();

      auto& x        = lines.ExpandAndGetRef();
      x.m_vStart     = pos;
      x.m_vEnd       = x.m_vStart + skinningMat.TransformDirection(xiiVec3::MakeAxisX());
      x.m_StartColor = xiiColor::Red;
      x.m_EndColor   = xiiColor::Red;

      auto& y        = lines.ExpandAndGetRef();
      y.m_vStart     = pos;
      y.m_vEnd       = y.m_vStart + skinningMat.TransformDirection(xiiVec3::MakeAxisY() * 2.0f);
      y.m_StartColor = xiiColor::Green;
      y.m_EndColor   = xiiColor::Green;

      auto& z        = lines.ExpandAndGetRef();
      z.m_vStart     = pos;
      z.m_vEnd       = z.m_vStart + skinningMat.TransformDirection(xiiVec3::MakeAxisZ() * 2.0f);
      z.m_StartColor = xiiColor::Blue;
      z.m_EndColor   = xiiColor::Blue;
    }

    xiiDebugRenderer::DrawLines(msg.m_pView->GetHandle(), lines, xiiColor::White, GetOwner()->GetGlobalTransform());
  }
}

void xiiRopeRenderComponent::SetMaterialFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* xiiRopeRenderComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void xiiRopeRenderComponent::SetThickness(float fThickness)
{
  if (m_fThickness != fThickness)
  {
    m_fThickness = fThickness;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      xiiHybridArray<xiiTransform, 128> transforms;
      transforms.SetCountUninitialized(m_SkinningState.m_Transforms.GetCount());

      xiiMat4 offsetMat;
      offsetMat.SetIdentity();

      for (xiiUInt32 i = 0; i < m_SkinningState.m_Transforms.GetCount(); ++i)
      {
        offsetMat.SetTranslationVector(xiiVec3(static_cast<float>(i), 0, 0));
        xiiMat4 skinningMat = m_SkinningState.m_Transforms[i].GetAsMat4() * offsetMat;

        transforms[i] = xiiTransform::MakeFromMat4(skinningMat);
      }

      UpdateSkinningTransformBuffer(transforms);
    }
  }
}

void xiiRopeRenderComponent::SetDetail(xiiUInt32 uiDetail)
{
  if (m_uiDetail != uiDetail)
  {
    m_uiDetail = uiDetail;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void xiiRopeRenderComponent::SetSubdivide(bool bSubdivide)
{
  if (m_bSubdivide != bSubdivide)
  {
    m_bSubdivide = bSubdivide;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void xiiRopeRenderComponent::SetUScale(float fUScale)
{
  if (m_fUScale != fUScale)
  {
    m_fUScale = fUScale;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void xiiRopeRenderComponent::OnMsgSetColor(xiiMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);
}

void xiiRopeRenderComponent::OnMsgSetMeshMaterial(xiiMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_hMaterial);
}

void xiiRopeRenderComponent::OnRopePoseUpdated(xiiMsgRopePoseUpdated& msg)
{
  if (msg.m_LinkTransforms.IsEmpty())
    return;

  if (m_SkinningState.m_Transforms.GetCount() != msg.m_LinkTransforms.GetCount())
  {
    m_SkinningState.Clear();

    GenerateRenderMesh(msg.m_LinkTransforms.GetCount());
  }

  UpdateSkinningTransformBuffer(msg.m_LinkTransforms);

  xiiBoundingBox newBounds = xiiBoundingBox::MakeFromPoints(&msg.m_LinkTransforms[0].m_vPosition, msg.m_LinkTransforms.GetCount(), sizeof(xiiTransform));

  // if the existing bounds are big enough, don't update them
  if (!m_LocalBounds.IsValid() || !m_LocalBounds.GetBox().Contains(newBounds))
  {
    m_LocalBounds.ExpandToInclude(xiiBoundingBoxSphere(newBounds));

    TriggerLocalBoundsUpdate();
  }
}

void xiiRopeRenderComponent::GenerateRenderMesh(xiiUInt32 uiNumRopePieces)
{
  xiiStringBuilder sResourceName;
  sResourceName.SetFormat("Rope-Mesh:{}{}-d{}-u{}", uiNumRopePieces, m_bSubdivide ? "Sub" : "", m_uiDetail, m_fUScale);

  m_hMesh = xiiResourceManager::GetExistingResource<xiiMeshResource>(sResourceName);
  if (m_hMesh.IsValid())
    return;

  xiiGeometry geom;

  const xiiAngle fDegStep = xiiAngle::MakeFromDegree(360.0f / m_uiDetail);
  const float    fVStep   = 1.0f / m_uiDetail;

  auto addCap = [&](float x, const xiiVec3& vNormal, xiiUInt16 uiBoneIndex, bool bFlipWinding) {
    xiiVec4U16 boneIndices(uiBoneIndex, 0, 0, 0);

    xiiUInt32 centerIndex = geom.AddVertex(xiiVec3(x, 0, 0), vNormal, xiiVec2(0.5f, 0.5f), xiiColor::White, boneIndices);

    xiiAngle deg = xiiAngle::MakeFromRadian(0);
    for (xiiUInt32 s = 0; s < m_uiDetail; ++s)
    {
      const float fY = xiiMath::Cos(deg);
      const float fZ = xiiMath::Sin(deg);

      geom.AddVertex(xiiVec3(x, fY, fZ), vNormal, xiiVec2(fY, fZ), xiiColor::White, boneIndices);

      deg += fDegStep;
    }

    xiiUInt32 triangle[3];
    triangle[0] = centerIndex;
    for (xiiUInt32 s = 0; s < m_uiDetail; ++s)
    {
      triangle[1] = s + triangle[0] + 1;
      triangle[2] = ((s + 1) % m_uiDetail) + triangle[0] + 1;

      geom.AddPolygon(triangle, bFlipWinding);
    }
  };

  auto addPiece = [&](float x, const xiiVec4U16& vBoneIndices, const xiiColorLinearUB& boneWeights, bool bCreatePolygons) {
    xiiAngle deg = xiiAngle::MakeFromRadian(0);
    float    fU  = x * m_fUScale;
    float    fV  = 0;

    for (xiiUInt32 s = 0; s <= m_uiDetail; ++s)
    {
      const float fY = xiiMath::Cos(deg);
      const float fZ = xiiMath::Sin(deg);

      const xiiVec3 pos(x, fY, fZ);
      const xiiVec3 normal(0, fY, fZ);

      geom.AddVertex(pos, normal, xiiVec2(fU, fV), xiiColor::White, vBoneIndices, boneWeights);

      deg += fDegStep;
      fV += fVStep;
    }

    if (bCreatePolygons)
    {
      xiiUInt32 endIndex   = geom.GetVertices().GetCount() - (m_uiDetail + 1);
      xiiUInt32 startIndex = endIndex - (m_uiDetail + 1);

      xiiUInt32 triangle[3];
      for (xiiUInt32 s = 0; s < m_uiDetail; ++s)
      {
        triangle[0] = startIndex + s;
        triangle[1] = startIndex + s + 1;
        triangle[2] = endIndex + s + 1;
        geom.AddPolygon(triangle, false);

        triangle[0] = startIndex + s;
        triangle[1] = endIndex + s + 1;
        triangle[2] = endIndex + s;
        geom.AddPolygon(triangle, false);
      }
    }
  };

  // cap
  {
    const xiiVec3 normal = xiiVec3(-1, 0, 0);
    addCap(0.0f, normal, 0, true);
  }

  // pieces
  {
    // first ring full weight to first bone
    addPiece(0.0f, xiiVec4U16(0, 0, 0, 0), xiiColorLinearUB(255, 0, 0, 0), false);

    xiiUInt16 p = 1;

    if (m_bSubdivide)
    {
      addPiece(0.75f, xiiVec4U16(0, 0, 0, 0), xiiColorLinearUB(255, 0, 0, 0), true);

      for (; p < uiNumRopePieces - 2; ++p)
      {
        addPiece(static_cast<float>(p) + 0.25f, xiiVec4U16(p, 0, 0, 0), xiiColorLinearUB(255, 0, 0, 0), true);
        addPiece(static_cast<float>(p) + 0.75f, xiiVec4U16(p, 0, 0, 0), xiiColorLinearUB(255, 0, 0, 0), true);
      }

      addPiece(static_cast<float>(p) + 0.25f, xiiVec4U16(p, 0, 0, 0), xiiColorLinearUB(255, 0, 0, 0), true);
      ++p;
    }
    else
    {
      for (; p < uiNumRopePieces - 1; ++p)
      {
        // Middle rings half weight between bones. To ensure that weights sum up to 1 we weight one bone with 128 and the other with 127,
        // since "ubyte normalized" can't represent 0.5 perfectly.
        addPiece(static_cast<float>(p), xiiVec4U16(p - 1, p, 0, 0), xiiColorLinearUB(128, 127, 0, 0), true);
      }
    }

    // last ring full weight to last bone
    addPiece(static_cast<float>(p), xiiVec4U16(p, 0, 0, 0), xiiColorLinearUB(255, 0, 0, 0), true);
  }

  // cap
  {
    const xiiVec3 normal = xiiVec3(1, 0, 0);
    addCap(static_cast<float>(uiNumRopePieces - 1), normal, static_cast<xiiUInt16>(uiNumRopePieces - 1), false);
  }

  geom.ComputeTangents();

  xiiMeshResourceDescriptor desc;

  // Data/Base/Materials/Prototyping/PrototypeBlack.xiiMaterialAsset
  desc.SetMaterial(0, "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }");

  auto& meshBufferDesc = desc.MeshBufferDesc();
  meshBufferDesc.AddCommonStreams();
  meshBufferDesc.AddStream(xiiGALInputLayoutSemantic::BoneIndices0, xiiGALResourceFormat::RGBA8UInt);
  meshBufferDesc.AddStream(xiiGALInputLayoutSemantic::BoneWeights0, xiiGALResourceFormat::RGBA8UNormalized);
  meshBufferDesc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList);

  desc.AddSubMesh(meshBufferDesc.GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();

  m_hMesh = xiiResourceManager::CreateResource<xiiMeshResource>(sResourceName, std::move(desc), sResourceName);
}

void xiiRopeRenderComponent::UpdateSkinningTransformBuffer(xiiArrayPtr<const xiiTransform> skinningTransforms)
{
  xiiMat4 bindPoseMat;
  bindPoseMat.SetIdentity();
  m_SkinningState.m_Transforms.SetCountUninitialized(skinningTransforms.GetCount());

  const xiiVec3 newScale = xiiVec3(1.0f, m_fThickness * 0.5f, m_fThickness * 0.5f);
  for (xiiUInt32 i = 0; i < skinningTransforms.GetCount(); ++i)
  {
    xiiTransform t = skinningTransforms[i];
    t.m_vScale     = newScale;

    // scale x axis to match the distance between this bone and the next bone
    if (i < skinningTransforms.GetCount() - 1)
    {
      t.m_vScale.x = (skinningTransforms[i + 1].m_vPosition - skinningTransforms[i].m_vPosition).GetLength();
    }

    bindPoseMat.SetTranslationVector(xiiVec3(-static_cast<float>(i), 0, 0));

    m_SkinningState.m_Transforms[i] = t.GetAsMat4() * bindPoseMat;
  }

  m_SkinningState.TransformsChanged();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_RopeRenderComponent);
