#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Profiling/Profiling.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelinePass, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("Name", GetName, SetName),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Grape))
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRenderPipelinePass::xiiRenderPipelinePass(const char* szName, bool bIsStereoAware) :
  m_bIsStereoAware(bIsStereoAware)
{
  m_sName.Assign(szName);
}

xiiRenderPipelinePass::~xiiRenderPipelinePass() = default;

void xiiRenderPipelinePass::SetName(const char* szName)
{
  if (!xiiStringUtils::IsNullOrEmpty(szName))
  {
    m_sName.Assign(szName);
  }
}

const char* xiiRenderPipelinePass::GetName() const
{
  return m_sName.GetData();
}

void xiiRenderPipelinePass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) {}

void xiiRenderPipelinePass::ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) {}

void xiiRenderPipelinePass::ReadBackProperties(xiiView* pView) {}

void xiiRenderPipelinePass::RenderDataWithCategory(const xiiRenderViewContext& renderViewContext, xiiRenderData::Category category, xiiRenderDataBatch::Filter filter)
{
  XII_PROFILE_AND_MARKER(renderViewContext.m_pRenderContext->GetCommandEncoder(), xiiRenderData::GetCategoryName(category));

  auto            batchList    = m_pPipeline->GetRenderDataBatchesWithCategory(category, filter);
  const xiiUInt32 uiBatchCount = batchList.GetBatchCount();
  for (xiiUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const xiiRenderDataBatch& batch = batchList.GetBatch(i);

    if (const xiiRenderData* pRenderData = batch.GetFirstData<xiiRenderData>())
    {
      const xiiRTTI* pType = pRenderData->GetDynamicRTTI();

      if (const xiiRenderer* pRenderer = xiiRenderData::GetCategoryRenderer(category, pType))
      {
        pRenderer->RenderBatch(renderViewContext, this, batch);
      }
    }
  }
}



XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelinePass);
