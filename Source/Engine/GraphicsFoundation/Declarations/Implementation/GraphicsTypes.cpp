#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALGraphicsDeviceType, 1)
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceType::Undefined),
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceType::D3D11),
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceType::D3D12),
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceType::Vulkan),
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

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALGraphicsDeviceValidationLevel, 1)
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceValidationLevel::Disabled),
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceValidationLevel::Standard),
  XII_ENUM_CONSTANT(xiiGALGraphicsDeviceValidationLevel::All),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALDeviceEventType, 1)
  XII_ENUM_CONSTANT(xiiGALDeviceEventType::Unknown),
  XII_ENUM_CONSTANT(xiiGALDeviceEventType::AfterInit),
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

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALCPUAccessFlag, 1)
  XII_ENUM_CONSTANT(xiiGALCPUAccessFlag::None),
  XII_ENUM_CONSTANT(xiiGALCPUAccessFlag::Read),
  XII_ENUM_CONSTANT(xiiGALCPUAccessFlag::Write),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALMapType, 1)
  XII_ENUM_CONSTANT(xiiGALMapType::Read),
  XII_ENUM_CONSTANT(xiiGALMapType::Write),
  XII_ENUM_CONSTANT(xiiGALMapType::ReadWrite),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALMapFlag, 1)
  XII_ENUM_CONSTANT(xiiGALMapFlag::None),
  XII_ENUM_CONSTANT(xiiGALMapFlag::DoNotWait),
  XII_ENUM_CONSTANT(xiiGALMapFlag::Discard),
  XII_ENUM_CONSTANT(xiiGALMapFlag::NoOverWrite),
XII_END_STATIC_REFLECTED_ENUM;

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
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R8SNorm),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R8SInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::A8UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::R1UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGB9E5SharedExponent),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RG8BG8UNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormat::RGR8GB8UNormalized),
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

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALMemoryProperties, 1)
  XII_ENUM_CONSTANT(xiiGALMemoryProperties::Unknown),
  XII_ENUM_CONSTANT(xiiGALMemoryProperties::HostCoherent),
XII_END_STATIC_REFLECTED_ENUM;

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

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALSwapChainUsageFlags, 1)
  XII_ENUM_CONSTANT(xiiGALSwapChainUsageFlags::None),
  XII_ENUM_CONSTANT(xiiGALSwapChainUsageFlags::RenderTarget),
  XII_ENUM_CONSTANT(xiiGALSwapChainUsageFlags::ShaderResource),
  XII_ENUM_CONSTANT(xiiGALSwapChainUsageFlags::InputAttachment),
  XII_ENUM_CONSTANT(xiiGALSwapChainUsageFlags::CopySource),
XII_END_STATIC_REFLECTED_ENUM;

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

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALWaveFeature, 1)
  XII_ENUM_CONSTANT(xiiGALWaveFeature::Unknown),
  XII_ENUM_CONSTANT(xiiGALWaveFeature::Basic),
  XII_ENUM_CONSTANT(xiiGALWaveFeature::Vote),
  XII_ENUM_CONSTANT(xiiGALWaveFeature::Arithmetic),
  XII_ENUM_CONSTANT(xiiGALWaveFeature::BallOut),
  XII_ENUM_CONSTANT(xiiGALWaveFeature::Shuffle),
  XII_ENUM_CONSTANT(xiiGALWaveFeature::ShuffleRelative),
  XII_ENUM_CONSTANT(xiiGALWaveFeature::Clustered),
  XII_ENUM_CONSTANT(xiiGALWaveFeature::Quad),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALRayTracingCapabilityFlags, 1)
  XII_ENUM_CONSTANT(xiiGALRayTracingCapabilityFlags::None),
  XII_ENUM_CONSTANT(xiiGALRayTracingCapabilityFlags::StandaloneShaders),
  XII_ENUM_CONSTANT(xiiGALRayTracingCapabilityFlags::InlineRayTracing),
  XII_ENUM_CONSTANT(xiiGALRayTracingCapabilityFlags::IndirectRayTracing),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALCommandQueueType, 1)
  XII_ENUM_CONSTANT(xiiGALCommandQueueType::Unknown),
  XII_ENUM_CONSTANT(xiiGALCommandQueueType::Transfer),
  XII_ENUM_CONSTANT(xiiGALCommandQueueType::Compute),
  XII_ENUM_CONSTANT(xiiGALCommandQueueType::Graphics),
  XII_ENUM_CONSTANT(xiiGALCommandQueueType::SparseBinding),
  XII_ENUM_CONSTANT(xiiGALCommandQueueType::PrimaryType),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALCommandQueuePriority, 1)
  XII_ENUM_CONSTANT(xiiGALCommandQueuePriority::Unknown),
  XII_ENUM_CONSTANT(xiiGALCommandQueuePriority::Low),
  XII_ENUM_CONSTANT(xiiGALCommandQueuePriority::Medium),
  XII_ENUM_CONSTANT(xiiGALCommandQueuePriority::High),
  XII_ENUM_CONSTANT(xiiGALCommandQueuePriority::RealTime),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShadingRateCombiner, 1)
  XII_ENUM_CONSTANT(xiiGALShadingRateCombiner::PassThrough),
  XII_ENUM_CONSTANT(xiiGALShadingRateCombiner::CombinerOverride),
  XII_ENUM_CONSTANT(xiiGALShadingRateCombiner::CombinerMin),
  XII_ENUM_CONSTANT(xiiGALShadingRateCombiner::CombinerMax),
  XII_ENUM_CONSTANT(xiiGALShadingRateCombiner::CombinerSum),
  XII_ENUM_CONSTANT(xiiGALShadingRateCombiner::CombinerMul),
XII_END_STATIC_REFLECTED_ENUM;

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

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShadingRate, 1)
  XII_ENUM_CONSTANT(xiiGALShadingRate::_1X1),
  XII_ENUM_CONSTANT(xiiGALShadingRate::_1X2),
  XII_ENUM_CONSTANT(xiiGALShadingRate::_1X4),
  XII_ENUM_CONSTANT(xiiGALShadingRate::_2X1),
  XII_ENUM_CONSTANT(xiiGALShadingRate::_2X2),
  XII_ENUM_CONSTANT(xiiGALShadingRate::_2X4),
  XII_ENUM_CONSTANT(xiiGALShadingRate::_4X1),
  XII_ENUM_CONSTANT(xiiGALShadingRate::_4X2),
  XII_ENUM_CONSTANT(xiiGALShadingRate::_4X4),
XII_END_STATIC_REFLECTED_ENUM;

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

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShadingRateCapabilityFlags, 1)
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::None),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::PerDraw),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::PerPrimitive),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::TextureBased),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::SampleMask),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::ShaderSampleMask),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::ShaderDepthStencilWrite),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::PerPrimitiveWithMultipleViewports),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::SameTextureForWholeRenderPass),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::TextureArray),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::ShadingRateShaderInput),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::AdditionalInvocations),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::NonSubSampledRenderTarget),
  XII_ENUM_CONSTANT(xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShadingRateTextureAccess, 1)
  XII_ENUM_CONSTANT(xiiGALShadingRateTextureAccess::Unknown),
  XII_ENUM_CONSTANT(xiiGALShadingRateTextureAccess::OnGPU),
  XII_ENUM_CONSTANT(xiiGALShadingRateTextureAccess::OnSubmit),
  XII_ENUM_CONSTANT(xiiGALShadingRateTextureAccess::OnSetRenderTarget),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALDrawCommandCapabilityFlags, 1)
  XII_ENUM_CONSTANT(xiiGALDrawCommandCapabilityFlags::None),
  XII_ENUM_CONSTANT(xiiGALDrawCommandCapabilityFlags::BaseVertex),
  XII_ENUM_CONSTANT(xiiGALDrawCommandCapabilityFlags::DrawIndirect),
  XII_ENUM_CONSTANT(xiiGALDrawCommandCapabilityFlags::DrawIndirectFirstInstance),
  XII_ENUM_CONSTANT(xiiGALDrawCommandCapabilityFlags::NativeMultiDrawIndirect),
  XII_ENUM_CONSTANT(xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALSparseResourceCapabilityFlags, 1)
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::None),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::ShaderResourceResidency),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Buffer),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture2D),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture3D),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture2Samples),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture4Samples),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture8Samples),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture16Samples),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Aliased),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Standard2DTileShape),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Standard2DMSTileShape),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Standard3DTileShape),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::AlignedMipSize),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::NonResidentStrict),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::Texture2DArrayMipTail),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::BufferStandardBlock),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::NonResidentSafe),
  XII_ENUM_CONSTANT(xiiGALSparseResourceCapabilityFlags::MixedResourceTypeSupport),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALTextureFormatComponentType, 1)
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::Undefined),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::Float),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::SignedNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::UnsignedNormalized),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::UnsignedNormalizedSRGB),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::SignedInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::UnsignedInt),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::Depth),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::DepthStencil),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::Compound),
  XII_ENUM_CONSTANT(xiiGALTextureFormatComponentType::Compressed),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALResourceDimensionCapabilityFlags, 1)
  XII_ENUM_CONSTANT(xiiGALResourceDimensionCapabilityFlags::None),
  XII_ENUM_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Buffer),
  XII_ENUM_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Texture1D),
  XII_ENUM_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Texture1DArray),
  XII_ENUM_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Texture2D),
  XII_ENUM_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Texture2DArray),
  XII_ENUM_CONSTANT(xiiGALResourceDimensionCapabilityFlags::Texture3D),
  XII_ENUM_CONSTANT(xiiGALResourceDimensionCapabilityFlags::TextureCube),
  XII_ENUM_CONSTANT(xiiGALResourceDimensionCapabilityFlags::TextureCubeArray),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALSparseTextureFlags, 1)
  XII_ENUM_CONSTANT(xiiGALSparseTextureFlags::None),
  XII_ENUM_CONSTANT(xiiGALSparseTextureFlags::SingleMipTail),
  XII_ENUM_CONSTANT(xiiGALSparseTextureFlags::AlignedMipSize),
  XII_ENUM_CONSTANT(xiiGALSparseTextureFlags::NonStandardBlockSize),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALResourceStateFlags, 1)
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::Unknown),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::Undefined),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::VertexBuffer),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::ConstantBuffer),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::IndexBuffer),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::RenderTarget),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::UnorderedAccess),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::DepthWrite),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::DepthRead),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::ShaderResource),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::StreamOut),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::IndirectArgument),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::CopyDestination),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::CopySource),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::ResolveDestination),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::ResolveSource),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::InputAttachment),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::Present),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::BuildAsRead),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::BuildAsWrite),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::RayTracing),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::Common),
  XII_ENUM_CONSTANT(xiiGALResourceStateFlags::ShadingRate),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALBlendFactor, 1)
  XII_ENUM_CONSTANT(xiiGALBlendFactor::Undefined),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::Zero),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::One),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::SourceColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseSourceColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::SourceAlpha),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseSourceAlpha),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::DestinationAlpha),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseDestinationAlpha),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::DestinationColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseDestinationColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::SourceAlphaSaturate),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::BlendFactor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseBlendFactor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::SourceOneColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseSourceOneColor),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::SourceOneAlpha),
  XII_ENUM_CONSTANT(xiiGALBlendFactor::InverseSourceOneAlpha),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALBlendOperation, 1)
  XII_ENUM_CONSTANT(xiiGALBlendOperation::Undefined),
  XII_ENUM_CONSTANT(xiiGALBlendOperation::Add),
  XII_ENUM_CONSTANT(xiiGALBlendOperation::Subtract),
  XII_ENUM_CONSTANT(xiiGALBlendOperation::ReverseSubtract),
  XII_ENUM_CONSTANT(xiiGALBlendOperation::Min),
  XII_ENUM_CONSTANT(xiiGALBlendOperation::Max),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALColorMask, 1)
  XII_ENUM_CONSTANT(xiiGALColorMask::None),
  XII_ENUM_CONSTANT(xiiGALColorMask::Red),
  XII_ENUM_CONSTANT(xiiGALColorMask::Green),
  XII_ENUM_CONSTANT(xiiGALColorMask::Blue),
  XII_ENUM_CONSTANT(xiiGALColorMask::Alpha),
  XII_ENUM_CONSTANT(xiiGALColorMask::RG),
  XII_ENUM_CONSTANT(xiiGALColorMask::RGB),
  XII_ENUM_CONSTANT(xiiGALColorMask::RGBA),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on
