#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/Renderer.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiRenderPipelinePassFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassFlags::None),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassFlags::StereoAware),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassFlags::AllowSubpassFuse),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassFlags::AsyncCompute),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassFlags::AsyncTransfer),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassFlags::DynamicResolution),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassFlags::DebugPass),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderPipelinePassConcurrencyHint, 1)
 XII_ENUM_CONSTANT(xiiRenderPipelinePassConcurrencyHint::Sequential),
 XII_ENUM_CONSTANT(xiiRenderPipelinePassConcurrencyHint::ParallelIndependent),
 XII_ENUM_CONSTANT(xiiRenderPipelinePassConcurrencyHint::ParallelWithSync),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelinePass, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("Name", GetName, SetName),
    XII_BITFLAGS_ACCESSOR_PROPERTY("Flags", xiiRenderPipelinePassFlags, GetPassFlags, SetPassFlags),
    XII_ENUM_ACCESSOR_PROPERTY("ConcurrencyHint", xiiRenderPipelinePassConcurrencyHint, GetPassConcurrencyHint, SetPassConcurrencyHint),
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

xiiRenderPipelinePass::xiiRenderPipelinePass(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassFlags> flags, xiiEnum<xiiRenderPipelinePassConcurrencyHint> concurrencyHint) :
  m_PassFlags(flags), m_PassConcurrencyHint(concurrencyHint)
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

void xiiRenderPipelinePass::SetPassFlags(xiiBitflags<xiiRenderPipelinePassFlags> flags)
{
  if (m_PassFlags == flags)
    return;

  m_PassFlags = flags;
}

void xiiRenderPipelinePass::SetPassConcurrencyHint(xiiEnum<xiiRenderPipelinePassConcurrencyHint> concurrencyHint)
{
  if (m_PassConcurrencyHint == concurrencyHint)
    return;

  m_PassConcurrencyHint = concurrencyHint;
}

void xiiRenderPipelinePass::InitializeRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) {}

void xiiRenderPipelinePass::ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) {}

void xiiRenderPipelinePass::ReadBackProperties(xiiView* pView) {}

xiiResult xiiRenderPipelinePass::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_bActive;
  inout_stream << m_sName;
  inout_stream << m_PassFlags;
  inout_stream << m_PassConcurrencyHint;

  return XII_SUCCESS;
}

xiiResult xiiRenderPipelinePass::Deserialize(xiiStreamReader& inout_stream)
{
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");

  inout_stream >> m_bActive;
  inout_stream >> m_sName;
  inout_stream >> m_PassFlags;
  inout_stream >> m_PassConcurrencyHint;

  return XII_SUCCESS;
}

void xiiRenderPipelinePass::RenderDataWithCategory(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList, xiiRenderData::Category category, xiiRenderDataBatch::Filter filter)
{
  xiiGALScopedDebugGroup renderGroup(pCommandList, xiiRenderData::GetCategoryName(category));

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
        pRenderer->RenderBatch(renderViewContext, pCommandList, this, batch);
      }
    }
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelinePass);
