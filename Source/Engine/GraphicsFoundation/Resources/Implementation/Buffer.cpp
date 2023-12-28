#include <GraphicsFoundation/GraphicsFoundationPCH.h>

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

// clang-format on

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBuffer, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALBuffer::xiiGALBuffer(const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALResource(), m_Description(creationDescription)
{
}

xiiGALBuffer::~xiiGALBuffer() = default;

void xiiGALBuffer::CreateDefaultResourceViews()
{
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Buffer);
