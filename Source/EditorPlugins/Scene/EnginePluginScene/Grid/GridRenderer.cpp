#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Grid/GridRenderer.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Resources/Buffer.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGridRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorGridExtractor, 1, xiiRTTIDefaultAllocator<xiiEditorGridExtractor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGridRenderer, 1, xiiRTTIDefaultAllocator<xiiGridRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEditorGridExtractor::xiiEditorGridExtractor(xiiStringView sName) :
  xiiExtractor(sName)
{
  m_pSceneContext = nullptr;
}

xiiGridRenderer::xiiGridRenderer()
{
  CreateVertexBuffer();
}

void xiiGridRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(xiiGetStaticRTTI<xiiGridRenderData>());
}

void xiiGridRenderer::GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(xiiDefaultRenderDataCategories::SimpleTransparent);
}

void xiiGridRenderer::CreateVertexBuffer()
{
  if (!m_hVertexBuffer.IsInvalidated())
    return;

  // load the shader
  {
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Debug/DebugPrimitive.xiiShader");
  }

  // Create the vertex buffer
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiElementByteStride = sizeof(GridVertex);
    desc.m_uiSize              = s_uiBufferSize;
    desc.m_BindFlags           = xiiGALBindFlags::VertexBuffer;
    desc.m_ResourceUsage       = xiiGALResourceUsage::Dynamic;
    desc.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

    m_hVertexBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  // Setup the input layout
  {
    {
      xiiVertexStreamInfo& si = m_InputLayoutInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic           = xiiGALInputLayoutSemantic::Position;
      si.m_Format             = xiiGALResourceFormat::RGB32Float;
      si.m_uiOffset           = 0;
      si.m_uiElementSize      = 12;
    }

    {
      xiiVertexStreamInfo& si = m_InputLayoutInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic           = xiiGALInputLayoutSemantic::Color0;
      si.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
      si.m_uiOffset           = 12;
      si.m_uiElementSize      = 4;
    }
  }
}

void xiiGridRenderer::CreateGrid(const xiiGridRenderData& rd) const
{
  m_Vertices.Clear();
  m_Vertices.Reserve(100);

  const xiiVec3  vCenter    = rd.m_GlobalTransform.m_vPosition;
  const xiiVec3  vTangent1  = rd.m_GlobalTransform.m_qRotation * xiiVec3(1, 0, 0);
  const xiiVec3  vTangent2  = rd.m_GlobalTransform.m_qRotation * xiiVec3(0, 1, 0);
  const xiiInt32 iNumLines1 = rd.m_iLastLine1 - rd.m_iFirstLine1;
  const xiiInt32 iNumLines2 = rd.m_iLastLine2 - rd.m_iFirstLine2;
  const float    maxExtent1 = iNumLines1 * rd.m_fDensity;
  const float    maxExtent2 = iNumLines2 * rd.m_fDensity;

  xiiColor cCenter = xiiColorScheme::GetColor(xiiColorScheme::Blue, 6, 0.5f);
  xiiColor cTen    = xiiColorScheme::LightUI(xiiColorScheme::Gray) * 0.7f;
  xiiColor cOther  = xiiColorScheme::LightUI(xiiColorScheme::Gray) * 0.5f;

  if (rd.m_bOrthoMode)
  {
    // dimmer colors in ortho mode, to be more in the background
    cCenter *= 0.5f;
    cTen *= 0.4f;
    cOther *= 0.4f;

    // in ortho mode, the origin lines are highlighted when global space is enabled
    if (!rd.m_bGlobal)
    {
      cCenter = cTen;
    }
  }
  else
  {
    // in perspective mode, the lines through the object are highlighted when local space is enabled
    if (rd.m_bGlobal)
    {
      cCenter = cTen;
    }
  }

  const xiiVec3 vCorner = vCenter + rd.m_iFirstLine1 * rd.m_fDensity * vTangent1 + rd.m_iFirstLine2 * rd.m_fDensity * vTangent2;

  for (xiiInt32 i = 0; i <= iNumLines1; ++i)
  {
    const xiiInt32 iLineIdx = rd.m_iFirstLine1 + i;

    xiiColor cCur = cOther;

    if (iLineIdx == 0)
      cCur = cCenter;
    else if (iLineIdx % 10 == 0)
      cCur = cTen;

    auto& v1 = m_Vertices.ExpandAndGetRef();
    auto& v2 = m_Vertices.ExpandAndGetRef();

    v1.m_color    = cCur;
    v1.m_position = vCorner + vTangent1 * rd.m_fDensity * (float)i;

    v2.m_color    = cCur;
    v2.m_position = vCorner + vTangent1 * rd.m_fDensity * (float)i + vTangent2 * maxExtent2;
  }

  for (xiiInt32 i = 0; i <= iNumLines2; ++i)
  {
    const xiiInt32 iLineIdx = rd.m_iFirstLine2 + i;

    xiiColor cCur = cOther;

    if (iLineIdx == 0)
      cCur = cCenter;
    else if (iLineIdx % 10 == 0)
      cCur = cTen;

    auto& v1 = m_Vertices.ExpandAndGetRef();
    auto& v2 = m_Vertices.ExpandAndGetRef();

    v1.m_color    = cCur;
    v1.m_position = vCorner + vTangent2 * rd.m_fDensity * (float)i;

    v2.m_color    = cCur;
    v2.m_position = vCorner + vTangent2 * rd.m_fDensity * (float)i + vTangent1 * maxExtent1;
  }
}

void xiiGridRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  for (auto it = batch.GetIterator<xiiGridRenderData>(); it.IsValid(); ++it)
  {
    CreateGrid(*it);

    if (m_Vertices.IsEmpty())
      return;

    xiiRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

    pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
    pRenderContext->BindShader(m_hShader);

    xiiUInt32         uiNumLineVertices = m_Vertices.GetCount();
    const GridVertex* pLineData         = m_Vertices.GetData();

    while (uiNumLineVertices > 0)
    {
      const xiiUInt32 uiNumLineVerticesInBatch = xiiMath::Min<xiiUInt32>(uiNumLineVertices, s_uiLineVerticesPerBatch);
      XII_ASSERT_DEBUG(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");

      pRenderContext->GetCommandList()->UpdateBufferExtended(m_hVertexBuffer, 0, xiiMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray());

      pRenderContext->BindMeshBuffer(m_hVertexBuffer, xiiGALBufferHandle(), &m_InputLayoutInfo, xiiGALPrimitiveTopology::LineList, uiNumLineVerticesInBatch / 2);
      pRenderContext->DrawMeshBuffer().IgnoreResult();

      uiNumLineVertices -= uiNumLineVerticesInBatch;
      pLineData += s_uiLineVerticesPerBatch;
    }
  }
}

float AdjustGridDensity(float fDensity, xiiUInt32 uiWindowWidth, float fOrthoDimX, xiiUInt32 uiMinPixelsDist)
{
  xiiInt32 iFactor     = 1;
  float    fNewDensity = fDensity;

  while (true)
  {
    const float stepsAtDensity     = fOrthoDimX / fNewDensity;
    const float minPixelsAtDensity = stepsAtDensity * uiMinPixelsDist;

    if (minPixelsAtDensity < uiWindowWidth)
      break;

    iFactor *= 10;
    fNewDensity = fDensity * iFactor;
  }

  return fNewDensity;
}

void xiiEditorGridExtractor::Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData)
{
  if (m_pSceneContext == nullptr || m_pSceneContext->GetGridDensity() == 0.0f)
    return;

  const xiiCamera* cam      = view.GetCamera();
  float            fDensity = m_pSceneContext->GetGridDensity();

  xiiGridRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiGridRenderData>(nullptr);
  pRenderData->m_bOrthoMode      = cam->IsOrthographic();
  pRenderData->m_bGlobal         = m_pSceneContext->IsGridInGlobalSpace();
  pRenderData->m_GlobalBounds    = xiiBoundingBoxSphere::MakeInvalid();

  if (cam->IsOrthographic())
  {
    const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;
    const float fDimX        = cam->GetDimensionX(fAspectRatio) * 0.5f;
    const float fDimY        = cam->GetDimensionY(fAspectRatio) * 0.5f;

    fDensity                = AdjustGridDensity(fDensity, (xiiUInt32)view.GetViewport().width, fDimX, 10);
    pRenderData->m_fDensity = fDensity;

    pRenderData->m_GlobalTransform.SetIdentity();
    pRenderData->m_GlobalTransform.m_vPosition = cam->GetCenterDirForwards() * cam->GetFarPlane() * 0.9f;

    xiiMat3 mRot;
    mRot.SetColumn(0, cam->GetCenterDirRight());
    mRot.SetColumn(1, cam->GetCenterDirUp());
    mRot.SetColumn(2, cam->GetCenterDirForwards());
    pRenderData->m_GlobalTransform.m_qRotation = xiiQuat::MakeFromMat3(mRot);

    const xiiVec3 vBottomLeft = cam->GetCenterPosition() - cam->GetCenterDirRight() * fDimX - cam->GetCenterDirUp() * fDimY;
    const xiiVec3 vTopRight   = cam->GetCenterPosition() + cam->GetCenterDirRight() * fDimX + cam->GetCenterDirUp() * fDimY;

    xiiPlane plane1, plane2;
    plane1 = xiiPlane::MakeFromNormalAndPoint(cam->GetCenterDirRight(), xiiVec3(0));
    plane2 = xiiPlane::MakeFromNormalAndPoint(cam->GetCenterDirUp(), xiiVec3(0));

    const float fFirstDist1 = plane1.GetDistanceTo(vBottomLeft) - fDensity;
    const float fLastDist1  = plane1.GetDistanceTo(vTopRight) + fDensity;

    const float fFirstDist2 = plane2.GetDistanceTo(vBottomLeft) - fDensity;
    const float fLastDist2  = plane2.GetDistanceTo(vTopRight) + fDensity;

    xiiVec3& val = pRenderData->m_GlobalTransform.m_vPosition;
    val.x        = xiiMath::RoundToMultiple(val.x, pRenderData->m_fDensity);
    val.y        = xiiMath::RoundToMultiple(val.y, pRenderData->m_fDensity);
    val.z        = xiiMath::RoundToMultiple(val.z, pRenderData->m_fDensity);

    pRenderData->m_iFirstLine1 = (xiiInt32)xiiMath::Trunc(fFirstDist1 / fDensity);
    pRenderData->m_iLastLine1  = (xiiInt32)xiiMath::Trunc(fLastDist1 / fDensity);
    pRenderData->m_iFirstLine2 = (xiiInt32)xiiMath::Trunc(fFirstDist2 / fDensity);
    pRenderData->m_iLastLine2  = (xiiInt32)xiiMath::Trunc(fLastDist2 / fDensity);
  }
  else
  {
    pRenderData->m_GlobalTransform = m_pSceneContext->GetGridTransform();

    // grid is disabled
    if (pRenderData->m_GlobalTransform.m_vScale.IsZero(0.001f))
      return;

    pRenderData->m_fDensity = fDensity;

    const xiiInt32 iNumLines   = 50;
    pRenderData->m_iFirstLine1 = -iNumLines;
    pRenderData->m_iLastLine1  = iNumLines;
    pRenderData->m_iFirstLine2 = -iNumLines;
    pRenderData->m_iLastLine2  = iNumLines;
  }

  ref_extractedRenderData.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::SimpleTransparent);
}

xiiResult xiiEditorGridExtractor::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return XII_SUCCESS;
}

xiiResult xiiEditorGridExtractor::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  return XII_SUCCESS;
}
