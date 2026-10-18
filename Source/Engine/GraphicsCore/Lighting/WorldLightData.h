/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Core/World/Declarations.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
class xiiWorld;
class xiiRenderData;
class xiiWorldLightDataState;
struct xiiWorldLightUploadStatistics
{
  xiiUInt64 m_uiBytesUploaded = 0;
  xiiUInt32 m_uiRanges = 0, m_uiUploadPasses = 0;
};
/// Derived canonical light/IES upload data for one authoritative world. No scene manipulation API.
/// Updates and borrowed CPU reads require the existing world extraction boundary.
class XII_GRAPHICSCORE_DLL xiiWorldLightData
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiWorldLightData);

public:
  explicit xiiWorldLightData(const xiiWorld& world);
  ~xiiWorldLightData();
  xiiResult                            UpdateFromExtraction(xiiArrayPtr<xiiRenderData* const> packets);
  xiiArrayPtr<const xiiGpuLightData>   GetLights() const;
  xiiUInt32                            GetIndex(xiiComponentHandle component) const;
  xiiResult                            Resolve(xiiUInt32 identity, xiiComponentHandle& out_component) const;
  const xiiWorldLightUploadStatistics& GetUploadStatistics() const;
  struct UploadHandles
  {
    xiiRenderGraphBufferHandle m_Lights, m_Profiles, m_Counts;
  };
  /// Retains upload bytes/state. Repeated views share completed allocations and one upload per setup.
  UploadHandles AddUploadPass(xiiRenderGraph& graph) const;

private:
  xiiSharedPtr<xiiWorldLightDataState> m_State;
};
