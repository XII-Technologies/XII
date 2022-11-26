#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFrameDataProviderBase, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiFrameDataProviderBase::xiiFrameDataProviderBase() :
  m_pOwnerPipeline(nullptr), m_pData(nullptr), m_uiLastUpdateFrame(0)
{
}

void* xiiFrameDataProviderBase::GetData(const xiiRenderViewContext& renderViewContext)
{
  if (m_pData == nullptr || m_uiLastUpdateFrame != xiiRenderWorld::GetFrameCounter())
  {
    m_pData = UpdateData(renderViewContext, m_pOwnerPipeline->GetRenderData());

    m_uiLastUpdateFrame = xiiRenderWorld::GetFrameCounter();
  }

  return m_pData;
}



XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_FrameDataProvider);
