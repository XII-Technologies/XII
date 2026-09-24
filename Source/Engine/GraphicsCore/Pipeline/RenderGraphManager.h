/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

struct XII_GRAPHICSCORE_DLL xiiRenderGraphCategory
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    ScenePreparation,
    SceneRendering,
    ScenePostProcess,
    Output,
    Background,
    AsyncCompute,

    ENUM_COUNT,
    Default = SceneRendering
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphCategory);

struct XII_GRAPHICSCORE_DLL xiiRenderGraphFrequency
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    EveryFrame,
    EveryNFrames,
    OnDemand,

    ENUM_COUNT,
    Default = EveryFrame
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphFrequency);

/// Serializable registration metadata. Category, priority and registration order form the stable
/// execution key; frequency only decides whether an entry participates in a frame.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphRegistrationDescription
{
  xiiString                        m_sName;
  xiiEnum<xiiRenderGraphCategory>  m_Category       = xiiRenderGraphCategory::SceneRendering;
  xiiEnum<xiiRenderGraphFrequency> m_Frequency      = xiiRenderGraphFrequency::EveryFrame;
  xiiInt32                         m_iPriority      = 0;
  xiiUInt32                        m_uiEveryNFrames = 1U;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphRegistrationDescription);

/// Deterministic owner and scheduler for multiple render graphs in a frame.
class XII_GRAPHICSCORE_DLL xiiRenderGraphManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphManager);

public:
  using BuildDelegate = xiiDelegate<void(xiiRenderGraph&, xiiRenderGraphBlackboard&)>;

  xiiRenderGraphManager() = default;

  [[nodiscard]] xiiRenderGraphGraphId RegisterGraph(const xiiRenderGraphRegistrationDescription& description, BuildDelegate buildDelegate);
  bool                                UnregisterGraph(xiiRenderGraphGraphId id);
  bool                                RequestExecution(xiiRenderGraphGraphId id);

  [[nodiscard]] xiiRenderGraph*           GetGraph(xiiRenderGraphGraphId id);
  [[nodiscard]] xiiRenderGraphBlackboard* GetBlackboard(xiiRenderGraphGraphId id);

  /// Builds, compiles and executes all graphs eligible for this frame in deterministic order.
  [[nodiscard]] xiiResult ExecuteFrame(xiiUInt64 uiFrameIndex, xiiGALDevice* pDevice, const xiiView* pView, xiiRenderGraphResourceCache* pResourceCache, xiiRenderGraphProfiler* pProfiler, const xiiRenderGraphCompileSettings& settings = {}, xiiStringBuilder* out_pError = nullptr);

private:
  struct Entry
  {
    xiiRenderGraphRegistrationDescription m_Description;
    xiiUniquePtr<xiiRenderGraph>          m_pGraph;
    xiiRenderGraphBlackboard              m_Blackboard;
    BuildDelegate                         m_BuildDelegate;
    xiiUInt32                             m_uiRegistrationOrder = 0U;
    bool                                  m_bOnDemandRequested  = false;
  };

  [[nodiscard]] xiiUInt32   FindEntry(xiiRenderGraphGraphId id) const;
  [[nodiscard]] static bool ShouldExecute(const Entry& entry, xiiUInt64 uiFrameIndex);

  xiiDynamicArray<xiiUniquePtr<Entry>> m_Entries;
  xiiUInt32                            m_uiNextRegistrationOrder = 0U;
};
