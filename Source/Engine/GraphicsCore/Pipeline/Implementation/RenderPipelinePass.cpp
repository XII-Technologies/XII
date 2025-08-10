#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/Renderer.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiRenderPipelinePassCapabilityFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassCapabilityFlags::StereoAware),
  XII_BITFLAGS_CONSTANT(xiiRenderPipelinePassCapabilityFlags::AllowSubpassFuse),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiRenderPipelinePassFlags, 1)
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
  XII_IGNORE_UNUSED(view);
  XII_IGNORE_UNUSED(pInputs);
  XII_IGNORE_UNUSED(pOutputs);

  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALDeviceObject> xiiRenderPipelinePassBase::QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request)
{
  XII_IGNORE_UNUSED(pPin);
  XII_IGNORE_UNUSED(request);

  return xiiSharedPtr<xiiGALDeviceObject>();
}

void xiiRenderPipelinePassBase::ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(renderViewContext);
  XII_IGNORE_UNUSED(pInputs);
  XII_IGNORE_UNUSED(pOutputs);
}

void xiiRenderPipelinePassBase::ReadBackProperties(xiiView* pView)
{
  XII_IGNORE_UNUSED(pView);
}

///////////////////////////////////////////////////////////////////////////////

xiiGraphicsPipelinePass::xiiGraphicsPipelinePass(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassCapabilityFlags> capabilityFlags) :
  xiiRenderPipelinePassBase(sName, capabilityFlags)
{
}

xiiGraphicsPipelinePass::~xiiGraphicsPipelinePass() = default;

void xiiGraphicsPipelinePass::RenderDataWithCategory(const xiiRenderViewContext& renderViewContext, xiiRenderData::Category category, xiiRenderDataBatch::Filter filter)
{
  xiiGALScopedDebugGroup renderGroup(renderViewContext.m_pRenderContext->GetCommandList(), xiiRenderData::GetCategoryName(category));

  xiiRenderDataBatchList batchList    = GetPipeline()->GetRenderDataBatchesWithCategory(category, filter);
  const xiiUInt32        uiBatchCount = batchList.GetBatchCount();

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

///////////////////////////////////////////////////////////////////////////////

xiiRenderPipelineResourceRequest::xiiRenderPipelineResourceRequest() = default;

xiiRenderPipelineResourceRequest::xiiRenderPipelineResourceRequest(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALBufferCreationDescription& description) :
  m_Type(resourceType), m_Buffer(description)
{
  XII_ASSERT_DEV(IsBuffer(), "Invalid resource type for a buffer resource.");
}

xiiRenderPipelineResourceRequest::xiiRenderPipelineResourceRequest(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALTextureCreationDescription& description) :
  m_Type(resourceType), m_Texture(description)
{
  XII_ASSERT_DEV(IsTexture(), "Invalid resource type for a texture resource.");
}

xiiRenderPipelineResourceRequest::xiiRenderPipelineResourceRequest(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALSamplerCreationDescription& description) :
  m_Type(resourceType), m_Sampler(description)
{
  XII_ASSERT_DEV(IsSampler(), "Invalid resource type for a sampler resource.");
}

xiiRenderPipelineResourceRequest::xiiRenderPipelineResourceRequest(const xiiRenderPipelinePassResource& passResource) :
  m_Type(passResource.m_Type)
{
  if (IsBuffer())
  {
    m_Buffer = passResource.m_Buffer.m_Description;
  }
  else if (IsTexture())
  {
    m_Texture = passResource.m_Texture.m_Description;
  }
  else if (IsSampler())
  {
    m_Sampler = passResource.m_Sampler.m_Description;
  }
}

///////////////////////////////////////////////////////////////////////////////

xiiRenderPipelinePassResource::xiiRenderPipelinePassResource() :
  m_Type(xiiRenderPipelineNodePinResourceType::Unknown)
{
  // It's undefined behavior to leave a union uninitialized with non-trivial members.
  // So we initialize the texture variant by default, even if it's unused.
  new (&m_Texture) decltype(m_Texture)();
}

xiiRenderPipelinePassResource::xiiRenderPipelinePassResource(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALBufferCreationDescription& description, const xiiSharedPtr<xiiGALBuffer>& pBuffer) :
  m_Type(resourceType)
{
  XII_ASSERT_DEV(IsBuffer(), "Invalid resource type for a buffer resource.");

  new (&m_Buffer) decltype(m_Buffer){description, pBuffer};
}

xiiRenderPipelinePassResource::xiiRenderPipelinePassResource(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALTextureCreationDescription& description, const xiiSharedPtr<xiiGALTexture>& pTexture) :
  m_Type(resourceType)
{
  XII_ASSERT_DEV(IsTexture(), "Invalid resource type for a texture resource.");

  new (&m_Texture) decltype(m_Texture){description, pTexture};
}

xiiRenderPipelinePassResource::xiiRenderPipelinePassResource(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALSamplerCreationDescription& description, const xiiSharedPtr<xiiGALSampler>& pSampler) :
  m_Type(resourceType)
{
  XII_ASSERT_DEV(IsSampler(), "Invalid resource type for a sampler resource.");

  new (&m_Sampler) decltype(m_Sampler){description, pSampler};
}

xiiRenderPipelinePassResource::xiiRenderPipelinePassResource(const xiiRenderPipelinePassResource& other) :
  m_Type(other.m_Type)
{
  if (IsBuffer())
  {
    new (&m_Buffer) decltype(m_Buffer)(other.m_Buffer);
  }
  else if (IsTexture())
  {
    new (&m_Texture) decltype(m_Texture)(other.m_Texture);
  }
  else if (IsSampler())
  {
    new (&m_Sampler) decltype(m_Sampler)(other.m_Sampler);
  }
}

xiiRenderPipelinePassResource::~xiiRenderPipelinePassResource()
{
  if (IsBuffer())
  {
    m_Buffer.m_pBuffer = nullptr;
  }
  else if (IsTexture())
  {
    m_Texture.m_pTexture = nullptr;
  }
  else if (IsSampler())
  {
    m_Sampler.m_pSampler = nullptr;
  }
}

xiiRenderPipelinePassResource& xiiRenderPipelinePassResource::operator=(const xiiRenderPipelinePassResource& other)
{
  if (this != &other)
  {
    // Clean up current resource.
    this->~xiiRenderPipelinePassResource();

    // Copy construct into this object.
    new (this) xiiRenderPipelinePassResource(other);
  }
  return *this;
}

xiiUInt32 xiiRenderPipelinePassResource::CalculateDescriptorHash() const
{
  if (IsBuffer())
  {
    return m_Buffer.m_Description.CalculateHash();
  }
  else if (IsTexture())
  {
    return m_Texture.m_Description.CalculateHash();
  }
  else if (IsSampler())
  {
    return m_Sampler.m_Description.CalculateHash();
  }
  return 0U;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelinePass);
