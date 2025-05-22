#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/Renderer.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

#include <GraphicsFoundation/Profiling/Profiling.h>

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

xiiRenderPipelinePass::xiiRenderPipelinePass(xiiStringView sName, bool bIsStereoAware) :
  m_bIsStereoAware(bIsStereoAware)
{
  if (!sName.IsEmpty())
  {
    m_sName.Assign(sName);
  }
}

xiiRenderPipelinePass::~xiiRenderPipelinePass() = default;

void xiiRenderPipelinePass::SetName(xiiStringView sName)
{
  if (!sName.IsEmpty())
  {
    m_sName.Assign(sName);
  }
}

xiiStringView xiiRenderPipelinePass::GetName() const
{
  return m_sName.GetView();
}

void xiiRenderPipelinePass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) {}

void xiiRenderPipelinePass::ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) {}

void xiiRenderPipelinePass::ReadBackProperties(xiiView* pView) {}

xiiResult xiiRenderPipelinePass::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_bActive;
  inout_stream << m_sName;

  return XII_SUCCESS;
}

xiiResult xiiRenderPipelinePass::Deserialize(xiiStreamReader& inout_stream)
{
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");

  inout_stream >> m_bActive;
  inout_stream >> m_sName;

  return XII_SUCCESS;
}

void xiiRenderPipelinePass::RenderDataWithCategory(const xiiRenderViewContext& renderViewContext, xiiRenderData::Category category, xiiRenderDataBatch::Filter filter)
{
  XII_PROFILE_AND_MARKER(renderViewContext.m_pRenderContext->GetCommandList(), xiiRenderData::GetCategoryName(category));

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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelinePass);
