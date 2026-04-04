#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Passes 53-54 â€” Exposure Histogram + Eye Adaptation.
///
/// Async Compute. Pass 53 bins HDR luminance into a 256-bucket histogram via atomics.
/// Pass 54 reads the histogram and computes the current adapted exposure value using
/// a smoothed adaptation curve. Both passes keep persistent buffers across frames for
/// temporal smoothing.
struct XII_GRAPHICSCORE_DLL xiiExposurePass
{
  float m_fMinEV100           = -4.0f;  ///< Minimum log2 exposure value.
  float m_fMaxEV100           =  16.0f; ///< Maximum log2 exposure value.
  float m_fAdaptationSpeed    =  2.0f;  ///< EV100 per second adaptation speed.
  float m_fLowPercent         =  45.0f; ///< Lower histogram percentile for metering.
  float m_fHighPercent        =  95.0f; ///< Upper histogram percentile for metering.
  xiiSharedPtr<xiiGALBuffer> m_pHistogramBuffer;  ///< RW: 256 x uint32 luminance bins.
  xiiSharedPtr<xiiGALBuffer> m_pExposureBuffer;   ///< RW: single float exposure + debug data.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "ExposurePass";
  bool      m_bActive = true;
};

void xiiPopulateExposurePass(xiiExposurePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

