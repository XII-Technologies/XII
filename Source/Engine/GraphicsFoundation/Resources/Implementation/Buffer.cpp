#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALBufferMode, 1)
  XII_ENUM_CONSTANT(xiiGALBufferMode::Undefined),
  XII_ENUM_CONSTANT(xiiGALBufferMode::Formatted),
  XII_ENUM_CONSTANT(xiiGALBufferMode::Structured),
  XII_ENUM_CONSTANT(xiiGALBufferMode::Raw),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALMiscBufferFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALMiscBufferFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALMiscBufferFlags::SparseAlias),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBuffer, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

xiiGALBuffer::xiiGALBuffer(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALResource(pDevice), m_Description(creationDescription)
{
}

xiiGALBuffer::~xiiGALBuffer() = default;

xiiSharedPtr<xiiGALBufferView> xiiGALBuffer::GetDefaultView(xiiEnum<xiiGALBufferViewType> viewType)
{
  XII_ASSERT_DEV(viewType > xiiGALBufferViewType::Undefined && viewType < xiiGALBufferViewType::ENUM_COUNT, "Invalid view type.");

  XII_ASSERT_DEV(m_DefaultBufferViews[viewType.GetValue()] != nullptr, "Buffer view handle is invalid!");

  return m_DefaultBufferViews[viewType.GetValue()];
}

void xiiGALBuffer::CreateDefaultResourceViews()
{
  // Cannot create default views for formatted buffers, since the view format is unknown at creation time.
  if (m_Description.m_Mode == xiiGALBufferMode::Formatted)
    return;

  xiiGALBufferViewCreationDescription viewDescription;
  viewDescription.m_pBuffer      = this;
  viewDescription.m_ViewType     = xiiGALBufferViewType::ShaderResource;
  viewDescription.m_uiByteOffset = 0;
  viewDescription.m_uiByteWidth  = 0;

  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
  {
    m_DefaultBufferViews[xiiGALBufferViewType::ShaderResource] = m_pDevice->CreateBufferView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess))
  {
    m_DefaultBufferViews[xiiGALBufferViewType::UnorderedAccess] = m_pDevice->CreateBufferView(viewDescription);
  }
}

void xiiGALBuffer::VerifyFlushMappedRangeArguments(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(GetMemoryProperties().IsSet(xiiGALMemoryPropertyFlags::HostCoherent), "Coherent memory does not need to be flushed.");
  XII_ASSERT_DEV(m_Description.m_ResourceUsage != xiiGALResourceUsage::Dynamic, "Dynamic buffer mapped memory must never be flushed.");
  XII_ASSERT_DEV((uiStartOffset + uiSize) <= m_Description.m_uiSize, "Memory range is out of buffer bounds.");
#  else
  XII_IGNORE_UNUSED(uiStartOffset);
  XII_IGNORE_UNUSED(uiSize);
#endif
}

void xiiGALBuffer::VerifyInvalidateMappedRangeArguments(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(GetMemoryProperties().IsSet(xiiGALMemoryPropertyFlags::HostCoherent), "Coherent memory does not need to be invalidated.");
  XII_ASSERT_DEV(m_Description.m_ResourceUsage != xiiGALResourceUsage::Dynamic, "Dynamic buffer mapped memory must never be invalidated.");
  XII_ASSERT_DEV((uiStartOffset + uiSize) <= m_Description.m_uiSize, "Memory range is out of buffer bounds.");
#else
  XII_IGNORE_UNUSED(uiStartOffset);
  XII_IGNORE_UNUSED(uiSize);
#endif
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Buffer);
