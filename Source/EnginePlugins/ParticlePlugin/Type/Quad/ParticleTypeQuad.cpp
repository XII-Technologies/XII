#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiQuadParticleOrientation, 2)
  XII_ENUM_CONSTANTS(xiiQuadParticleOrientation::Billboard)
  XII_ENUM_CONSTANTS(xiiQuadParticleOrientation::Rotating_OrthoEmitterDir, xiiQuadParticleOrientation::Rotating_EmitterDir)
  XII_ENUM_CONSTANTS(xiiQuadParticleOrientation::Fixed_EmitterDir, xiiQuadParticleOrientation::Fixed_RandomDir, xiiQuadParticleOrientation::Fixed_WorldUp)
  XII_ENUM_CONSTANTS(xiiQuadParticleOrientation::FixedAxis_EmitterDir, xiiQuadParticleOrientation::FixedAxis_ParticleDir)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypeQuadFactory, 2, xiiRTTIDefaultAllocator<xiiParticleTypeQuadFactory>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Orientation", xiiQuadParticleOrientation, m_Orientation),
    XII_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(0), xiiAngle::Degree(90))),
    XII_ENUM_MEMBER_PROPERTY("RenderMode", xiiParticleTypeRenderMode, m_RenderMode),
    XII_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new xiiDefaultValueAttribute(xiiStringView("{ e00262e8-58f5-42f5-880d-569257047201 }"))),// wrap in xiiStringView to prevent a memory leak report
    XII_ENUM_MEMBER_PROPERTY("TextureAtlas", xiiParticleTextureAtlasType, m_TextureAtlasType),
    XII_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new xiiDefaultValueAttribute(1), new xiiClampValueAttribute(1, 16)),
    XII_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new xiiDefaultValueAttribute(1), new xiiClampValueAttribute(1, 16)),
    XII_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    XII_MEMBER_PROPERTY("DistortionTexture", m_sDistortionTexture)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    XII_MEMBER_PROPERTY("DistortionStrength", m_fDistortionStrength)->AddAttributes(new xiiDefaultValueAttribute(100.0f), new xiiClampValueAttribute(0.0f, 500.0f)),
    XII_MEMBER_PROPERTY("ParticleStretch", m_fStretch)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(-100.0f, 100.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypeQuad, 1, xiiRTTIDefaultAllocator<xiiParticleTypeQuad>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const xiiRTTI* xiiParticleTypeQuadFactory::GetTypeType() const
{
  return xiiGetStaticRTTI<xiiParticleTypeQuad>();
}

void xiiParticleTypeQuadFactory::CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const
{
  xiiParticleTypeQuad* pType = static_cast<xiiParticleTypeQuad*>(pObject);

  pType->m_Orientation  = m_Orientation;
  pType->m_MaxDeviation = m_MaxDeviation;
  pType->m_hTexture.Invalidate();
  pType->m_RenderMode          = m_RenderMode;
  pType->m_uiNumSpritesX       = m_uiNumSpritesX;
  pType->m_uiNumSpritesY       = m_uiNumSpritesY;
  pType->m_sTintColorParameter = xiiTempHashedString(m_sTintColorParameter.GetData());
  pType->m_hDistortionTexture.Invalidate();
  pType->m_fDistortionStrength = m_fDistortionStrength;
  pType->m_TextureAtlasType    = m_TextureAtlasType;
  pType->m_fStretch            = m_fStretch;

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(m_sTexture);
  if (!m_sDistortionTexture.IsEmpty())
    pType->m_hDistortionTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(m_sDistortionTexture);
}

enum class TypeQuadVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // sprite deviation
  Version_3, // distortion
  Version_4, // added texture atlas type
  Version_5, // added particle stretch

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleTypeQuadFactory::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = (int)TypeQuadVersion::Version_Current;
  stream << uiVersion;

  stream << m_Orientation;
  stream << m_RenderMode;
  stream << m_sTexture;
  stream << m_uiNumSpritesX;
  stream << m_uiNumSpritesY;
  stream << m_sTintColorParameter;
  stream << m_MaxDeviation;
  stream << m_sDistortionTexture;
  stream << m_fDistortionStrength;
  stream << m_TextureAtlasType;

  // Version 5
  stream << m_fStretch;
}

void xiiParticleTypeQuadFactory::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)TypeQuadVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_Orientation;
  stream >> m_RenderMode;
  stream >> m_sTexture;
  stream >> m_uiNumSpritesX;
  stream >> m_uiNumSpritesY;
  stream >> m_sTintColorParameter;

  if (uiVersion >= 2)
  {
    stream >> m_MaxDeviation;
  }

  if (uiVersion >= 3)
  {
    stream >> m_sDistortionTexture;
    stream >> m_fDistortionStrength;
  }

  if (uiVersion >= 4)
  {
    stream >> m_TextureAtlasType;

    if (m_TextureAtlasType == xiiParticleTextureAtlasType::None)
    {
      m_uiNumSpritesX = 1;
      m_uiNumSpritesY = 1;
    }
  }

  if (uiVersion >= 5)
  {
    stream >> m_fStretch;
  }
}

void xiiParticleTypeQuadFactory::QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_FinalizerDeps) const
{
  if (m_Orientation == xiiQuadParticleOrientation::FixedAxis_ParticleDir)
  {
    inout_FinalizerDeps.Insert(xiiGetStaticRTTI<xiiParticleFinalizerFactory_LastPosition>());
  }
}

xiiParticleTypeQuad::xiiParticleTypeQuad()  = default;
xiiParticleTypeQuad::~xiiParticleTypeQuad() = default;

void xiiParticleTypeQuad::CreateRequiredStreams()
{
  CreateStream("LifeTime", xiiProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", xiiProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", xiiProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", xiiProcessingStream::DataType::Half, &m_pStreamRotationSpeed, false);
  CreateStream("RotationOffset", xiiProcessingStream::DataType::Half, &m_pStreamRotationOffset, false);

  m_pStreamAxis         = nullptr;
  m_pStreamVariation    = nullptr;
  m_pStreamLastPosition = nullptr;

  if (m_Orientation == xiiQuadParticleOrientation::Fixed_RandomDir || m_Orientation == xiiQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == xiiQuadParticleOrientation::Fixed_WorldUp)
  {
    CreateStream("Axis", xiiProcessingStream::DataType::Float3, &m_pStreamAxis, true);
  }

  if (m_TextureAtlasType == xiiParticleTextureAtlasType::RandomVariations || m_TextureAtlasType == xiiParticleTextureAtlasType::RandomYAnimatedX)
  {
    CreateStream("Variation", xiiProcessingStream::DataType::Int, &m_pStreamVariation, false);
  }

  if (m_Orientation == xiiQuadParticleOrientation::FixedAxis_ParticleDir)
  {
    CreateStream("LastPosition", xiiProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
  }
}

struct sodComparer
{
  // sort farther particles to the front, so that they get rendered first (back to front)
  XII_ALWAYS_INLINE bool Less(const xiiParticleTypeQuad::sod& a, const xiiParticleTypeQuad::sod& b) const { return a.dist > b.dist; }
  XII_ALWAYS_INLINE bool Equal(const xiiParticleTypeQuad::sod& a, const xiiParticleTypeQuad::sod& b) const { return a.dist == b.dist; }
};

void xiiParticleTypeQuad::ExtractTypeRenderData(xiiMsgExtractRenderData& msg, const xiiTransform& instanceTransform) const
{
  XII_PROFILE_SCOPE("PFX: Quad");

  const xiiUInt32 numParticles = (xiiUInt32)GetOwnerSystem()->GetNumActiveParticles();
  if (!m_hTexture.IsValid() || numParticles == 0)
    return;

  const bool bNeedsSorting = (m_RenderMode == xiiParticleTypeRenderMode::Blended) || (m_RenderMode == xiiParticleTypeRenderMode::BlendedForeground) || (m_RenderMode == xiiParticleTypeRenderMode::BlendedBackground) || (m_RenderMode == xiiParticleTypeRenderMode::BlendAdd);

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if ((m_uiLastExtractedFrame != xiiRenderWorld::GetFrameCounter())
      /*&& !bNeedsSorting*/) // TODO: in theory every shared instance has to sort the Quads, in practice this maybe should be an option
  {
    m_uiLastExtractedFrame = xiiRenderWorld::GetFrameCounter();

    if (bNeedsSorting)
    {
      // TODO: Using the frame allocator this way results in memory corruptions.
      // Not sure, whether this is supposed to work.
      xiiHybridArray<sod, 64> sorted; // (xiiFrameAllocator::GetCurrentAllocator());
      sorted.SetCountUninitialized(numParticles);

      const xiiVec3  vCameraPos = msg.m_pView->GetCullingCamera()->GetCenterPosition();
      const xiiVec4* pPosition  = m_pStreamPosition->GetData<xiiVec4>();

      for (xiiUInt32 p = 0; p < numParticles; ++p)
      {
        sorted[p].dist  = (pPosition[p].GetAsVec3() - vCameraPos).GetLengthSquared();
        sorted[p].index = p;
      }

      sorted.Sort(sodComparer());

      CreateExtractedData(&sorted);
    }
    else
    {
      CreateExtractedData(nullptr);
    }
  }

  AddParticleRenderData(msg, instanceTransform);
}

XII_ALWAYS_INLINE xiiUInt32 noRedirect(xiiUInt32 idx, const xiiHybridArray<xiiParticleTypeQuad::sod, 64>* pSorted)
{
  return idx;
}

XII_ALWAYS_INLINE xiiUInt32 sortedRedirect(xiiUInt32 idx, const xiiHybridArray<xiiParticleTypeQuad::sod, 64>* pSorted)
{
  return (*pSorted)[idx].index;
}

void xiiParticleTypeQuad::CreateExtractedData(const xiiHybridArray<sod, 64>* pSorted) const
{
  auto redirect = (pSorted != nullptr) ? sortedRedirect : noRedirect;

  const xiiUInt32 numParticles = (xiiUInt32)GetOwnerSystem()->GetNumActiveParticles();

  const bool bNeedsBillboardData = m_Orientation == xiiQuadParticleOrientation::Billboard;
  const bool bNeedsTangentData   = !bNeedsBillboardData;

  const xiiVec3 vEmitterPos      = GetOwnerSystem()->GetTransform().m_vPosition;
  const xiiVec3 vEmitterDir      = GetOwnerSystem()->GetTransform().m_qRotation * xiiVec3(0, 0, 1); // Z axis
  const xiiVec3 vEmitterDirOrtho = vEmitterDir.GetOrthogonalVector();

  const xiiTime  tCur      = GetOwnerEffect()->GetTotalEffectLifeTime();
  const xiiColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, xiiColor::White);

  const xiiFloat16Vec2*    pLifeTime       = m_pStreamLifeTime->GetData<xiiFloat16Vec2>();
  const xiiVec4*           pPosition       = m_pStreamPosition->GetData<xiiVec4>();
  const xiiFloat16*        pSize           = m_pStreamSize->GetData<xiiFloat16>();
  const xiiColorLinear16f* pColor          = m_pStreamColor->GetData<xiiColorLinear16f>();
  const xiiFloat16*        pRotationSpeed  = m_pStreamRotationSpeed->GetData<xiiFloat16>();
  const xiiFloat16*        pRotationOffset = m_pStreamRotationOffset->GetData<xiiFloat16>();
  const xiiVec3*           pAxis           = m_pStreamAxis ? m_pStreamAxis->GetData<xiiVec3>() : nullptr;
  const xiiUInt32*         pVariation      = m_pStreamVariation ? m_pStreamVariation->GetData<xiiUInt32>() : nullptr;
  const xiiVec3*           pLastPosition   = m_pStreamLastPosition ? m_pStreamLastPosition->GetData<xiiVec3>() : nullptr;

  // this will automatically be deallocated at the end of the frame
  m_BaseParticleData = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiBaseParticleShaderData, numParticles);

  AllocateParticleData(numParticles, bNeedsBillboardData, bNeedsTangentData);

  auto SetBaseData = [&](xiiUInt32 dstIdx, xiiUInt32 srcIdx) {
    m_BaseParticleData[dstIdx].Size      = pSize[srcIdx];
    m_BaseParticleData[dstIdx].Color     = pColor[srcIdx].ToLinearFloat() * tintColor;
    m_BaseParticleData[dstIdx].Life      = pLifeTime[srcIdx].x * pLifeTime[srcIdx].y;
    m_BaseParticleData[dstIdx].Variation = (pVariation != nullptr) ? pVariation[srcIdx] : 0;
  };

  auto SetBillboardData = [&](xiiUInt32 dstIdx, xiiUInt32 srcIdx) {
    m_BillboardParticleData[dstIdx].Position       = pPosition[srcIdx].GetAsVec3();
    m_BillboardParticleData[dstIdx].RotationOffset = pRotationOffset[srcIdx];
    m_BillboardParticleData[dstIdx].RotationSpeed  = pRotationSpeed[srcIdx];
  };

  auto SetTangentDataEmitterDir = [&](xiiUInt32 dstIdx, xiiUInt32 srcIdx) {
    xiiMat3 mRotation;
    mRotation.SetRotationMatrix(vEmitterDir, xiiAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[srcIdx]) + pRotationOffset[srcIdx]));

    m_TangentParticleData[dstIdx].Position = pPosition[srcIdx].GetAsVec3();
    m_TangentParticleData[dstIdx].TangentX = mRotation * vEmitterDirOrtho;
    m_TangentParticleData[dstIdx].TangentZ = vEmitterDir;
  };

  auto SetTangentDataEmitterDirOrtho = [&](xiiUInt32 dstIdx, xiiUInt32 srcIdx) {
    const xiiVec3 vDirToParticle = (pPosition[srcIdx].GetAsVec3() - vEmitterPos);
    xiiVec3       vOrthoDir      = vEmitterDir.CrossRH(vDirToParticle);
    vOrthoDir.NormalizeIfNotZero(xiiVec3(1, 0, 0)).IgnoreResult();

    xiiMat3 mRotation;
    mRotation.SetRotationMatrix(vOrthoDir, xiiAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[srcIdx]) + pRotationOffset[srcIdx]));

    m_TangentParticleData[dstIdx].Position = pPosition[srcIdx].GetAsVec3();
    m_TangentParticleData[dstIdx].TangentX = vOrthoDir;
    m_TangentParticleData[dstIdx].TangentZ = mRotation * vEmitterDir;
  };

  auto SetTangentDataFromAxis = [&](xiiUInt32 dstIdx, xiiUInt32 srcIdx) {
    xiiVec3 vNormal = pAxis[srcIdx];
    vNormal.Normalize();

    const xiiVec3 vTangentStart = vNormal.GetOrthogonalVector().GetNormalized();

    xiiMat3 mRotation;
    mRotation.SetRotationMatrix(vNormal, xiiAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[srcIdx]) + pRotationOffset[srcIdx]));

    const xiiVec3 vTangentX = mRotation * vTangentStart;

    m_TangentParticleData[dstIdx].Position = pPosition[srcIdx].GetAsVec3();
    m_TangentParticleData[dstIdx].TangentX = vTangentX;
    m_TangentParticleData[dstIdx].TangentZ = vTangentX.CrossRH(vNormal);
  };

  auto SetTangentDataAligned_Emitter = [&](xiiUInt32 dstIdx, xiiUInt32 srcIdx) {
    m_TangentParticleData[dstIdx].Position   = pPosition[srcIdx].GetAsVec3();
    m_TangentParticleData[dstIdx].TangentX   = vEmitterDir;
    m_TangentParticleData[dstIdx].TangentZ.x = m_fStretch;
  };

  auto SetTangentDataAligned_ParticleDir = [&](xiiUInt32 dstIdx, xiiUInt32 srcIdx) {
    const xiiVec3 vCurPos                    = pPosition[srcIdx].GetAsVec3();
    const xiiVec3 vLastPos                   = pLastPosition[srcIdx];
    const xiiVec3 vDir                       = vCurPos - vLastPos;
    m_TangentParticleData[dstIdx].Position   = vCurPos;
    m_TangentParticleData[dstIdx].TangentX   = vDir;
    m_TangentParticleData[dstIdx].TangentZ.x = m_fStretch;
  };

  for (xiiUInt32 p = 0; p < numParticles; ++p)
  {
    SetBaseData(p, redirect(p, pSorted));
  }

  if (bNeedsBillboardData)
  {
    for (xiiUInt32 p = 0; p < numParticles; ++p)
    {
      SetBillboardData(p, redirect(p, pSorted));
    }
  }

  if (bNeedsTangentData)
  {
    if (m_Orientation == xiiQuadParticleOrientation::Rotating_EmitterDir)
    {
      for (xiiUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataEmitterDir(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == xiiQuadParticleOrientation::Rotating_OrthoEmitterDir)
    {
      for (xiiUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataEmitterDirOrtho(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == xiiQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == xiiQuadParticleOrientation::Fixed_RandomDir || m_Orientation == xiiQuadParticleOrientation::Fixed_WorldUp)
    {
      for (xiiUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataFromAxis(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == xiiQuadParticleOrientation::FixedAxis_EmitterDir)
    {
      for (xiiUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataAligned_Emitter(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == xiiQuadParticleOrientation::FixedAxis_ParticleDir)
    {
      for (xiiUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataAligned_ParticleDir(p, redirect(p, pSorted));
      }
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void xiiParticleTypeQuad::AddParticleRenderData(xiiMsgExtractRenderData& msg, const xiiTransform& instanceTransform) const
{
  auto pRenderData = xiiCreateRenderDataForThisFrame<xiiParticleQuadRenderData>(nullptr);

  pRenderData->m_uiBatchId    = xiiHashingUtils::StringHashTo32(m_hTexture.GetResourceIDHash());
  pRenderData->m_uiSortingKey = ComputeSortingKey(m_RenderMode, pRenderData->m_uiBatchId);

  pRenderData->m_bApplyObjectTransform    = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_GlobalTransform          = instanceTransform;
  pRenderData->m_TotalEffectLifeTime      = GetOwnerEffect()->GetTotalEffectLifeTime();
  pRenderData->m_RenderMode               = m_RenderMode;
  pRenderData->m_hTexture                 = m_hTexture;
  pRenderData->m_BaseParticleData         = m_BaseParticleData;
  pRenderData->m_BillboardParticleData    = m_BillboardParticleData;
  pRenderData->m_TangentParticleData      = m_TangentParticleData;
  pRenderData->m_uiNumVariationsX         = 1;
  pRenderData->m_uiNumVariationsY         = 1;
  pRenderData->m_uiNumFlipbookAnimationsX = 1;
  pRenderData->m_uiNumFlipbookAnimationsY = 1;
  pRenderData->m_hDistortionTexture       = m_hDistortionTexture;
  pRenderData->m_fDistortionStrength      = m_fDistortionStrength;

  switch (m_Orientation)
  {
    case xiiQuadParticleOrientation::Billboard:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_BILLBOARD";
      break;
    case xiiQuadParticleOrientation::Rotating_OrthoEmitterDir:
    case xiiQuadParticleOrientation::Rotating_EmitterDir:
    case xiiQuadParticleOrientation::Fixed_EmitterDir:
    case xiiQuadParticleOrientation::Fixed_WorldUp:
    case xiiQuadParticleOrientation::Fixed_RandomDir:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_TANGENTS";
      break;
    case xiiQuadParticleOrientation::FixedAxis_EmitterDir:
    case xiiQuadParticleOrientation::FixedAxis_ParticleDir:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_AXIS_ALIGNED";
      break;
  }

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

  msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::LitTransparent, xiiRenderData::Caching::Never);
}

void xiiParticleTypeQuad::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  if (m_pStreamAxis != nullptr)
  {
    xiiVec3*   pAxis = m_pStreamAxis->GetWritableData<xiiVec3>();
    xiiRandom& rng   = GetRNG();

    if (m_Orientation == xiiQuadParticleOrientation::Fixed_RandomDir)
    {
      XII_PROFILE_SCOPE("PFX: Init Quad Axis Random");

      for (xiiUInt32 i = 0; i < uiNumElements; ++i)
      {
        const xiiUInt64 uiElementIdx = uiStartIndex + i;

        pAxis[uiElementIdx] = xiiVec3::CreateRandomDirection(rng);
      }
    }
    else if (m_Orientation == xiiQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == xiiQuadParticleOrientation::Fixed_WorldUp)
    {
      XII_PROFILE_SCOPE("PFX: Init Quad Axis");

      xiiVec3 vNormal;

      if (m_Orientation == xiiQuadParticleOrientation::Fixed_EmitterDir)
      {
        vNormal = GetOwnerSystem()->GetTransform().m_qRotation * xiiVec3(0, 0, 1); // Z axis
      }
      else if (m_Orientation == xiiQuadParticleOrientation::Fixed_WorldUp)
      {
        xiiCoordinateSystem coord;
        GetOwnerSystem()->GetWorld()->GetCoordinateSystem(GetOwnerSystem()->GetTransform().m_vPosition, coord);

        vNormal = coord.m_vUpDir;
      }

      if (m_MaxDeviation > xiiAngle::Degree(1.0f))
      {
        // how to get from the X axis to the desired normal
        xiiQuat qRotToDir;
        qRotToDir.SetShortestRotation(xiiVec3(1, 0, 0), vNormal);

        for (xiiUInt32 i = 0; i < uiNumElements; ++i)
        {
          const xiiUInt64 uiElementIdx = uiStartIndex + i;
          const xiiVec3   vRandomX     = xiiVec3::CreateRandomDeviationX(rng, m_MaxDeviation);

          pAxis[uiElementIdx] = qRotToDir * vRandomX;
        }
      }
      else
      {
        for (xiiUInt32 i = 0; i < uiNumElements; ++i)
        {
          const xiiUInt64 uiElementIdx = uiStartIndex + i;
          pAxis[uiElementIdx]          = vNormal;
        }
      }
    }
  }
}

void xiiParticleTypeQuad::AllocateParticleData(const xiiUInt32 numParticles, const bool bNeedsBillboardData, const bool bNeedsTangentData) const
{
  m_BillboardParticleData = nullptr;
  if (bNeedsBillboardData)
  {
    m_BillboardParticleData = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiBillboardQuadParticleShaderData, numParticles);
  }

  m_TangentParticleData = nullptr;
  if (bNeedsTangentData)
  {
    m_TangentParticleData = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiTangentQuadParticleShaderData, (xiiUInt32)GetOwnerSystem()->GetNumActiveParticles());
  }
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class xiiQuadParticleOrientationPatch_1_2 final : public xiiGraphPatch
{
public:
  xiiQuadParticleOrientationPatch_1_2() :
    xiiGraphPatch("xiiQuadParticleOrientation", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    // TODO: this type of patch does not work

    pNode->RenameProperty("FragmentOrthogonalEmitterDirection", "Rotating_OrthoEmitterDir");
    pNode->RenameProperty("FragmentEmitterDirection", "Rotating_EmitterDir");

    pNode->RenameProperty("SpriteEmitterDirection", "Fixed_EmitterDir");
    pNode->RenameProperty("SpriteRandom", "Fixed_RandomDir");
    pNode->RenameProperty("SpriteWorldUp", "Fixed_WorldUp");

    pNode->RenameProperty("AxisAligned_Emitter", "FixedAxis_EmitterDir");
  }
};

xiiQuadParticleOrientationPatch_1_2 g_xiiQuadParticleOrientationPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class xiiParticleTypeQuadFactory_1_2 final : public xiiGraphPatch
{
public:
  xiiParticleTypeQuadFactory_1_2() :
    xiiGraphPatch("xiiParticleTypeQuadFactory", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    xiiAbstractObjectNode::Property* pProp = pNode->FindProperty("Orientation");
    const xiiStringBuilder           sOri  = pProp->m_Value.Get<xiiString>();

    if (sOri == "xiiQuadParticleOrientation::FragmentOrthogonalEmitterDirection")
      pProp->m_Value = "xiiQuadParticleOrientation::Rotating_OrthoEmitterDir";

    if (sOri == "xiiQuadParticleOrientation::FragmentEmitterDirection")
      pProp->m_Value = "xiiQuadParticleOrientation::Rotating_EmitterDir";

    if (sOri == "xiiQuadParticleOrientation::SpriteEmitterDirection")
      pProp->m_Value = "xiiQuadParticleOrientation::Fixed_EmitterDir";

    if (sOri == "xiiQuadParticleOrientation::SpriteRandom")
      pProp->m_Value = "xiiQuadParticleOrientation::Fixed_RandomDir";

    if (sOri == "xiiQuadParticleOrientation::SpriteWorldUp")
      pProp->m_Value = "xiiQuadParticleOrientation::Fixed_WorldUp";

    if (sOri == "xiiQuadParticleOrientation::AxisAligned_Emitter")
      pProp->m_Value = "xiiQuadParticleOrientation::FixedAxis_EmitterDir";
  }
};

xiiParticleTypeQuadFactory_1_2 g_xiiParticleTypeQuadFactory_1_2;
