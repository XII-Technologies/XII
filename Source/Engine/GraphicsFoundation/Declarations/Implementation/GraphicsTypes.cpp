#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGraphicsDeviceType, 1)
  XII_ENUM_CONSTANT(xiiGraphicsDeviceType::Undefined),
  XII_ENUM_CONSTANT(xiiGraphicsDeviceType::D3D11),
  XII_ENUM_CONSTANT(xiiGraphicsDeviceType::D3D12),
  XII_ENUM_CONSTANT(xiiGraphicsDeviceType::Vulkan),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALValueType, 1)
  XII_ENUM_CONSTANT(xiiGALValueType::Undefined),
  XII_ENUM_CONSTANT(xiiGALValueType::Int8),
  XII_ENUM_CONSTANT(xiiGALValueType::Int16),
  XII_ENUM_CONSTANT(xiiGALValueType::Int32),
  XII_ENUM_CONSTANT(xiiGALValueType::UInt8),
  XII_ENUM_CONSTANT(xiiGALValueType::UInt16),
  XII_ENUM_CONSTANT(xiiGALValueType::UInt32),
  XII_ENUM_CONSTANT(xiiGALValueType::Float16),
  XII_ENUM_CONSTANT(xiiGALValueType::Float32),
  XII_ENUM_CONSTANT(xiiGALValueType::Float64),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShaderStage, 1)
  XII_ENUM_CONSTANT(xiiGALShaderStage::Unknown),
  XII_ENUM_CONSTANT(xiiGALShaderStage::Vertex),
  XII_ENUM_CONSTANT(xiiGALShaderStage::Pixel),
  XII_ENUM_CONSTANT(xiiGALShaderStage::Geometry),
  XII_ENUM_CONSTANT(xiiGALShaderStage::Hull),
  XII_ENUM_CONSTANT(xiiGALShaderStage::Domain),
  XII_ENUM_CONSTANT(xiiGALShaderStage::Compute),
  XII_ENUM_CONSTANT(xiiGALShaderStage::Amplification),
  XII_ENUM_CONSTANT(xiiGALShaderStage::Mesh),
  XII_ENUM_CONSTANT(xiiGALShaderStage::RayGeneration),
  XII_ENUM_CONSTANT(xiiGALShaderStage::RayMiss),
  XII_ENUM_CONSTANT(xiiGALShaderStage::RayClosestHit),
  XII_ENUM_CONSTANT(xiiGALShaderStage::RayAnyHit),
  XII_ENUM_CONSTANT(xiiGALShaderStage::RayIntersection),
  XII_ENUM_CONSTANT(xiiGALShaderStage::Callable),
  XII_ENUM_CONSTANT(xiiGALShaderStage::Tile),
  XII_ENUM_CONSTANT(xiiGALShaderStage::AllGraphics),
  XII_ENUM_CONSTANT(xiiGALShaderStage::AllMesh),
  XII_ENUM_CONSTANT(xiiGALShaderStage::AllRayTracing),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALBindFlags, 1)
  XII_ENUM_CONSTANT(xiiGALBindFlags::None),
  XII_ENUM_CONSTANT(xiiGALBindFlags::VertexBuffer),
  XII_ENUM_CONSTANT(xiiGALBindFlags::IndexBuffer),
  XII_ENUM_CONSTANT(xiiGALBindFlags::UniformBuffer),
  XII_ENUM_CONSTANT(xiiGALBindFlags::ShaderResource),
  XII_ENUM_CONSTANT(xiiGALBindFlags::StreamOutput),
  XII_ENUM_CONSTANT(xiiGALBindFlags::RenderTarget),
  XII_ENUM_CONSTANT(xiiGALBindFlags::DepthStencil),
  XII_ENUM_CONSTANT(xiiGALBindFlags::UnorderedAccess),
  XII_ENUM_CONSTANT(xiiGALBindFlags::IndirectDrawArguments),
  XII_ENUM_CONSTANT(xiiGALBindFlags::InputAttachment),
  XII_ENUM_CONSTANT(xiiGALBindFlags::RayTracing),
  XII_ENUM_CONSTANT(xiiGALBindFlags::ShadingRate),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALResourceUsage, 1)
  XII_ENUM_CONSTANT(xiiGALResourceUsage::Immutable),
  XII_ENUM_CONSTANT(xiiGALResourceUsage::Default),
  XII_ENUM_CONSTANT(xiiGALResourceUsage::Dynamic),
  XII_ENUM_CONSTANT(xiiGALResourceUsage::Staging),
  XII_ENUM_CONSTANT(xiiGALResourceUsage::Unified),
  XII_ENUM_CONSTANT(xiiGALResourceUsage::Sparse),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on
