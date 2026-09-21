/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Query.h>

/// Abstract interface for per-pass GPU timing instrumentation in the render graph.
///
/// Implement this interface to capture GPU performance data during render graph execution.
/// The executor calls OnGraphBegin / OnGraphEnd around each queue submission command list,
/// and OnPassBegin / OnPassEnd around each pass's RecordCommands call.
/// OnFrameEnd is called after all passes have been submitted so implementations can schedule readback or finalize timing data.
class XII_GRAPHICSCORE_DLL xiiRenderGraphProfiler
{
public:
  xiiRenderGraphProfiler()          = default;
  virtual ~xiiRenderGraphProfiler() = default;

  /// Called at the start of a submission command list.
  virtual void OnGraphBegin(xiiGALCommandList& commandList, xiiUInt32 uiSubmissionIndex) = 0;

  /// Called at the end of a submission command list.
  virtual void OnGraphEnd(xiiGALCommandList& commandList, xiiUInt32 uiSubmissionIndex) = 0;

  /// Called immediately before a pass records its commands. Insert a begin-query here.
  virtual void OnPassBegin(xiiGALCommandList& commandList, xiiStringView sPassName, xiiUInt32 uiPassIndex) = 0;

  /// Called immediately after a pass records its commands. Insert an end-query here.
  virtual void OnPassEnd(xiiGALCommandList& commandList, xiiStringView sPassName, xiiUInt32 uiPassIndex) = 0;

  /// Called once per frame after all passes have been submitted.
  ///        Implementations should schedule result readback here (with appropriate frame delay).
  virtual void OnFrameEnd(xiiUInt64 uiFrameIndex) = 0;

  /// Returns the last resolved GPU duration for the given pass in milliseconds.
  ///        Returns 0.0f if no data is yet available (warmup frames).
  [[nodiscard]] virtual float GetPassDurationMs(xiiStringView sPassName) const = 0;
};

/// A concrete render graph profiler that uses GPU Duration queries.
///
/// Uses xiiGALQueryType::Duration to bracket each pass with BeginQuery / EndQuery.
/// Results are read back with a configurable number of frames of delay (default: 2) to avoid GPU stalls.
/// Up to xiiRenderGraphTimestampProfiler::MaxTrackedPasses distinct pass names can be tracked simultaneously.
class XII_GRAPHICSCORE_DLL xiiRenderGraphTimestampProfiler final : public xiiRenderGraphProfiler
{
public:
  static constexpr xiiUInt32 s_uiRingFrameCount   = 3U; ///< Number of in-flight frames buffered.
  static constexpr xiiUInt32 s_uiMaxTrackedPasses = 128U;

public:
  xiiRenderGraphTimestampProfiler();
  ~xiiRenderGraphTimestampProfiler() override;

  /// Initializes the profiler with the device that will be used to create queries.
  void Initialize(xiiSharedPtr<xiiGALDevice> pDevice);

  /// Releases all GPU queries.
  void Shutdown();

  // xiiRenderGraphProfiler interface
  void OnGraphBegin(xiiGALCommandList& commandList, xiiUInt32 uiSubmissionIndex) override;
  void OnGraphEnd(xiiGALCommandList& commandList, xiiUInt32 uiSubmissionIndex) override;
  void OnPassBegin(xiiGALCommandList& commandList, xiiStringView sPassName, xiiUInt32 uiPassIndex) override;
  void OnPassEnd(xiiGALCommandList& commandList, xiiStringView sPassName, xiiUInt32 uiPassIndex) override;
  void OnFrameEnd(xiiUInt64 uiFrameIndex) override;

  /// Returns the last resolved GPU duration for the given pass in milliseconds.
  [[nodiscard]] float GetPassDurationMs(xiiStringView sPassName) const override;

  /// Returns the last resolved GPU duration for the entire frame in milliseconds.
  [[nodiscard]] float GetFrameDurationMs() const;

private:
  struct PassQueries
  {
    xiiSharedPtr<xiiGALQuery> m_pDurationQuery;
    xiiHashedString           m_sPassName;
    bool                      m_bActive = false;
  };

  struct FrameData
  {
    xiiDynamicArray<PassQueries>               m_PassQueries;
    xiiDynamicArray<xiiSharedPtr<xiiGALQuery>> m_SubmissionDurationQueries;
    xiiDynamicArray<bool>                      m_SubmissionQueryActive;
    xiiUInt64                                  m_uiFrameIndex = xiiInvalidIndex;
  };

  void ReadbackFrame(FrameData& frameData);

private:
  xiiSharedPtr<xiiGALDevice>           m_pDevice;
  FrameData                            m_FrameRing[s_uiRingFrameCount];
  xiiUInt32                            m_uiCurrentRingSlot = 0U;
  xiiHashTable<xiiHashedString, float> m_ResolvedDurationsMs;
  mutable xiiMutex                     m_ResultMutex;
};
