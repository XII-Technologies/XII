#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/Renderer.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiRenderPipelinePassCapabilityFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassCapabilityFlags::None),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassCapabilityFlags::StereoAware),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassCapabilityFlags::AllowSubpassFuse),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiRenderPipelinePassFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassFlags::None),
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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelinePassBase, 1, xiiRTTINoAllocator)
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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGraphicsPipelinePass, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Graphics")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiComputePipelinePass, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Compute")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCopyPipelinePass, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Copy")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPresentPipelinePass, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Present")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiUtilityPipelinePass, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Utility")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRenderPipelinePassBase::xiiRenderPipelinePassBase(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassCapabilityFlags> capabilityFlags) :
  m_CapabilityFlags(capabilityFlags)
{
  if (!sName.IsEmpty())
  {
    m_sName.Assign(sName);
  }
}

xiiRenderPipelinePassBase::~xiiRenderPipelinePassBase() = default;

void xiiRenderPipelinePassBase::SetName(xiiStringView sName)
{
  if (!sName.IsEmpty())
  {
    m_sName.Assign(sName);
  }
}

void xiiRenderPipelinePassBase::SetPassFlags(xiiBitflags<xiiRenderPipelinePassFlags> flags)
{
  if (m_PassFlags == flags)
    return;

  m_PassFlags = flags;
}

void xiiRenderPipelinePassBase::SetPassConcurrencyHint(xiiEnum<xiiRenderPipelinePassConcurrencyHint> concurrencyHint)
{
  if (m_PassConcurrencyHint == concurrencyHint)
    return;

  m_PassConcurrencyHint = concurrencyHint;
}

xiiResult xiiRenderPipelinePassBase::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_bActive;
  inout_stream << m_sName;
  inout_stream << m_PassFlags;
  inout_stream << m_PassConcurrencyHint;

  return XII_SUCCESS;
}

xiiResult xiiRenderPipelinePassBase::Deserialize(xiiStreamReader& inout_stream)
{
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_ASSERT_DEBUG(uiVersion == 1, "Unknown render pipeline pass version!");

  inout_stream >> m_bActive;
  inout_stream >> m_sName;
  inout_stream >> m_PassFlags;
  inout_stream >> m_PassConcurrencyHint;

  return XII_SUCCESS;
}

xiiResult xiiRenderPipelinePassBase::InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALDeviceObject> xiiRenderPipelinePassBase::QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request)
{
  return xiiSharedPtr<xiiGALDeviceObject>();
}

void xiiRenderPipelinePassBase::ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
}

void xiiRenderPipelinePassBase::ReadBackProperties(xiiView* pView)
{
}

///////////////////////////////////////////////////////////////////////////////

xiiGraphicsPipelinePass::xiiGraphicsPipelinePass(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassCapabilityFlags> capabilityFlags):
  xiiRenderPipelinePassBase(sName, capabilityFlags)
{
}

xiiGraphicsPipelinePass::~xiiGraphicsPipelinePass() = default;

void xiiGraphicsPipelinePass::RenderDataWithCategory(const xiiRenderViewContext& renderViewContext, xiiRenderData::Category category, xiiRenderDataBatch::Filter filter)
{
  xiiGALScopedDebugGroup renderGroup(renderViewContext.m_pCommandList, xiiRenderData::GetCategoryName(category));

  auto            batchList    = GetPipeline()->GetRenderDataBatchesWithCategory(category, filter);
  const xiiUInt32 uiBatchCount = batchList.GetBatchCount();

  for (xiiUInt32 i = 0U; i < uiBatchCount; ++i)
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

///////////////////////////////////////////////////////////////////////////////

xiiComputePipelinePass::xiiComputePipelinePass(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassCapabilityFlags> capabilityFlags) :
  xiiRenderPipelinePassBase(sName, capabilityFlags)
{
}

xiiComputePipelinePass::~xiiComputePipelinePass() = default;

///////////////////////////////////////////////////////////////////////////////

xiiCopyPipelinePass::xiiCopyPipelinePass(xiiStringView sName) :
  xiiRenderPipelinePassBase(sName, xiiRenderPipelinePassCapabilityFlags::None)
{
}

xiiCopyPipelinePass::~xiiCopyPipelinePass() = default;

///////////////////////////////////////////////////////////////////////////////

xiiPresentPipelinePass::xiiPresentPipelinePass(xiiStringView sName) :
  xiiRenderPipelinePassBase(sName, xiiRenderPipelinePassCapabilityFlags::None)
{
}

xiiPresentPipelinePass::~xiiPresentPipelinePass() = default;

///////////////////////////////////////////////////////////////////////////////

xiiUtilityPipelinePass::xiiUtilityPipelinePass(xiiStringView sName) :
  xiiRenderPipelinePassBase(sName, xiiRenderPipelinePassCapabilityFlags::None)
{
}

xiiUtilityPipelinePass::~xiiUtilityPipelinePass() = default;

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelinePass);
