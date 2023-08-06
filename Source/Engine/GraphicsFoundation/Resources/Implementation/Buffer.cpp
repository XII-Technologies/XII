#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Buffer.h>

// clang-format off

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
