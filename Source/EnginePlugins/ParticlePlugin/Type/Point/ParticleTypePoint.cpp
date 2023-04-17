#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypePointFactory, 1, xiiRTTIDefaultAllocator<xiiParticleTypePointFactory>)
{
  //XII_BEGIN_ATTRIBUTES
  //{
  //  new xiiHiddenAttribute()
  //}
  //XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypePoint, 1, xiiRTTIDefaultAllocator<xiiParticleTypePoint>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const xiiRTTI* xiiParticleTypePointFactory::GetTypeType() const
{
  return xiiGetStaticRTTI<xiiParticleTypePoint>();
}

void xiiParticleTypePointFactory::CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const
{
  // xiiParticleTypePoint* pType = static_cast<xiiParticleTypePoint*>(pObject);
}

enum class TypePointVersion
{
  Version_0 = 0,


  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleTypePointFactory::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)TypePointVersion::Version_Current;
  inout_stream << uiVersion;
}

void xiiParticleTypePointFactory::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)TypePointVersion::Version_Current, "Invalid version {0}", uiVersion);
}

void xiiParticleTypePoint::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Color", xiiProcessingStream::DataType::Half4, &m_pStreamColor, false);
}

void xiiParticleTypePoint::ExtractTypeRenderData(xiiMsgExtractRenderData& ref_msg, const xiiTransform& instanceTransform) const
{
  XII_PROFILE_SCOPE("PFX: Point");

  const xiiUInt32 numParticles = (xiiUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != xiiRenderWorld::GetFrameCounter())
  {
    m_uiLastExtractedFrame = xiiRenderWorld::GetFrameCounter();

    const xiiVec4*           pPosition = m_pStreamPosition->GetData<xiiVec4>();
    const xiiColorLinear16f* pColor    = m_pStreamColor->GetData<xiiColorLinear16f>();

    // this will automatically be deallocated at the end of the frame
    m_BaseParticleData      = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiBaseParticleShaderData, numParticles);
    m_BillboardParticleData = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiBillboardQuadParticleShaderData, numParticles);

    for (xiiUInt32 p = 0; p < numParticles; ++p)
    {
      m_BaseParticleData[p].Color         = pColor[p].ToLinearFloat();
      m_BillboardParticleData[p].Position = pPosition[p].GetAsVec3();
    }
  }

  auto pRenderData = xiiCreateRenderDataForThisFrame<xiiParticlePointRenderData>(nullptr);

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_GlobalTransform       = instanceTransform;
  pRenderData->m_TotalEffectLifeTime   = GetOwnerEffect()->GetTotalEffectLifeTime();
  pRenderData->m_BaseParticleData      = m_BaseParticleData;
  pRenderData->m_BillboardParticleData = m_BillboardParticleData;

  ref_msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::LitTransparent, xiiRenderData::Caching::Never);
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Point_ParticleTypePoint);
