#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/FrameDataProvider.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFrameDataProviderBase, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiFrameDataProviderBase::xiiFrameDataProviderBase() = default;

void* xiiFrameDataProviderBase::GetData(const xiiRenderViewContext& renderViewContext)
{
  if (m_pData == nullptr || m_uiLastUpdateFrame != xiiRenderWorld::GetFrameCounter())
  {
    m_pData = UpdateData(renderViewContext, m_pOwnerPipeline->GetRenderData());

    m_uiLastUpdateFrame = xiiRenderWorld::GetFrameCounter();
  }

  return m_pData;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_FrameDataProvider);
