#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGraphicsDeviceType, 1)
  XII_ENUM_CONSTANT(xiiGraphicsDeviceType::Undefined),
  XII_ENUM_CONSTANT(xiiGraphicsDeviceType::D3D11),
  XII_ENUM_CONSTANT(xiiGraphicsDeviceType::D3D12),
  XII_ENUM_CONSTANT(xiiGraphicsDeviceType::Vulkan),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on
