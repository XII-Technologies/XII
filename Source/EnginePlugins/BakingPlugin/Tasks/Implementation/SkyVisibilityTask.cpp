#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/Tasks/SkyVisibilityTask.h>
#include <BakingPlugin/Tracer/TracerInterface.h>
#include <RendererCore/BakedProbes/BakingInterface.h>

using namespace xiiBakingInternal;

SkyVisibilityTask::SkyVisibilityTask(const xiiBakingSettings& settings, xiiTracerInterface& tracer, xiiArrayPtr<const xiiVec3> probePositions) :
  m_Settings(settings), m_Tracer(tracer), m_ProbePositions(probePositions)
{
}

SkyVisibilityTask::~SkyVisibilityTask() = default;

void SkyVisibilityTask::Execute()
{
  m_SkyVisibility.SetCountUninitialized(m_ProbePositions.GetCount());

  const xiiUInt32                              uiNumSamples = m_Settings.m_uiNumSamplesPerProbe;
  xiiHybridArray<xiiTracerInterface::Ray, 128> rays(xiiFrameAllocator::GetCurrentAllocator());
  rays.SetCountUninitialized(uiNumSamples);

  xiiAmbientCube<float> weightNormalization;
  for (xiiUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
  {
    auto& ray       = rays[uiSampleIndex];
    ray.m_vDir      = xiiBakingUtils::FibonacciSphere(uiSampleIndex, uiNumSamples);
    ray.m_fDistance = m_Settings.m_fMaxRayDistance;

    weightNormalization.AddSample(ray.m_vDir, 1.0f);
  }

  for (xiiUInt32 i = 0; i < xiiAmbientCubeBasis::NumDirs; ++i)
  {
    weightNormalization.m_Values[i] = 1.0f / weightNormalization.m_Values[i];
  }

  xiiHybridArray<xiiTracerInterface::Hit, 128> hits(xiiFrameAllocator::GetCurrentAllocator());
  hits.SetCountUninitialized(uiNumSamples);

  for (xiiUInt32 uiProbeIndex = 0; uiProbeIndex < m_ProbePositions.GetCount(); ++uiProbeIndex)
  {
    xiiVec3 probePos = m_ProbePositions[uiProbeIndex];
    for (xiiUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
    {
      rays[uiSampleIndex].m_vStartPos = probePos;
    }

    m_Tracer.TraceRays(rays, hits);

    xiiAmbientCube<float> skyVisibility;
    for (xiiUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
    {
      const auto& ray   = rays[uiSampleIndex];
      const auto& hit   = hits[uiSampleIndex];
      const float value = hit.m_fDistance < 0.0f ? 1.0f : 0.0f;

      skyVisibility.AddSample(ray.m_vDir, value);
    }

    for (xiiUInt32 i = 0; i < xiiAmbientCubeBasis::NumDirs; ++i)
    {
      skyVisibility.m_Values[i] *= weightNormalization.m_Values[i];
    }
    auto& compressedSkyVisibility = m_SkyVisibility[uiProbeIndex];
    compressedSkyVisibility       = xiiBakingUtils::CompressSkyVisibility(skyVisibility);
  }
}
