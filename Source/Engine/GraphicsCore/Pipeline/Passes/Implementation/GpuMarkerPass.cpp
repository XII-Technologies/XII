#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/GpuMarkerPass.h>

xiiRenderGraphGpuMarkerPass::xiiRenderGraphGpuMarkerPass() : m_MarkerColor(xiiColor::White)
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("GpuMarker");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = true;

  m_sMarkerName = "GpuMarker";
}

void xiiRenderGraphGpuMarkerPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphGpuMarkerPass::SetQueueFlags(xiiBitflags<xiiGALCommandQueueFlags> queueFlags)
{
  m_PassDescription.m_QueueFlags = queueFlags;
}

void xiiRenderGraphGpuMarkerPass::SetHasSideEffects(bool bHasSideEffects)
{
  m_PassDescription.m_bHasSideEffects = bHasSideEffects;
}

void xiiRenderGraphGpuMarkerPass::SetMarkerMode(MarkerMode markerMode)
{
  m_MarkerMode = markerMode;
}

void xiiRenderGraphGpuMarkerPass::SetMarkerName(xiiStringView sMarkerName)
{
  m_sMarkerName = sMarkerName;
}

void xiiRenderGraphGpuMarkerPass::SetMarkerColor(const xiiColor& markerColor)
{
  m_MarkerColor = markerColor;
}

void xiiRenderGraphGpuMarkerPass::SetPreMarkerCommandListFunc(PreMarkerCommandListFunc preMarkerCommandListFunc)
{
  m_PreMarkerCommandListFunc = preMarkerCommandListFunc;
}

void xiiRenderGraphGpuMarkerPass::SetPostMarkerCommandListFunc(PostMarkerCommandListFunc postMarkerCommandListFunc)
{
  m_PostMarkerCommandListFunc = postMarkerCommandListFunc;
}

void xiiRenderGraphGpuMarkerPass::ClearPreMarkerCommandListFunc()
{
  m_PreMarkerCommandListFunc = {};
}

void xiiRenderGraphGpuMarkerPass::ClearPostMarkerCommandListFunc()
{
  m_PostMarkerCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphGpuMarkerPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphGpuMarkerPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
{
  if (!m_bEnabled || executionContext.m_pCommandList == nullptr)
  {
    return;
  }

  if (m_PreMarkerCommandListFunc.IsValid())
  {
    m_PreMarkerCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }

  switch (m_MarkerMode)
  {
    case MarkerMode::InsertLabel:
    {
      if (!m_sMarkerName.IsEmpty())
      {
        executionContext.m_pCommandList->InsertDebugLabel(m_sMarkerName.GetView(), m_MarkerColor);
      }
      break;
    }

    case MarkerMode::BeginGroup:
    {
      if (!m_sMarkerName.IsEmpty())
      {
        executionContext.m_pCommandList->BeginDebugGroup(m_sMarkerName.GetView(), m_MarkerColor);
      }
      break;
    }

    case MarkerMode::EndGroup:
    {
      executionContext.m_pCommandList->EndDebugGroup();
      break;
    }

    default:
      break;
  }

  if (m_PostMarkerCommandListFunc.IsValid())
  {
    m_PostMarkerCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_GpuMarkerPass);
