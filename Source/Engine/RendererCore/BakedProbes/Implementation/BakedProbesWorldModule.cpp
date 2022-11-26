#include <RendererCore/RendererCorePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <RendererCore/BakedProbes/BakedProbesWorldModule.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>

// clang-format off
XII_IMPLEMENT_WORLD_MODULE(xiiBakedProbesWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBakedProbesWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

xiiBakedProbesWorldModule::xiiBakedProbesWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiBakedProbesWorldModule::~xiiBakedProbesWorldModule() = default;

void xiiBakedProbesWorldModule::Initialize()
{
}

void xiiBakedProbesWorldModule::Deinitialize()
{
}

bool xiiBakedProbesWorldModule::HasProbeData() const
{
  return m_hProbeTree.IsValid();
}

xiiResult xiiBakedProbesWorldModule::GetProbeIndexData(const xiiVec3& globalPosition, const xiiVec3& normal, ProbeIndexData& out_ProbeIndexData) const
{
  // TODO: optimize

  if (!HasProbeData())
    return XII_FAILURE;

  xiiResourceLock<xiiProbeTreeSectorResource> pProbeTree(m_hProbeTree, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pProbeTree.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return XII_FAILURE;

  xiiSimdVec4f gridSpacePos = xiiSimdConversion::ToVec3((globalPosition - pProbeTree->GetGridOrigin()).CompDiv(pProbeTree->GetProbeSpacing()));
  gridSpacePos              = gridSpacePos.CompMax(xiiSimdVec4f::ZeroVector());

  xiiSimdVec4f gridSpacePosFloor = gridSpacePos.Floor();
  xiiSimdVec4f weights           = gridSpacePos - gridSpacePosFloor;

  xiiSimdVec4i maxIndices = xiiSimdVec4i(pProbeTree->GetProbeCount().x, pProbeTree->GetProbeCount().y, pProbeTree->GetProbeCount().z) - xiiSimdVec4i(1);
  xiiSimdVec4i pos0       = xiiSimdVec4i::Truncate(gridSpacePosFloor).CompMin(maxIndices);
  xiiSimdVec4i pos1       = (pos0 + xiiSimdVec4i(1)).CompMin(maxIndices);

  xiiUInt32 x0 = pos0.x();
  xiiUInt32 y0 = pos0.y();
  xiiUInt32 z0 = pos0.z();

  xiiUInt32 x1 = pos1.x();
  xiiUInt32 y1 = pos1.y();
  xiiUInt32 z1 = pos1.z();

  xiiUInt32 xCount  = pProbeTree->GetProbeCount().x;
  xiiUInt32 xyCount = xCount * pProbeTree->GetProbeCount().y;

  out_ProbeIndexData.m_probeIndices[0] = z0 * xyCount + y0 * xCount + x0;
  out_ProbeIndexData.m_probeIndices[1] = z0 * xyCount + y0 * xCount + x1;
  out_ProbeIndexData.m_probeIndices[2] = z0 * xyCount + y1 * xCount + x0;
  out_ProbeIndexData.m_probeIndices[3] = z0 * xyCount + y1 * xCount + x1;
  out_ProbeIndexData.m_probeIndices[4] = z1 * xyCount + y0 * xCount + x0;
  out_ProbeIndexData.m_probeIndices[5] = z1 * xyCount + y0 * xCount + x1;
  out_ProbeIndexData.m_probeIndices[6] = z1 * xyCount + y1 * xCount + x0;
  out_ProbeIndexData.m_probeIndices[7] = z1 * xyCount + y1 * xCount + x1;

  xiiVec3 w1 = xiiSimdConversion::ToVec3(weights);
  xiiVec3 w0 = xiiVec3(1.0f) - w1;

  // TODO: add geometry factor to weight
  out_ProbeIndexData.m_probeWeights[0] = w0.x * w0.y * w0.z;
  out_ProbeIndexData.m_probeWeights[1] = w1.x * w0.y * w0.z;
  out_ProbeIndexData.m_probeWeights[2] = w0.x * w1.y * w0.z;
  out_ProbeIndexData.m_probeWeights[3] = w1.x * w1.y * w0.z;
  out_ProbeIndexData.m_probeWeights[4] = w0.x * w0.y * w1.z;
  out_ProbeIndexData.m_probeWeights[5] = w1.x * w0.y * w1.z;
  out_ProbeIndexData.m_probeWeights[6] = w0.x * w1.y * w1.z;
  out_ProbeIndexData.m_probeWeights[7] = w1.x * w1.y * w1.z;

  float weightSum = 0;
  for (xiiUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    weightSum += out_ProbeIndexData.m_probeWeights[i];
  }

  float normalizeFactor = 1.0f / weightSum;
  for (xiiUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    out_ProbeIndexData.m_probeWeights[i] *= normalizeFactor;
  }

  return XII_SUCCESS;
}

xiiAmbientCube<float> xiiBakedProbesWorldModule::GetSkyVisibility(const ProbeIndexData& indexData) const
{
  // TODO: optimize

  xiiAmbientCube<float> result;

  xiiResourceLock<xiiProbeTreeSectorResource> pProbeTree(m_hProbeTree, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pProbeTree.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return result;

  auto                  compressedSkyVisibility = pProbeTree->GetSkyVisibility();
  xiiAmbientCube<float> skyVisibility;

  for (xiiUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    xiiBakingUtils::DecompressSkyVisibility(compressedSkyVisibility[indexData.m_probeIndices[i]], skyVisibility);

    for (xiiUInt32 d = 0; d < xiiAmbientCubeBasis::NumDirs; ++d)
    {
      result.m_Values[d] += skyVisibility.m_Values[d] * indexData.m_probeWeights[i];
    }
  }

  return result;
}

void xiiBakedProbesWorldModule::SetProbeTreeResourcePrefix(const xiiHashedString& prefix)
{
  xiiStringBuilder sResourcePath;
  sResourcePath.Format("{}_Global.xiiProbeTreeSector", prefix);

  m_hProbeTree = xiiResourceManager::LoadResource<xiiProbeTreeSectorResource>(sResourcePath);
}
