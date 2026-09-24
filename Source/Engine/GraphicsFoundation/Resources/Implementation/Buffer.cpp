/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALBufferMode, 1)
  XII_ENUM_CONSTANT(xiiGALBufferMode::Undefined),
  XII_ENUM_CONSTANT(xiiGALBufferMode::Formatted),
  XII_ENUM_CONSTANT(xiiGALBufferMode::Structured),
  XII_ENUM_CONSTANT(xiiGALBufferMode::Raw),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALMiscBufferFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALMiscBufferFlags::SparseAlias),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBuffer, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

#define XII_GAL_BUFFER_CHECK(expression, ...)  \
  do                                           \
  {                                            \
    XII_ASSERT_DEV((expression), __VA_ARGS__); \
    if (!(expression)) { return {}; }          \
  } while (false)

xiiGALBuffer::xiiGALBuffer(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALResource(std::move(pDevice)), m_Description(creationDescription)
{
}

xiiGALBuffer::~xiiGALBuffer() = default;

xiiSharedPtr<xiiGALBufferView> xiiGALBuffer::GetDefaultView(xiiEnum<xiiGALBufferViewType> viewType)
{
  XII_ASSERT_DEV(viewType > xiiGALBufferViewType::Undefined && viewType < xiiGALBufferViewType::ENUM_COUNT, "Invalid view type.");

  XII_ASSERT_DEV(m_DefaultBufferViews[viewType.GetValue()] != nullptr, "Buffer view handle is invalid!");

  return m_DefaultBufferViews[viewType.GetValue()];
}

xiiSharedPtr<xiiGALBufferView> xiiGALBuffer::CreateView(xiiGALBufferViewCreationDescription& description)
{
  if (description.m_uiByteWidth == 0U)
  {
    XII_GAL_BUFFER_CHECK(m_Description.m_uiSize > description.m_uiByteOffset, "The byte offset ({0}) exceeds the buffer size ({1}).", description.m_uiByteOffset, m_Description.m_uiSize);

    description.m_uiByteWidth = m_Description.m_uiSize - description.m_uiByteOffset;
  }

  XII_GAL_BUFFER_CHECK((description.m_uiByteOffset + description.m_uiByteWidth) <= m_Description.m_uiSize, "The buffer view range [{0}, {1}) is out of the buffer boundaries [0, {2}).", description.m_uiByteOffset, (description.m_uiByteOffset + description.m_uiByteWidth), m_Description.m_uiSize);

  if (m_Description.m_BindFlags.IsAnySet(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess))
  {
    if (m_Description.m_Mode == xiiGALBufferMode::Structured || m_Description.m_Mode == xiiGALBufferMode::Formatted)
    {
      XII_GAL_BUFFER_CHECK(m_Description.m_uiElementByteStride != 0U, "The element byte stride is zero.");
      XII_GAL_BUFFER_CHECK((description.m_uiByteOffset % m_Description.m_uiElementByteStride) == 0U, "The buffer view byte offset ({0}) is not a multiple of the element byte stride ({1}).", description.m_uiByteOffset, m_Description.m_uiElementByteStride);
      XII_GAL_BUFFER_CHECK((description.m_uiByteWidth % m_Description.m_uiElementByteStride) == 0U, "The buffer view byte width ({0}) is not a multiple of the element byte stride ({1}).", description.m_uiByteWidth, m_Description.m_uiElementByteStride);
    }

    XII_GAL_BUFFER_CHECK(!(m_Description.m_Mode == xiiGALBufferMode::Formatted && description.m_Format == xiiGALResourceFormat::Unknown), "The format must be specified when creating a view of a formatted buffer.");

    if (m_Description.m_Mode == xiiGALBufferMode::Formatted || (m_Description.m_Mode == xiiGALBufferMode::Raw && description.m_Format != xiiGALResourceFormat::Unknown))
    {
      XII_GAL_BUFFER_CHECK(m_Description.m_Mode != xiiGALBufferMode::Raw && m_Description.m_uiElementByteStride != 0U, "To enable formatted views of a raw buffer, the element byte stride must be specified in the buffer creation description.");

      const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(description.m_Format);

      XII_GAL_BUFFER_CHECK(m_Description.m_uiElementByteStride == formatProperties.GetElementSize(), "The buffer element byte stride ({0}) is not consistent with the size ({1}) defined by the format ({2}) of the view ({2}).", m_Description.m_uiElementByteStride, formatProperties.GetElementSize(), description.m_Format);
    }

    if (m_Description.m_Mode == xiiGALBufferMode::Raw && description.m_Format == xiiGALResourceFormat::Unknown)
    {
      XII_GAL_BUFFER_CHECK((description.m_uiByteOffset % 16U) == 0U, "When creating a Raw buffer view, the offset of the first element from the start of the buffer ({0}) must be a multiple of 16 bytes.", description.m_uiByteOffset);
    }

    if (m_Description.m_Mode == xiiGALBufferMode::Structured)
    {
      const xiiUInt32 uiStructuredBufferOffsetAlignment = m_pDevice->GetGraphicsDeviceAdapterProperties().m_BufferProperties.m_uiStructuredBufferOffsetAlignment;

      XII_GAL_BUFFER_CHECK(uiStructuredBufferOffsetAlignment != 0, "Device structured buffer offset alignment may not have been initialized.");
      XII_GAL_BUFFER_CHECK((description.m_uiByteOffset % uiStructuredBufferOffsetAlignment) == 0U, "Structured buffer view byte offset ({0}) is not a multiple of the required structured buffer offset alignment ({1}).", description.m_uiByteOffset, uiStructuredBufferOffsetAlignment);
    }
  }

  return CreateViewPlatform(description);
}

void xiiGALBuffer::CreateDefaultResourceViews()
{
  // Cannot create default views for formatted buffers, since the view format is unknown at creation time.
  if (m_Description.m_Mode == xiiGALBufferMode::Formatted)
    return;

  xiiGALBufferViewCreationDescription viewDescription;
  viewDescription.m_ViewType     = xiiGALBufferViewType::ShaderResource;
  viewDescription.m_uiByteOffset = 0;
  viewDescription.m_uiByteWidth  = 0;

  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
  {
    m_DefaultBufferViews[xiiGALBufferViewType::ShaderResource] = CreateView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess))
  {
    viewDescription.m_ViewType = xiiGALBufferViewType::UnorderedAccess;
    m_DefaultBufferViews[xiiGALBufferViewType::UnorderedAccess] = CreateView(viewDescription);
  }
}

void xiiGALBuffer::VerifyFlushMappedRangeArguments(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(GetMemoryProperties().IsSet(xiiGALMemoryPropertyFlags::HostCoherent), "Coherent memory does not need to be flushed.");
  XII_ASSERT_DEV(m_Description.m_Usage != xiiGALResourceUsage::Dynamic, "Dynamic buffer mapped memory must never be flushed.");
  XII_ASSERT_DEV((uiStartOffset + uiSize) <= m_Description.m_uiSize, "Memory range is out of buffer bounds.");
#else
  XII_IGNORE_UNUSED(uiStartOffset);
  XII_IGNORE_UNUSED(uiSize);
#endif
}

void xiiGALBuffer::VerifyInvalidateMappedRangeArguments(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(GetMemoryProperties().IsSet(xiiGALMemoryPropertyFlags::HostCoherent), "Coherent memory does not need to be invalidated.");
  XII_ASSERT_DEV(m_Description.m_Usage != xiiGALResourceUsage::Dynamic, "Dynamic buffer mapped memory must never be invalidated.");
  XII_ASSERT_DEV((uiStartOffset + uiSize) <= m_Description.m_uiSize, "Memory range is out of buffer bounds.");
#else
  XII_IGNORE_UNUSED(uiStartOffset);
  XII_IGNORE_UNUSED(uiSize);
#endif
}
#undef XII_GAL_BUFFER_CHECK

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Buffer);
