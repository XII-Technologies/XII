#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Buffer.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALBuffer, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALBufferMode, 1)
  XII_ENUM_CONSTANT(xiiGALBufferMode::Undefined),
  XII_ENUM_CONSTANT(xiiGALBufferMode::Formatted),
  XII_ENUM_CONSTANT(xiiGALBufferMode::Structured),
  XII_ENUM_CONSTANT(xiiGALBufferMode::Raw),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALMiscBufferFlags, 1)
  XII_ENUM_CONSTANT(xiiGALMiscBufferFlags::None),
  XII_ENUM_CONSTANT(xiiGALMiscBufferFlags::SparseAlias),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on

xiiGALBuffer::xiiGALBuffer(const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALResource<xiiGALBufferCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALBuffer::~xiiGALBuffer() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Buffer);
