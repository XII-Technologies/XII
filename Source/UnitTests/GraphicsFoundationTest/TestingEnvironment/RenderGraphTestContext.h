/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

/// Owns isolated resources for tests of the low-level graph compiler and executor.
/// Tests drive this cache's frame/completion timeline explicitly; application code uses
/// xiiRenderGraphManager and xiiRenderGraphFrame instead.
class xiiRenderGraphTestContext
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphTestContext);

public:
  xiiRenderGraphTestContext() = default;
  ~xiiRenderGraphTestContext() { Shutdown(); }

  xiiResult Initialize(xiiStringView sName)
  {
    const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
    if (m_pGraph != nullptr || pDevice == nullptr)
      return XII_FAILURE;

    m_pDevice   = pDevice;
    m_pGraph    = XII_DEFAULT_NEW(xiiRenderGraph, sName);
    m_pCache    = XII_DEFAULT_NEW(xiiRenderGraphResourceCache, pDevice);
    m_pProfiler = XII_DEFAULT_NEW(xiiRenderGraphTimestampProfiler, pDevice);
    return XII_SUCCESS;
  }

  void Shutdown()
  {
    if (m_pGraph != nullptr)
      m_pDevice->WaitIdle();
    m_pGraph.Clear();
    m_pProfiler.Clear();
    m_pCache.Clear();
    m_Blackboard.Clear();
    m_pDevice.Clear();
  }

  xiiRenderGraph&                  GetGraph() { return *m_pGraph; }
  xiiRenderGraphBlackboard&        GetBlackboard() { return m_Blackboard; }
  xiiRenderGraphResourceCache&     GetResourceCache() { return *m_pCache; }
  xiiRenderGraphTimestampProfiler& GetProfiler() { return *m_pProfiler; }

private:
  xiiSharedPtr<xiiGALDevice>                    m_pDevice;
  xiiUniquePtr<xiiRenderGraph>                  m_pGraph;
  xiiUniquePtr<xiiRenderGraphResourceCache>     m_pCache;
  xiiUniquePtr<xiiRenderGraphTimestampProfiler> m_pProfiler;
  xiiRenderGraphBlackboard                      m_Blackboard;
};
