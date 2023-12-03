#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALGraphicsDeviceType, 1)
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceType::Undefined),
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceType::OpenGLES),
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceType::OpenGL),
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceType::Direct3D11),
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceType::Direct3D12),
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceType::Vulkan),
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceType::Metal),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALDeviceFeatureState, 1)
  XII_ENUM_CONSTANT(xiiGALDeviceFeatureState::Disabled),
  XII_ENUM_CONSTANT(xiiGALDeviceFeatureState::Enabled),
  XII_ENUM_CONSTANT(xiiGALDeviceFeatureState::Optional),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALGraphicsAdapterVendor, 1)
  XII_ENUM_CONSTANT(xiiGALGraphicsAdapterVendor::Unknown),
  XII_ENUM_CONSTANT(xiiGALGraphicsAdapterVendor::Nvidia),
  XII_ENUM_CONSTANT(xiiGALGraphicsAdapterVendor::AMD),
  XII_ENUM_CONSTANT(xiiGALGraphicsAdapterVendor::Intel),
  XII_ENUM_CONSTANT(xiiGALGraphicsAdapterVendor::ARM),
  XII_ENUM_CONSTANT(xiiGALGraphicsAdapterVendor::Qualcomm),
  XII_ENUM_CONSTANT(xiiGALGraphicsAdapterVendor::ImaginationTechnologies),
  XII_ENUM_CONSTANT(xiiGALGraphicsAdapterVendor::Microsoft),
  XII_ENUM_CONSTANT(xiiGALGraphicsAdapterVendor::Apple),
  XII_ENUM_CONSTANT(xiiGALGraphicsAdapterVendor::Mesa),
  XII_ENUM_CONSTANT(xiiGALGraphicsAdapterVendor::Broadcom),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALDeviceValidationLevel, 1)
  XII_ENUM_CONSTANT(xiiGALDeviceValidationLevel::Disabled),
  XII_ENUM_CONSTANT(xiiGALDeviceValidationLevel::Standard),
  XII_ENUM_CONSTANT(xiiGALDeviceValidationLevel::All),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALDeviceEventType, 1)
  XII_ENUM_CONSTANT(xiiGALDeviceEventType::Unknown),
  XII_ENUM_CONSTANT(xiiGALDeviceEventType::AfterInitialization),
  XII_ENUM_CONSTANT(xiiGALDeviceEventType::BeforeShutdown),
  XII_ENUM_CONSTANT(xiiGALDeviceEventType::BeforeBeginFrame),
  XII_ENUM_CONSTANT(xiiGALDeviceEventType::AfterBeginFrame),
  XII_ENUM_CONSTANT(xiiGALDeviceEventType::BeforeEndFrame),
  XII_ENUM_CONSTANT(xiiGALDeviceEventType::AfterEndFrame),
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

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALShaderStage, 1)
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::Unknown),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::Vertex),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::Pixel),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::Geometry),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::Hull),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::Domain),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::Compute),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::Amplification),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::Mesh),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::RayGeneration),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::RayMiss),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::RayClosestHit),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::RayAnyHit),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::RayIntersection),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::Callable),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::Tile),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::AllGraphics),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::AllMesh),
  XII_BITFLAGS_CONSTANT(xiiGALShaderStage::AllRayTracing),
XII_END_STATIC_REFLECTED_BITFLAGS;

const char* xiiGALShaderStage::Names[ENUM_COUNT] = {
  "Vertex",
  "Pixel",
  "Geometry",
  "Hull",
  "Domain",
  "Compute",
  "Amplification",
  "Mesh",
  "RayGeneration",
  "RayMiss",
  "RayClosestHit",
  "RayAnyHit",
  "RayIntersection",
  "Callable",
  "Tile",
};

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALBindFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::VertexBuffer),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::IndexBuffer),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::UniformBuffer),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::ShaderResource),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::StreamOutput),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::RenderTarget),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::DepthStencil),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::UnorderedAccess),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::IndirectDrawArguments),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::InputAttachment),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::RayTracing),
  XII_BITFLAGS_CONSTANT(xiiGALBindFlags::ShadingRate),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALResourceUsage, 1)
  XII_ENUM_CONSTANT(xiiGALResourceUsage::Immutable),
  XII_ENUM_CONSTANT(xiiGALResourceUsage::Dynamic),
  XII_ENUM_CONSTANT(xiiGALResourceUsage::Staging),
  XII_ENUM_CONSTANT(xiiGALResourceUsage::Unified),
  XII_ENUM_CONSTANT(xiiGALResourceUsage::Sparse),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALCPUAccessFlag, 1)
  XII_BITFLAGS_CONSTANT(xiiGALCPUAccessFlag::None),
  XII_BITFLAGS_CONSTANT(xiiGALCPUAccessFlag::Read),
  XII_BITFLAGS_CONSTANT(xiiGALCPUAccessFlag::Write),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALMapType, 1)
  XII_ENUM_CONSTANT(xiiGALMapType::Read),
  XII_ENUM_CONSTANT(xiiGALMapType::Write),
  XII_ENUM_CONSTANT(xiiGALMapType::ReadWrite),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALMapFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALMapFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALMapFlags::DoNotWait),
  XII_BITFLAGS_CONSTANT(xiiGALMapFlags::Discard),
  XII_BITFLAGS_CONSTANT(xiiGALMapFlags::NoOverWrite),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALResourceDimension, 1)
  XII_ENUM_CONSTANT(xiiGALResourceDimension::Undefined),
  XII_ENUM_CONSTANT(xiiGALResourceDimension::Buffer),
  XII_ENUM_CONSTANT(xiiGALResourceDimension::Texture1D),
  XII_ENUM_CONSTANT(xiiGALResourceDimension::Texture1DArray),
  XII_ENUM_CONSTANT(xiiGALResourceDimension::Texture2D),
  XII_ENUM_CONSTANT(xiiGALResourceDimension::Texture2DArray),
  XII_ENUM_CONSTANT(xiiGALResourceDimension::Texture3D),
  XII_ENUM_CONSTANT(xiiGALResourceDimension::TextureCube),
  XII_ENUM_CONSTANT(xiiGALResourceDimension::TextureCubeArray),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALTextureViewType, 1)
  XII_ENUM_CONSTANT(xiiGALTextureViewType::Undefined),
  XII_ENUM_CONSTANT(xiiGALTextureViewType::ShaderResource),
  XII_ENUM_CONSTANT(xiiGALTextureViewType::RenderTarget),
  XII_ENUM_CONSTANT(xiiGALTextureViewType::DepthStencil),
  XII_ENUM_CONSTANT(xiiGALTextureViewType::ReadOnlyDepthStencil),
  XII_ENUM_CONSTANT(xiiGALTextureViewType::UnorderedAccess),
  XII_ENUM_CONSTANT(xiiGALTextureViewType::ShadingRate),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALBufferViewType, 1)
  XII_ENUM_CONSTANT(xiiGALBufferViewType::Undefined),
  XII_ENUM_CONSTANT(xiiGALBufferViewType::ShaderResource),
  XII_ENUM_CONSTANT(xiiGALBufferViewType::UnorderedAccess),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALTextureFormat, 1)
  XII_ENUM_CONSTANT(xiiGALTextureFormat::Unknown),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA32Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA32Float),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA32UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA32SInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGB32Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGB32Float),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGB32UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGB32SInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA16Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA16Float),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA16UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA16UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA16SNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA16SInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG32Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG32Float),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG32UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG32SInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R32G8X24Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::D32FloatS8X24UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R32FloatX8X24Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::X32TypelessG8X24UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGB10A2Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGB10A2UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGB10A2UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG11B10Float),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA8Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA8UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA8UNormalizedSRGB),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA8UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA8SNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGBA8SInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG16Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG16Float),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG16UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG16UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG16SNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG16SInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R32Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::D32Float),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R32Float),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R32UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R32SInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R24G8Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::D24UNormalizedS8UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R24UNormalizedX8Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::X24TypelessG8UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG8Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG8UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG8UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG8SNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG8SInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R16Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R16Float),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::D16UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R16UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R16UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R16SNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R16SInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R8Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R8UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R8UInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R8SNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R8SInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::A8UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R1UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGB9E5SharedExponent),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG8BG8UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::GR8GB8UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC1Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC1UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC1UNormalizedSRGB),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC2Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC2UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC2UNormalizedSRGB),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC3Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC3UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC3UNormalizedSRGB),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC4Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC4UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC4SNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC5Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC5UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC5SNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::B5G6R5UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::B5G5R5A1UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BGRA8UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BGRX8UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R10G10B10XRBiasA2UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BGRA8Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BGRA8UNormalizedSRGB),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BGRX8Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BGRX8UNormalizedSRGB),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC6HTypeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC6HUF16),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC6HSF16),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC7Typeless),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC7UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::BC7UNormalizedSRGB),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALFilterType, 1)
  XII_ENUM_CONSTANT(xiiGALFilterType::Unknown),
  XII_ENUM_CONSTANT(xiiGALFilterType::Point),
  XII_ENUM_CONSTANT(xiiGALFilterType::Linear),
  XII_ENUM_CONSTANT(xiiGALFilterType::Anisotropic),
  XII_ENUM_CONSTANT(xiiGALFilterType::ComparisonPoint),
  XII_ENUM_CONSTANT(xiiGALFilterType::ComparisonLinear),
  XII_ENUM_CONSTANT(xiiGALFilterType::ComparisonAnisotropic),
  XII_ENUM_CONSTANT(xiiGALFilterType::MinimumPoint),
  XII_ENUM_CONSTANT(xiiGALFilterType::MinimumLinear),
  XII_ENUM_CONSTANT(xiiGALFilterType::MinimumAnisotropic),
  XII_ENUM_CONSTANT(xiiGALFilterType::MaximumPoint),
  XII_ENUM_CONSTANT(xiiGALFilterType::MaximumLinear),
  XII_ENUM_CONSTANT(xiiGALFilterType::MaximumAnisotropic),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALTextureAddressMode, 1)
  XII_ENUM_CONSTANT(xiiGALTextureAddressMode::Unknown),
  XII_ENUM_CONSTANT(xiiGALTextureAddressMode::Wrap),
  XII_ENUM_CONSTANT(xiiGALTextureAddressMode::Mirror),
  XII_ENUM_CONSTANT(xiiGALTextureAddressMode::Clamp),
  XII_ENUM_CONSTANT(xiiGALTextureAddressMode::Border),
  XII_ENUM_CONSTANT(xiiGALTextureAddressMode::MirrorOnce),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALComparisonFunction, 1)
  XII_ENUM_CONSTANT(xiiGALComparisonFunction::Unknown),
  XII_ENUM_CONSTANT(xiiGALComparisonFunction::Never),
  XII_ENUM_CONSTANT(xiiGALComparisonFunction::Less),
  XII_ENUM_CONSTANT(xiiGALComparisonFunction::Equal),
  XII_ENUM_CONSTANT(xiiGALComparisonFunction::LessEqual),
  XII_ENUM_CONSTANT(xiiGALComparisonFunction::Greater),
  XII_ENUM_CONSTANT(xiiGALComparisonFunction::NotEqual),
  XII_ENUM_CONSTANT(xiiGALComparisonFunction::GreaterEqual),
  XII_ENUM_CONSTANT(xiiGALComparisonFunction::Always),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALPrimitiveTopology, 1)
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::Undefined),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::TriangleList),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::TriangleStrip),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::PointList),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::LineList),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::LineStrip),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::TriangleListAdjacent),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::TrangleStripAdjacent),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::LineListAdjacent),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::LineStripAdjacent),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList1),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList2),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList3),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList4),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList5),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList6),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList7),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList8),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList9),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList10),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList11),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList12),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList13),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList14),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList15),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList16),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList17),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList18),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList19),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList20),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList21),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList22),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList23),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList24),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList25),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList26),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList27),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList28),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList29),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList30),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList31),
  XII_ENUM_CONSTANT(xiiGALPrimitiveTopology::ControlPointPatchList32),
XII_END_STATIC_REFLECTED_ENUM;

const char* xiiGALPrimitiveTopology::Names[ENUM_COUNT] = {
  "TriangleList",
  "TriangleStrip",
  "PointList",
  "LineList",
  "LineStrip",
  "TriangleListAdjacent",
  "TrangleStripAdjacent",
  "LineListAdjacent",
  "LineStripAdjacent",
  "ControlPointPatchList1",
  "ControlPointPatchList2",
  "ControlPointPatchList3",
  "ControlPointPatchList4",
  "ControlPointPatchList5",
  "ControlPointPatchList6",
  "ControlPointPatchList7",
  "ControlPointPatchList8",
  "ControlPointPatchList9",
  "ControlPointPatchList10",
  "ControlPointPatchList11",
  "ControlPointPatchList12",
  "ControlPointPatchList13",
  "ControlPointPatchList14",
  "ControlPointPatchList15",
  "ControlPointPatchList16",
  "ControlPointPatchList17",
  "ControlPointPatchList18",
  "ControlPointPatchList19",
  "ControlPointPatchList20",
  "ControlPointPatchList21",
  "ControlPointPatchList22",
  "ControlPointPatchList23",
  "ControlPointPatchList24",
  "ControlPointPatchList25",
  "ControlPointPatchList26",
  "ControlPointPatchList27",
  "ControlPointPatchList28",
  "ControlPointPatchList29",
  "ControlPointPatchList30",
  "ControlPointPatchList31",
  "ControlPointPatchList32",
};

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALMemoryProperties, 1)
  XII_BITFLAGS_CONSTANT(xiiGALMemoryProperties::Unknown),
  XII_BITFLAGS_CONSTANT(xiiGALMemoryProperties::HostCoherent),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALDeviceAdapterType, 1)
  XII_ENUM_CONSTANT(xiiGALDeviceAdapterType::Unknown),
  XII_ENUM_CONSTANT(xiiGALDeviceAdapterType::Software),
  XII_ENUM_CONSTANT(xiiGALDeviceAdapterType::Integrated),
  XII_ENUM_CONSTANT(xiiGALDeviceAdapterType::Discrete),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALScalingMode, 1)
  XII_ENUM_CONSTANT(xiiGALScalingMode::Unspecified),
  XII_ENUM_CONSTANT(xiiGALScalingMode::Centered),
  XII_ENUM_CONSTANT(xiiGALScalingMode::Stretched),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALScanLineOrder, 1)
  XII_ENUM_CONSTANT(xiiGALScanLineOrder::Unspecified),
  XII_ENUM_CONSTANT(xiiGALScanLineOrder::Progressive),
  XII_ENUM_CONSTANT(xiiGALScanLineOrder::UpperFieldFirst),
  XII_ENUM_CONSTANT(xiiGALScanLineOrder::LowerFieldFirst),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALSwapChainUsageFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALSwapChainUsageFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALSwapChainUsageFlags::RenderTarget),
  XII_BITFLAGS_CONSTANT(xiiGALSwapChainUsageFlags::ShaderResource),
  XII_BITFLAGS_CONSTANT(xiiGALSwapChainUsageFlags::InputAttachment),
  XII_BITFLAGS_CONSTANT(xiiGALSwapChainUsageFlags::CopySource),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALSurfaceTransform, 1)
  XII_ENUM_CONSTANT(xiiGALSurfaceTransform::Optimal),
  XII_ENUM_CONSTANT(xiiGALSurfaceTransform::Identity),
  XII_ENUM_CONSTANT(xiiGALSurfaceTransform::Rotate90),
  XII_ENUM_CONSTANT(xiiGALSurfaceTransform::Rotate180),
  XII_ENUM_CONSTANT(xiiGALSurfaceTransform::Rotate270),
  XII_ENUM_CONSTANT(xiiGALSurfaceTransform::HorizontalMirror),
  XII_ENUM_CONSTANT(xiiGALSurfaceTransform::HorizontalMirrorRotate90),
  XII_ENUM_CONSTANT(xiiGALSurfaceTransform::HorizontalMirrorRotate180),
  XII_ENUM_CONSTANT(xiiGALSurfaceTransform::HorizontalMirrorRotate270),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALQueryType, 1)
  XII_ENUM_CONSTANT(xiiGALQueryType::Undefined),
  XII_ENUM_CONSTANT(xiiGALQueryType::Occlusion),
  XII_ENUM_CONSTANT(xiiGALQueryType::BinaryOcclusion),
  XII_ENUM_CONSTANT(xiiGALQueryType::Timestamp),
  XII_ENUM_CONSTANT(xiiGALQueryType::PipelineStatistics),
  XII_ENUM_CONSTANT(xiiGALQueryType::Duration),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALWaveFeature, 1)
  XII_BITFLAGS_CONSTANT(xiiGALWaveFeature::Unknown),
  XII_BITFLAGS_CONSTANT(xiiGALWaveFeature::Basic),
  XII_BITFLAGS_CONSTANT(xiiGALWaveFeature::Vote),
  XII_BITFLAGS_CONSTANT(xiiGALWaveFeature::Arithmetic),
  XII_BITFLAGS_CONSTANT(xiiGALWaveFeature::BallOut),
  XII_BITFLAGS_CONSTANT(xiiGALWaveFeature::Shuffle),
  XII_BITFLAGS_CONSTANT(xiiGALWaveFeature::ShuffleRelative),
  XII_BITFLAGS_CONSTANT(xiiGALWaveFeature::Clustered),
  XII_BITFLAGS_CONSTANT(xiiGALWaveFeature::Quad),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALRayTracingCapabilityFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingCapabilityFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingCapabilityFlags::StandaloneShaders),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingCapabilityFlags::InlineRayTracing),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingCapabilityFlags::IndirectRayTracing),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALValidationFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALValidationFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALValidationFlags::CheckShaderBufferSize),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALCommandQueueType, 1)
  XII_BITFLAGS_CONSTANT(xiiGALCommandQueueType::Unknown),
  XII_BITFLAGS_CONSTANT(xiiGALCommandQueueType::Transfer),
  XII_BITFLAGS_CONSTANT(xiiGALCommandQueueType::Compute),
  XII_BITFLAGS_CONSTANT(xiiGALCommandQueueType::Graphics),
  XII_BITFLAGS_CONSTANT(xiiGALCommandQueueType::SparseBinding),
  XII_BITFLAGS_CONSTANT(xiiGALCommandQueueType::PrimaryType),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALCommandQueuePriority, 1)
  XII_ENUM_CONSTANT(xiiGALCommandQueuePriority::Unknown),
  XII_ENUM_CONSTANT(xiiGALCommandQueuePriority::Low),
  XII_ENUM_CONSTANT(xiiGALCommandQueuePriority::Medium),
  XII_ENUM_CONSTANT(xiiGALCommandQueuePriority::High),
  XII_ENUM_CONSTANT(xiiGALCommandQueuePriority::RealTime),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALShadingRateCombiner, 1)
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCombiner::PassThrough),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCombiner::CombinerOverride),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCombiner::CombinerMin),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCombiner::CombinerMax),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCombiner::CombinerSum),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCombiner::CombinerMul),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShadingRateFormat, 1)
  XII_ENUM_CONSTANT(xiiGALShadingRateFormat::Unknown),
  XII_ENUM_CONSTANT(xiiGALShadingRateFormat::Palette),
  XII_ENUM_CONSTANT(xiiGALShadingRateFormat::RG8UNormalized),
  XII_ENUM_CONSTANT(xiiGALShadingRateFormat::ColumnRowFloat32),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShadingRateAxis, 1)
  XII_ENUM_CONSTANT(xiiGALShadingRateAxis::X1),
  XII_ENUM_CONSTANT(xiiGALShadingRateAxis::X2),
  XII_ENUM_CONSTANT(xiiGALShadingRateAxis::X4),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALShadingRate, 1)
  XII_BITFLAGS_CONSTANT(xiiGALShadingRate::_1X1),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRate::_1X2),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRate::_1X4),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRate::_2X1),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRate::_2X2),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRate::_2X4),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRate::_4X1),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRate::_4X2),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRate::_4X4),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALSampleCount, 1)
  XII_ENUM_CONSTANT(xiiGALSampleCount::None),
  XII_ENUM_CONSTANT(xiiGALSampleCount::OneSample),
  XII_ENUM_CONSTANT(xiiGALSampleCount::TwoSamples),
  XII_ENUM_CONSTANT(xiiGALSampleCount::FourSamples),
  XII_ENUM_CONSTANT(xiiGALSampleCount::EightSamples),
  XII_ENUM_CONSTANT(xiiGALSampleCount::SixteenSamples),
  XII_ENUM_CONSTANT(xiiGALSampleCount::ThirtyTwoSamples),
  XII_ENUM_CONSTANT(xiiGALSampleCount::SixtyFourSamples),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALShadingRateCapabilityFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::PerDraw),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::PerPrimitive),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::TextureBased),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::SampleMask),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::ShaderSampleMask),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::ShaderDepthStencilWrite),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::PerPrimitiveWithMultipleViewports),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::SameTextureForWholeRenderPass),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::TextureArray),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::ShadingRateShaderInput),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::AdditionalInvocations),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::NonSubSampledRenderTarget),
  XII_BITFLAGS_CONSTANT(xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShadingRateTextureAccess, 1)
  XII_ENUM_CONSTANT(xiiGALShadingRateTextureAccess::Unknown),
  XII_ENUM_CONSTANT(xiiGALShadingRateTextureAccess::OnGPU),
  XII_ENUM_CONSTANT(xiiGALShadingRateTextureAccess::OnSubmit),
  XII_ENUM_CONSTANT(xiiGALShadingRateTextureAccess::OnSetRenderTarget),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALDrawCommandCapabilityFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALDrawCommandCapabilityFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALDrawCommandCapabilityFlags::BaseVertex),
  XII_BITFLAGS_CONSTANT(xiiGALDrawCommandCapabilityFlags::DrawIndirect),
  XII_BITFLAGS_CONSTANT(xiiGALDrawCommandCapabilityFlags::DrawIndirectFirstInstance),
  XII_BITFLAGS_CONSTANT(xiiGALDrawCommandCapabilityFlags::NativeMultiDrawIndirect),
  XII_BITFLAGS_CONSTANT(xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALSparseResourceCapabilityFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::ShaderResourceResidency),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Buffer),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture2D),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture3D),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture2Samples),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture4Samples),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture8Samples),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture16Samples),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Aliased),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Standard2DTileShape),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Standard2DMSTileShape),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Standard3DTileShape),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::AlignedMipSize),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::NonResidentStrict),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture2DArrayMipTail),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::BufferStandardBlock),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::NonResidentSafe),
  XII_BITFLAGS_CONSTANT(xiiGALSparseResourceCapabilityFlags::MixedResourceTypeSupport),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALTextureFormatComponentType, 1)
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::Undefined),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::Float),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::SignedNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::UnsignedNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::UnsignedNormalizedSRGB),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::SignedInteger),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::UnsignedInteger),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::Depth),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::DepthStencil),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::Compound),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::Compressed),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALResourceDimensionCapabilityFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALResourceDimensionCapabilityFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Buffer),
  XII_BITFLAGS_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Texture1D),
  XII_BITFLAGS_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Texture1DArray),
  XII_BITFLAGS_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Texture2D),
  XII_BITFLAGS_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Texture2DArray),
  XII_BITFLAGS_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Texture3D),
  XII_BITFLAGS_CONSTANT(xiiGALResourceDimensionCapabilityFlags::TextureCube),
  XII_BITFLAGS_CONSTANT(xiiGALResourceDimensionCapabilityFlags::TextureCubeArray),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALSparseTextureFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALSparseTextureFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALSparseTextureFlags::SingleMipTail),
  XII_BITFLAGS_CONSTANT(xiiGALSparseTextureFlags::AlignedMipSize),
  XII_BITFLAGS_CONSTANT(xiiGALSparseTextureFlags::NonStandardBlockSize),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALResourceStateFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::Unknown),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::Undefined),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::VertexBuffer),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::ConstantBuffer),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::IndexBuffer),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::RenderTarget),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::UnorderedAccess),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::DepthWrite),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::DepthRead),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::ShaderResource),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::StreamOut),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::IndirectArgument),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::CopyDestination),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::CopySource),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::ResolveDestination),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::ResolveSource),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::InputAttachment),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::Present),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::BuildAsRead),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::BuildAsWrite),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::RayTracing),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::Common),
  XII_BITFLAGS_CONSTANT(xiiGALResourceStateFlags::ShadingRate),
XII_END_STATIC_REFLECTED_BITFLAGS;

// clang-format on

const xiiUInt8 xiiGALTextureFormat::s_BitsPerElement[xiiGALTextureFormat::ENUM_COUNT] = {
  0U,   // Unknown
  128U, // RGBA32Typeless
  128U, // RGBA32Float
  128U, // RGBA32UInt
  128U, // RGBA32SInt
  96U,  // RGB32Typeless
  96U,  // RGB32Float
  96U,  // RGB32UInt
  96U,  // RGB32SInt
  64U,  // RGBA16Typeless
  64U,  // RGBA16Float
  64U,  // RGBA16UNormalized
  64U,  // RGBA16UInt
  64U,  // RGBA16SNormalized
  64U,  // RGBA16SInt
  64U,  // RG32Typeless
  64U,  // RG32Float
  64U,  // RG32UInt
  64U,  // RG32SInt
  64U,  // R32G8X24Typeless
  64U,  // D32FloatS8X24UInt
  64U,  // R32FloatX8X24Typeless
  64U,  // X32TypelessG8X24UInt
  32U,  // RGB10A2Typeless
  32U,  // RGB10A2UNormalized
  32U,  // RGB10A2UInt
  32U,  // RG11B10Float
  32U,  // RGBA8Typeless
  32U,  // RGBA8UNormalized
  32U,  // RGBA8UNormalizedSRGB
  32U,  // RGBA8UInt
  32U,  // RGBA8SNormalized
  32U,  // RGBA8SInt
  32U,  // RG16Typeless
  32U,  // RG16Float
  32U,  // RG16UNormalized
  32U,  // RG16UInt
  32U,  // RG16SNormalized
  32U,  // RG16SInt
  32U,  // R32Typeless
  32U,  // D32Float
  32U,  // R32Float
  32U,  // R32UInt
  32U,  // R32SInt
  32U,  // R24G8Typeless
  32U,  // D24UNormalizedS8UInt
  32U,  // R24UNormalizedX8Typeless
  32U,  // X24TypelessG8UInt
  16U,  // RG8Typeless
  16U,  // RG8UNormalized
  16U,  // RG8UInt
  16U,  // RG8SNormalized
  16U,  // RG8SInt
  16U,  // R16Typeless
  16U,  // R16Float
  16U,  // D16UNormalized
  16U,  // R16UNormalized
  16U,  // R16UInt
  16U,  // R16SNormalized
  16U,  // R16SInt
  8U,   // R8Typeless
  8U,   // R8UNormalized
  8U,   // R8UInt
  8U,   // R8SNorm
  8U,   // R8SInt
  8U,   // A8UNormalized
  1U,   // R1UNormalized
  32U,  // RGB9E5SharedExponent
  32U,  // RG8BG8UNormalized
  32U,  // GR8GB8UNormalized
  4U,   // BC1Typeless
  4U,   // BC1UNormalized
  4U,   // BC1UNormalizedSRGB
  8U,   // BC2Typeless
  8U,   // BC2UNormalized
  8U,   // BC2UNormalizedSRGB
  8U,   // BC3Typeless
  8U,   // BC3UNormalized
  8U,   // BC3UNormalizedSRGB
  4U,   // BC4Typeless
  4U,   // BC4UNormalized
  4U,   // BC4SNormalized
  8U,   // BC5Typeless
  8U,   // BC5UNormalized
  8U,   // BC5SNormalized
  16U,  // B5G6R5UNormalized
  16U,  // B5G5R5A1UNormalized
  32U,  // BGRA8UNormalized
  32U,  // BGRX8UNormalized
  32U,  // R10G10B10XRBiasA2UNormalized
  32U,  // BGRA8Typeless
  32U,  // BGRA8UNormalizedSRGB
  32U,  // BGRX8Typeless
  32U,  // BGRX8UNormalizedSRGB
  8U,   // BC6HTypeless
  8U,   // BC6HUF16
  8U,   // BC6HSF16
  8U,   // BC7Typeless
  8U,   // BC7UNormalized
  8U,   // BC7UNormalizedSRGB
};

const xiiUInt8 xiiGALTextureFormat::s_ChannelCount[xiiGALTextureFormat::ENUM_COUNT] = {
  0U, // Unknown
  4U, // RGBA32Typeless
  4U, // RGBA32Float
  4U, // RGBA32UInt
  4U, // RGBA32SInt
  3U, // RGB32Typeless
  3U, // RGB32Float
  3U, // RGB32UInt
  3U, // RGB32SInt
  4U, // RGBA16Typeless
  4U, // RGBA16Float
  4U, // RGBA16UNormalized
  4U, // RGBA16UInt
  4U, // RGBA16SNormalized
  4U, // RGBA16SInt
  2U, // RG32Typeless
  2U, // RG32Float
  2U, // RG32UInt
  2U, // RG32SInt
  2U, // R32G8X24Typeless
  2U, // D32FloatS8X24UInt
  2U, // R32FloatX8X24Typeless
  2U, // X32TypelessG8X24UInt
  4U, // RGB10A2Typeless
  4U, // RGB10A2UNormalized
  4U, // RGB10A2UInt
  3U, // RG11B10Float
  4U, // RGBA8Typeless
  4U, // RGBA8UNormalized
  4U, // RGBA8UNormalizedSRGB
  4U, // RGBA8UInt
  4U, // RGBA8SNormalized
  4U, // RGBA8SInt
  2U, // RG16Typeless
  2U, // RG16Float
  2U, // RG16UNormalized
  2U, // RG16UInt
  2U, // RG16SNormalized
  2U, // RG16SInt
  1U, // R32Typeless
  1U, // D32Float
  1U, // R32Float
  1U, // R32UInt
  1U, // R32SInt
  2U, // R24G8Typeless
  2U, // D24UNormalizedS8UInt
  2U, // R24UNormalizedX8Typeless
  2U, // X24TypelessG8UInt
  2U, // RG8Typeless
  2U, // RG8UNormalized
  2U, // RG8UInt
  2U, // RG8SNormalized
  2U, // RG8SInt
  1U, // R16Typeless
  1U, // R16Float
  1U, // D16UNormalized
  1U, // R16UNormalized
  1U, // R16UInt
  1U, // R16SNormalized
  1U, // R16SInt
  1U, // R8Typeless
  1U, // R8UNormalized
  1U, // R8UInt
  1U, // R8SNorm
  1U, // R8SInt
  1U, // A8UNormalized
  1U, // R1UNormalized
  3U, // RGB9E5SharedExponent
  4U, // RG8BG8UNormalized
  4U, // GR8GB8UNormalized
  4U, // BC1Typeless
  4U, // BC1UNormalized
  4U, // BC1UNormalizedSRGB
  4U, // BC2Typeless
  4U, // BC2UNormalized
  4U, // BC2UNormalizedSRGB
  4U, // BC3Typeless
  4U, // BC3UNormalized
  4U, // BC3UNormalizedSRGB
  1U, // BC4Typeless
  1U, // BC4UNormalized
  1U, // BC4SNormalized
  2U, // BC5Typeless
  2U, // BC5UNormalized
  2U, // BC5SNormalized
  3U, // B5G6R5UNormalized
  4U, // B5G5R5A1UNormalized
  4U, // BGRA8UNormalized
  4U, // BGRX8UNormalized
  4U, // R10G10B10XRBiasA2UNormalized
  4U, // BGRA8Typeless
  4U, // BGRA8UNormalizedSRGB
  4U, // BGRX8Typeless
  4U, // BGRX8UNormalizedSRGB
  3U, // BC6HTypeless
  3U, // BC6HUF16
  3U, // BC6HSF16
  4U, // BC7Typeless
  4U, // BC7UNormalized
  4U, // BC7UNormalizedSRGB
};
