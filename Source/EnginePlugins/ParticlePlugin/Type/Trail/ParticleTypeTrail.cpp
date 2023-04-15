#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypeTrailFactory, 1, xiiRTTIDefaultAllocator<xiiParticleTypeTrailFactory>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("RenderMode", xiiParticleTypeRenderMode, m_RenderMode),
    XII_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new xiiDefaultValueAttribute(xiiStringView("{ e00262e8-58f5-42f5-880d-569257047201 }"))),// wrap in xiiStringView to prevent a memory leak report
    XII_MEMBER_PROPERTY("Segments", m_uiMaxPoints)->AddAttributes(new xiiDefaultValueAttribute(6), new xiiClampValueAttribute(3, 64)),
    XII_ENUM_MEMBER_PROPERTY("TextureAtlas", xiiParticleTextureAtlasType, m_TextureAtlasType),
    XII_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new xiiDefaultValueAttribute(1), new xiiClampValueAttribute(1, 16)),
    XII_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new xiiDefaultValueAttribute(1), new xiiClampValueAttribute(1, 16)),
    XII_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    XII_MEMBER_PROPERTY("DistortionTexture", m_sDistortionTexture)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    XII_MEMBER_PROPERTY("DistortionStrength", m_fDistortionStrength)->AddAttributes(new xiiDefaultValueAttribute(100.0f), new xiiClampValueAttribute(0.0f, 500.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypeTrail, 1, xiiRTTIDefaultAllocator<xiiParticleTypeTrail>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const xiiRTTI* xiiParticleTypeTrailFactory::GetTypeType() const
{
  return xiiGetStaticRTTI<xiiParticleTypeTrail>();
}

void xiiParticleTypeTrailFactory::CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const
{
  xiiParticleTypeTrail* pType = static_cast<xiiParticleTypeTrail*>(pObject);

  pType->m_RenderMode  = m_RenderMode;
  pType->m_uiMaxPoints = m_uiMaxPoints;
  pType->m_hTexture.Invalidate();
  pType->m_TextureAtlasType    = m_TextureAtlasType;
  pType->m_uiNumSpritesX       = m_uiNumSpritesX;
  pType->m_uiNumSpritesY       = m_uiNumSpritesY;
  pType->m_sTintColorParameter = xiiTempHashedString(m_sTintColorParameter.GetData());
  pType->m_hDistortionTexture.Invalidate();
  pType->m_fDistortionStrength = m_fDistortionStrength;

  // fixed 25 FPS for the update rate
  pType->m_UpdateDiff = xiiTime::Seconds(1.0 / 25.0); // m_UpdateDiff;

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(m_sTexture);
  if (!m_sDistortionTexture.IsEmpty())
    pType->m_hDistortionTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(m_sDistortionTexture);

  if (bFirstTime)
  {
    pType->GetOwnerSystem()->AddParticleDeathEventHandler(xiiMakeDelegate(&xiiParticleTypeTrail::OnParticleDeath, pType));

    pType->m_LastSnapshot = pType->GetOwnerEffect()->GetTotalEffectLifeTime();
  }

  // m_uiMaxPoints = xiiMath::Min<xiiUInt16>(8, m_uiMaxPoints);

  // clamp the number of points to the maximum possible count
  pType->m_uiMaxPoints = xiiMath::Min<xiiUInt16>(pType->m_uiMaxPoints, pType->ComputeTrailPointBucketSize(pType->m_uiMaxPoints));

  pType->m_uiCurFirstIndex = 1;
}

enum class TypeTrailVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added render mode
  Version_3, // added texture atlas support
  Version_4, // added tint color
  Version_5, // added distortion mode

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleTypeTrailFactory::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)TypeTrailVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_sTexture;
  inout_stream << m_uiMaxPoints;
  inout_stream << m_UpdateDiff;
  inout_stream << m_RenderMode;

  // version 3
  inout_stream << m_TextureAtlasType;
  inout_stream << m_uiNumSpritesX;
  inout_stream << m_uiNumSpritesY;

  // version 4
  inout_stream << m_sTintColorParameter;

  // version 5
  inout_stream << m_sDistortionTexture;
  inout_stream << m_fDistortionStrength;
}

void xiiParticleTypeTrailFactory::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)TypeTrailVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sTexture;
  inout_stream >> m_uiMaxPoints;
  inout_stream >> m_UpdateDiff;

  if (uiVersion >= 2)
  {
    inout_stream >> m_RenderMode;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_TextureAtlasType;
    inout_stream >> m_uiNumSpritesX;
    inout_stream >> m_uiNumSpritesY;

    if (m_TextureAtlasType == xiiParticleTextureAtlasType::None)
    {
      m_uiNumSpritesX = 1;
      m_uiNumSpritesY = 1;
    }
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_sTintColorParameter;
  }

  if (uiVersion >= 5)
  {
    inout_stream >> m_sDistortionTexture;
    inout_stream >> m_fDistortionStrength;
  }
}

//////////////////////////////////////////////////////////////////////////

xiiParticleTypeTrail::xiiParticleTypeTrail() = default;

xiiParticleTypeTrail::~xiiParticleTypeTrail()
{
  if (m_pStreamPosition != nullptr)
  {
    GetOwnerSystem()->RemoveParticleDeathEventHandler(xiiMakeDelegate(&xiiParticleTypeTrail::OnParticleDeath, this));
  }
}

void xiiParticleTypeTrail::CreateRequiredStreams()
{
  CreateStream("LifeTime", xiiProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", xiiProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", xiiProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("TrailData", xiiProcessingStream::DataType::Short2, &m_pStreamTrailData, true);

  m_pStreamVariation = nullptr;

  if (m_TextureAtlasType == xiiParticleTextureAtlasType::RandomVariations || m_TextureAtlasType == xiiParticleTextureAtlasType::RandomYAnimatedX)
  {
    CreateStream("Variation", xiiProcessingStream::DataType::Int, &m_pStreamVariation, false);
  }
}

void xiiParticleTypeTrail::ExtractTypeRenderData(xiiMsgExtractRenderData& ref_msg, const xiiTransform& instanceTransform) const
{
  XII_PROFILE_SCOPE("PFX: Trail");

  if (!m_hTexture.IsValid())
    return;

  const xiiUInt32 numActiveParticles = (xiiUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numActiveParticles == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != xiiRenderWorld::GetFrameCounter())
  {
    m_uiLastExtractedFrame = xiiRenderWorld::GetFrameCounter();

    const xiiColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, xiiColor::White);

    const xiiFloat16*        pSize      = m_pStreamSize->GetData<xiiFloat16>();
    const xiiColorLinear16f* pColor     = m_pStreamColor->GetData<xiiColorLinear16f>();
    const TrailData*         pTrailData = m_pStreamTrailData->GetData<TrailData>();
    const xiiFloat16Vec2*    pLifeTime  = m_pStreamLifeTime->GetData<xiiFloat16Vec2>();
    const xiiUInt32*         pVariation = m_pStreamVariation ? m_pStreamVariation->GetData<xiiUInt32>() : nullptr;

    const xiiUInt32 uiBucketSize = ComputeTrailPointBucketSize(m_uiMaxPoints);

    // this will automatically be deallocated at the end of the frame
    m_BaseParticleData =
      XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiBaseParticleShaderData, (xiiUInt32)GetOwnerSystem()->GetNumActiveParticles());
    m_TrailPointsShared =
      XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiVec4, (xiiUInt32)GetOwnerSystem()->GetNumActiveParticles() * uiBucketSize);
    m_TrailParticleData =
      XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiTrailParticleShaderData, (xiiUInt32)GetOwnerSystem()->GetNumActiveParticles());

    for (xiiUInt32 p = 0; p < numActiveParticles; ++p)
    {
      m_BaseParticleData[p].Size      = pSize[p];
      m_BaseParticleData[p].Color     = pColor[p].ToLinearFloat() * tintColor;
      m_BaseParticleData[p].Life      = pLifeTime[p].x * pLifeTime[p].y;
      m_BaseParticleData[p].Variation = (pVariation != nullptr) ? pVariation[p] : 0;

      m_TrailParticleData[p].NumPoints = pTrailData[p].m_uiNumPoints;
    }

    for (xiiUInt32 p = 0; p < numActiveParticles; ++p)
    {
      const xiiVec4* pTrailPositions = GetTrailPointsPositions(pTrailData[p].m_uiIndexForTrailPoints);

      xiiVec4* pRenderPositions = &m_TrailPointsShared[p * uiBucketSize];

      /// \todo This loop could be done without a condition
      for (xiiUInt32 i = 0; i < m_uiMaxPoints; ++i)
      {
        if (i > m_uiCurFirstIndex)
        {
          pRenderPositions[i] = pTrailPositions[m_uiCurFirstIndex + m_uiMaxPoints - i];
        }
        else
        {
          pRenderPositions[i] = pTrailPositions[m_uiCurFirstIndex - i];
        }
      }
    }
  }

  auto pRenderData = xiiCreateRenderDataForThisFrame<xiiParticleTrailRenderData>(nullptr);

  pRenderData->m_uiBatchId    = xiiHashingUtils::StringHashTo32(m_hTexture.GetResourceIDHash()) + m_uiMaxPoints;
  pRenderData->m_uiSortingKey = ComputeSortingKey(m_RenderMode, pRenderData->m_uiBatchId);

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_TotalEffectLifeTime   = GetOwnerEffect()->GetTotalEffectLifeTime();
  pRenderData->m_RenderMode            = m_RenderMode;
  pRenderData->m_GlobalTransform       = instanceTransform;
  pRenderData->m_uiMaxTrailPoints      = m_uiMaxPoints;
  pRenderData->m_hTexture              = m_hTexture;
  pRenderData->m_BaseParticleData      = m_BaseParticleData;
  pRenderData->m_TrailParticleData     = m_TrailParticleData;
  pRenderData->m_TrailPointsShared     = m_TrailPointsShared;
  pRenderData->m_fSnapshotFraction     = m_fSnapshotFraction;
  pRenderData->m_hDistortionTexture    = m_hDistortionTexture;
  pRenderData->m_fDistortionStrength   = m_fDistortionStrength;

  pRenderData->m_uiNumVariationsX         = 1;
  pRenderData->m_uiNumVariationsY         = 1;
  pRenderData->m_uiNumFlipbookAnimationsX = 1;
  pRenderData->m_uiNumFlipbookAnimationsY = 1;

  switch (m_TextureAtlasType)
  {
    case xiiParticleTextureAtlasType::None:
      break;

    case xiiParticleTextureAtlasType::RandomVariations:
      pRenderData->m_uiNumVariationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumVariationsY = m_uiNumSpritesY;
      break;

    case xiiParticleTextureAtlasType::FlipbookAnimation:
      pRenderData->m_uiNumFlipbookAnimationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumFlipbookAnimationsY = m_uiNumSpritesY;
      break;

    case xiiParticleTextureAtlasType::RandomYAnimatedX:
      pRenderData->m_uiNumFlipbookAnimationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumVariationsY         = m_uiNumSpritesY;
      break;
  }

  ref_msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::LitTransparent, xiiRenderData::Caching::Never);
}

void xiiParticleTypeTrail::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  TrailData* pTrailData = m_pStreamTrailData->GetWritableData<TrailData>() + uiStartIndex;

  const xiiVec4* pPosData = m_pStreamPosition->GetData<xiiVec4>() + uiStartIndex;

  const xiiUInt32 uiPrevIndex  = (m_uiCurFirstIndex > 0) ? (m_uiCurFirstIndex - 1) : (m_uiMaxPoints - 1);
  const xiiUInt32 uiPrevIndex2 = (uiPrevIndex > 0) ? (uiPrevIndex - 1) : (m_uiMaxPoints - 1);

  for (xiiUInt64 i = 0; i < uiNumElements; ++i)
  {
    const xiiVec4 vStartPos = pPosData[i];

    TrailData& td              = pTrailData[i];
    td.m_uiNumPoints           = 2;
    td.m_uiIndexForTrailPoints = GetIndexForTrailPoints();

    xiiVec4* pPos           = GetTrailPointsPositions(td.m_uiIndexForTrailPoints);
    pPos[m_uiCurFirstIndex] = vStartPos;
    pPos[uiPrevIndex]       = vStartPos;
    pPos[uiPrevIndex2]      = vStartPos;
  }
}


void xiiParticleTypeTrail::Process(xiiUInt64 uiNumElements)
{
  const xiiTime tNow = GetOwnerEffect()->GetTotalEffectLifeTime();

  TrailData*     pTrailData = m_pStreamTrailData->GetWritableData<TrailData>();
  const xiiVec4* pPosData   = m_pStreamPosition->GetData<xiiVec4>();

  if (tNow - m_LastSnapshot >= m_UpdateDiff)
  {
    m_LastSnapshot = tNow;

    m_uiCurFirstIndex = (m_uiCurFirstIndex + 1) == m_uiMaxPoints ? 0 : (m_uiCurFirstIndex + 1);

    for (xiiUInt64 i = 0; i < uiNumElements; ++i)
    {
      pTrailData[i].m_uiNumPoints = xiiMath::Min<xiiUInt16>(pTrailData[i].m_uiNumPoints + 1, m_uiMaxPoints);
    }
  }

  m_fSnapshotFraction = 1.0f - (float)((tNow - m_LastSnapshot).GetSeconds() / m_UpdateDiff.GetSeconds());

  for (xiiUInt64 i = 0; i < uiNumElements; ++i)
  {
    xiiVec4* pPositions           = GetTrailPointsPositions(pTrailData[i].m_uiIndexForTrailPoints);
    pPositions[m_uiCurFirstIndex] = pPosData[i];
  }
}

xiiUInt16 xiiParticleTypeTrail::GetIndexForTrailPoints()
{
  xiiUInt16 res = 0;

  if (!m_FreeTrailData.IsEmpty())
  {
    res = m_FreeTrailData.PeekBack();
    m_FreeTrailData.PopBack();
  }
  else
  {
    // expand the proper array

    // if (m_uiMaxPoints > 32)
    //{
    res = static_cast<xiiUInt16>(m_TrailPoints64.GetCount());
    m_TrailPoints64.ExpandAndGetRef();
    //}
    // else if (m_uiMaxPoints > 16)
    //{
    //  res = m_TrailData32.GetCount() & 0xFFFF;
    //  m_TrailData64.ExpandAndGetRef();
    //}
    // else if (m_uiMaxPoints > 8)
    //{
    //  res = m_TrailData16.GetCount() & 0xFFFF;
    //  m_TrailData64.ExpandAndGetRef();
    //}
    // else
    //{
    //  res = m_TrailData8.GetCount() & 0xFFFF;
    //  m_TrailData8.ExpandAndGetRef();
    //}
  }

  return res;
}

xiiVec4* xiiParticleTypeTrail::GetTrailPointsPositions(xiiUInt32 index)
{
  // if (m_uiMaxPoints > 32)
  {
    return &m_TrailPoints64[index].Positions[0];
  }
  // else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailPoints32[index].Positions[0];
  //}
  // else if (m_uiMaxPoints > 8)
  //{
  //  return &m_TrailPoints16[index].Positions[0];
  //}
  // else
  //{
  //  return &m_TrailPoints8[index].Positions[0];
  //}
}

const xiiVec4* xiiParticleTypeTrail::GetTrailPointsPositions(xiiUInt32 index) const
{
  // if (m_uiMaxPoints > 32)
  {
    return &m_TrailPoints64[index].Positions[0];
  }
  // else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailPoints32[index].Positions[0];
  //}
  // else if (m_uiMaxPoints > 8)
  //{
  //  return &m_TrailPoints16[index].Positions[0];
  //}
  // else
  //{
  //  return &m_TrailPoints8[index].Positions[0];
  //}
}


xiiUInt16 xiiParticleTypeTrail::ComputeTrailPointBucketSize(xiiUInt16 uiMaxTrailPoints)
{
  if (uiMaxTrailPoints > 32)
  {
    return 64;
  }
  else if (uiMaxTrailPoints > 16)
  {
    return 32;
  }
  else if (uiMaxTrailPoints > 8)
  {
    return 16;
  }
  else
  {
    return 8;
  }
}

void xiiParticleTypeTrail::OnParticleDeath(const xiiStreamGroupElementRemovedEvent& e)
{
  const TrailData* pTrailData = m_pStreamTrailData->GetData<TrailData>();

  // return the trail data to the list of free elements
  m_FreeTrailData.PushBack(pTrailData[e.m_uiElementIndex].m_uiIndexForTrailPoints);
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Trail_ParticleTypeTrail);
