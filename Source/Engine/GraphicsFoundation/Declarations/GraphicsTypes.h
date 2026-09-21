/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/Constants.h>

/// Defines the graphics device.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsDeviceType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Null = 0U,  ///< Quasi implementation of a graphics device.
    Vulkan,     ///< Vulkan grsaphics device.
    Direct3D12, ///< Direct3D 12 graphics device.
    Metal,      ///< Metal graphics device.

    ENUM_COUNT,

    Default = Null
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALGraphicsDeviceType);

/// This describes the graphics device feature state.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceFeatureState
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Disabled = 0U, ///< Device feature is disabled.
    Enabled,       ///< Device feature is enabled. If a feature is requested to be enabled during the initialization but is not supported by the device/driver/platform, the device will fail to be initialized.
    Optional,      ///< Device feature is optional. The device will attempt to enable the feature during initialization. If the feature is not supported by the device/driver/platform, the device will initialize successfully, but the feature will be disabled.
                   ///< The actual feature state can be queried from the device capabilities description.

    ENUM_COUNT,

    Default = Disabled
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALDeviceFeatureState);

/// This describes the graphics device adapter vendor.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsAdapterVendor
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U,            ///< Adapter vendor is unknowm.
    Nvidia,                  ///< Adapter vendor is NVidia.
    AMD,                     ///< Adapter vendor is AMD.
    Intel,                   ///< Adapter vendor is Intel.
    ARM,                     ///< Adapter vendor is ARM.
    Qualcomm,                ///< Adapter vendor is Qualcomm.
    ImaginationTechnologies, ///< Adapter vendor is Imagination Technologies.
    Microsoft,               ///< Adapter vendor is Microsoft (software rasterizer).
    Apple,                   ///< Adapter vendor is Apple.
    Mesa,                    ///< Adapter vendor is Mesa (software rasterizer).
    Broadcom,                ///< Adapter vendor is Broadcom (Raspberry Pi).

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALGraphicsAdapterVendor);

/// This describes common validation levels that translate to specific settings for different backends.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceValidationLevel
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Disabled = 0U, ///< Validation is disabled.
    Standard,      ///< Standard validation options are enabled.
    All,           ///< All validation options are enabled. Note that enabling this level may add a significant overhead.

    ENUM_COUNT,

    Default = Disabled
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALDeviceValidationLevel);

struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceEventType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U,        ///< Unknown device event.
    AfterInitialization, ///< After initialization.
    BeforeBeginFrame,    ///< Before begin frame.
    AfterBeginFrame,     ///< After begin frame.
    BeforeEndFrame,      ///< Before end frame.
    AfterEndFrame,       ///< After end frame.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALDeviceEventType);

/// This describes the represented value type. It is used by the buffer description
/// to describe the value type of a formatted buffer, and also used to specify the index type
/// for an indexed draw call.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALValueType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0U, ///< Undefined type.
    Int8,           ///< Signed 8-bit integer.
    Int16,          ///< Signed 16-bit integer.
    Int32,          ///< Signed 32-bit integer.
    UInt8,          ///< Unsigned 8-bit integer.
    UInt16,         ///< Unsigned 16-bit integer.
    UInt32,         ///< Unsigned 32-bit integer.
    Float16,        ///< Half-precision 16-bit floating point value.
    Float32,        ///< Full-precision 32-bit floating point value.
    Float64,        ///< Double-precision 64-bit floating point value.

    ENUM_COUNT,

    Default = Undefined
  };

  /// This returns the size in bytes, of the given value type.
  XII_ALWAYS_INLINE static xiiUInt32 GetSize(const xiiGALValueType::Enum type)
  {
    switch (type)
    {
      case xiiGALValueType::Int8:
        return 1U;
      case xiiGALValueType::Int16:
        return 2U;
      case xiiGALValueType::Int32:
        return 4U;
      case xiiGALValueType::UInt8:
        return 1U;
      case xiiGALValueType::UInt16:
        return 2U;
      case xiiGALValueType::UInt32:
        return 4U;
      case xiiGALValueType::Float16:
        return 2U;
      case xiiGALValueType::Float32:
        return 4U;
      case xiiGALValueType::Float64:
        return 8U;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return 0U;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALValueType);

/// This describes the shader stage.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderType
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    Unknown         = 0U,          ///< Unknown shader stage.
    Vertex          = XII_BIT(0),  ///< Vertex shader.
    Pixel           = XII_BIT(1),  ///< Pixel (fragment) shader.
    Geometry        = XII_BIT(2),  ///< Geometry shader.
    Hull            = XII_BIT(3),  ///< Hull (tessellation control) shader.
    Domain          = XII_BIT(4),  ///< Domain (tessellation evaluation) shader.
    Compute         = XII_BIT(5),  ///< Compute shader.
    Amplification   = XII_BIT(6),  ///< Amplification (task) shader.
    Mesh            = XII_BIT(7),  ///< Mesh shader.
    RayGeneration   = XII_BIT(8),  ///< Ray generation shader.
    RayMiss         = XII_BIT(9),  ///< Ray miss shader.
    RayClosestHit   = XII_BIT(10), ///< Ray closest hit shader.
    RayAnyHit       = XII_BIT(11), ///< Ray any hit shader.
    RayIntersection = XII_BIT(12), ///< Ray intersection shader.
    Callable        = XII_BIT(13), ///< Callable shader.
    Tile            = XII_BIT(14), ///< Tile shader (Only for Metal graphics device).

    ENUM_COUNT = 15U,

    AllGraphics   = Vertex | Pixel | Geometry | Hull | Domain,                                        ///< All graphics pipeline shader stages.
    AllMesh       = Amplification | Mesh | Pixel,                                                     ///< All mesh shading pipeline stages.
    AllRayTracing = RayGeneration | RayMiss | RayClosestHit | RayAnyHit | RayIntersection | Callable, ///< All ray-tracing shader stages.

    All = Vertex | Pixel | Geometry | Hull | Domain | Compute | Amplification | Mesh | RayGeneration | RayMiss | RayClosestHit | RayAnyHit | RayIntersection | Callable | Tile, ///< All shader stages.

    Default = Unknown
  };

  struct Bits
  {
    StorageType Vertex : 1;
    StorageType Pixel : 1;
    StorageType Geometry : 1;
    StorageType Hull : 1;
    StorageType Domain : 1;
    StorageType Compute : 1;
    StorageType Amplification : 1;
    StorageType Mesh : 1;
    StorageType RayGeneration : 1;
    StorageType RayMiss : 1;
    StorageType RayClosestHit : 1;
    StorageType RayAnyHit : 1;
    StorageType RayIntersection : 1;
    StorageType Callable : 1;
    StorageType Tile : 1;

    StorageType AllGraphics : 1;
    StorageType AllMesh : 1;
    StorageType AllRayTracing : 1;
  };

  /// Retrieves the shader stage index of a single shader stage.
  static xiiUInt32 GetStageIndex(xiiGALShaderType::Enum stage);

  /// Returns a the stage flag for a given stage index.
  static xiiGALShaderType::Enum GetStageFlag(xiiUInt32 uiIndex);

  static const char* Names[ENUM_COUNT];

  // Note: Changes to this enumeration flag requires updating code paths.
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALShaderType);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShaderType);

/// [D3D11_BIND_FLAG]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_bind_flag
///
/// This describes which parts of the pipeline a resource can be bound to.
/// It generally mirrors [D3D11_BIND_FLAG][] enumeration. It is used by the buffer description to describe
/// the bind flags for a buffer, and also used in the texture description to describe the bind flags for a texture.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBindFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None                  = 0U,          ///< Undefined binding.
    VertexBuffer          = XII_BIT(0),  ///< A buffer may be bound as a vertex buffer.
    IndexBuffer           = XII_BIT(1),  ///< A buffer may be bound as an index buffer.
    UniformBuffer         = XII_BIT(2),  ///< A buffer may be bound as a uniform buffer. Note that this flag may not be combined with any other bind flag.
    ShaderResource        = XII_BIT(3),  ///< A buffer or texture may be bound as a shader resource.
    StreamOutput          = XII_BIT(4),  ///< A buffer may be bound as a target for the stream output stage.
    RenderTarget          = XII_BIT(5),  ///< A texture may be bound as a render target.
    DepthStencil          = XII_BIT(6),  ///< A texture may be bound as a depth-stencil target.
    UnorderedAccess       = XII_BIT(7),  ///< A buffer or texture may be bound as an unordered access view.
    IndirectDrawArguments = XII_BIT(8),  ///< A buffer may be bound as the source buffer for indirect draw commands.
    InputAttachment       = XII_BIT(9),  ///< A texture may be bound as a render pass input argument.
    RayTracing            = XII_BIT(10), ///< A buffer may be used as a scratch buffer or as the source of primitive data for acceleration structure building.
    ShadingRate           = XII_BIT(11), ///< A texture may be used as shading rate texture.

    BindAll = VertexBuffer | IndexBuffer | UniformBuffer | ShaderResource | StreamOutput | RenderTarget | DepthStencil | UnorderedAccess | IndirectDrawArguments | InputAttachment | RayTracing | ShadingRate, ///< All bind flags.

    Default = None
  };

  struct Bits
  {
    StorageType VertexBuffer : 1;
    StorageType IndexBuffer : 1;
    StorageType UniformBuffer : 1;
    StorageType ShaderResource : 1;
    StorageType StreamOutput : 1;
    StorageType RenderTarget : 1;
    StorageType DepthStencil : 1;
    StorageType UnorderedAccess : 1;
    StorageType IndirectDrawArguments : 1;
    StorageType InputAttachment : 1;
    StorageType RayTracing : 1;
    StorageType ShadingRate : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALBindFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALBindFlags);

/// [D3D11_USAGE]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage
///
/// This describes the expected resource usage.
/// It generally mirrors the [D3D11_USAGE][] enumeration, which is used to describe the usage for both
/// the buffer and texture descriptions.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALResourceUsage
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Immutable = 0, ///< A resource that can only be read by the GPU. It cannot be written to by the GPU, and cannot be accessed by the CPU.
                   ///  This type of resource must be initialized when it is created, since it cannot be modified after creation.
    Mutable,       ///< A resource that requires read and write access by the GPU and can also be occasionally written to by the CPU.
    Dynamic,       ///< A resource that can be read by the GPU and written to, at least once per frame by the CPU.
    Staging,       ///< A resource that facilitates transferring data between the GPU and CPU.
    Unified,       ///< A resource that resides in a unified memory (eg. memory shared between the CPU and GPU), that can be read and written
                   ///  to by the GPU and can also be directly accessed by the CPU.
                   ///  \remarks An application should check if unified memory is available on the device by checking the device capabilities.
                   ///           If there is no unified memory support, an application should choose another usage type (typically xiiGALResourceUsage::Mutable).
    Sparse,        ///< A resource that can be partially committed to physical memory.

    ENUM_COUNT,

    Default = Mutable
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALResourceUsage);

/// This describes the allowed CPU access mode flags when mapping a resource.
/// This is used by the buffer and texture descriptions to describe the CPU access mode for buffers and textures.
///
/// \note Only resources with xiiGALResourceUsage::Dynamic can be mapped.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCPUAccessFlag
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None  = 0U,         ///< No CPU access.
    Read  = XII_BIT(0), ///< A resource should be mapped for reading.
    Write = XII_BIT(1), ///< A resource should be mapped for writing.

    Default = None
  };

  struct Bits
  {
    StorageType Read : 1;
    StorageType Write : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALCPUAccessFlag);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALCPUAccessFlag);

/// [D3D11_MAP]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_map
///
/// This describes how a mapped resource will be accessed. It generally mirrors the [D3D11_MAP][] enumeration.
/// It is used to describe a texture or buffer mapping type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMapType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Read = 0,  ///< The resource is mapped for reading.
    Write,     ///< The resource is mapped for writing.
    ReadWrite, ///< The resource is mapped for reading and writing.

    ENUM_COUNT,

    Default = Read
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALMapType);

/// This describes special arguments for a map operation. This is used to describe addition map flags
/// when mapping buffers and textures.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMapFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None      = 0U,           ///< No map flag specified.
    DoNotWait = XII_BIT(0),   ///< Specifies that the map operation should not wait until previous command that is using the same resource goes to completion.
                              ///< Map returns a null pointer if the resource is still in use.
    Discard = XII_BIT(1),     ///< Specifies that the previous contents of the resource will be discarded and undefined.
                              ///< This flag is only compatible with xiiGALMapType::Write.
    NoOverWrite = XII_BIT(2), ///< The system will not synchronize pending operations before mapping the buffer.
                              ///< It is the responsibility of the application to ensure that the buffer contents is not overwritten while it is in use by the GPU.

    Default = None
  };

  struct Bits
  {
    StorageType DoNotWait : 1;
    StorageType Discard : 1;
    StorageType NoOverwrite : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALMapFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALMapFlags);

/// This describes the resorurce dimension. This is used by the texture description to describe the texture type,
/// and the texture view description to describe the texture view type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALResourceDimension
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0U,   ///< Undefined resource type.
    Buffer,           ///< Buffer resource type.
    Texture1D,        ///< One-dimensional texture.
    Texture1DArray,   ///< One-dimensional texture array.
    Texture2D,        ///< Two-dimensional texture.
    Texture2DArray,   ///< Two-dimensional texture array.
    Texture3D,        ///< Three-dimension texture.
    TextureCube,      ///< Cube-map texture.
    TextureCubeArray, ///< Cube map array texture.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALResourceDimension);

/// This describes the texture view type used by the texture view description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureViewType
{
  using StorageType = xiiInt8;

  enum Enum : StorageType
  {
    Undefined = -1,       ///< Undefined texture view type.
    ShaderResource,       ///< A texture view will define the shader resource view that will be used as the source for the shader read operations.
    RenderTarget,         ///< A texture view will define a render target view that will be used as the render target for rendering operations.
    DepthStencil,         ///< A texture view will define a depth stencil view that will be used as the target for rendering operations.
    ReadOnlyDepthStencil, ///< A texture view will define a read-only depth stencil view that will be used as depth stencil source for rendering operations, but can also be simultaneously read from shaders.
    UnorderedAccess,      ///< A texture view will define an unordered access view that will be used for unordered read or write operations from the shaders.
    ShadingRate,          ///< A texture view will define a variable shading rate view that will be used as the shading rate source for rendering operations.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALTextureViewType);

/// This describes the buffer view type used by the buffer view description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferViewType
{
  using StorageType = xiiInt8;

  enum Enum : StorageType
  {
    Undefined = -1,  ///< Undefined buffer view type.
    ShaderResource,  ///< A buffer view will define a shader resource view that will be used as the source for the shader read operations.
    UnorderedAccess, ///< A buffer view will define an unordered access view that will be used for unordered read or write operations from the shaders.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALBufferViewType);

/// This describes the available texture formats and generally mirrors the DXGI_FORMAT enumeratinon.
/// The table below provides detailed information on each format. Most of these formats are widely supported by all modern
/// APIs (DX10+, OpenGL3.3+ and OpenGLES3.0+). Specific requirements are additionally indicated.
/// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format">DXGI_FORMAT enumeration on MSDN.</a>,
///     <a href = "https://www.opengl.org/wiki/Image_Format">OpenGL Texture Formats.</a>
struct XII_GRAPHICSFOUNDATION_DLL xiiGALResourceFormat
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    Unknown = 0U,                 ///< Unknown format.
    RGBA32Typeless,               ///< Four component 128-bit typeless format with 32-bit channels.
    RGBA32Float,                  ///< Four-component 128-bit floating-point format with 32-bit channels.
    RGBA32UInt,                   ///< Four-component 128-bit unsigned-integer format with 32-bit channels.
    RGBA32SInt,                   ///< Four-component 128-bit signed-integer format with 32-bit channels.
    RGB32Typeless,                ///< Three-component 96-bit typeless format with 32-bit channels.
    RGB32Float,                   ///< Three-component 96-bit floating-point format with 32-bit channels.
    RGB32UInt,                    ///< Three-component 96-bit unsigned-integer format with 32-bit channels.
    RGB32SInt,                    ///< Three-component 96-bit signed-integer format with 32-bit channels.
    RGBA16Typeless,               ///< Four-component 64-bit typeless format with 16-bit channels.
    RGBA16Float,                  ///< Four-component 64-bit floating-point format with 16-bit channels.
    RGBA16UNormalized,            ///< Four-component 64-bit unsigned-normalized-integer format with 16-bit channels.
    RGBA16UInt,                   ///< Four-component 64-bit unsigned-integer format with 16-bit channels.
    RGBA16SNormalized,            ///< Four-component 64-bit signed-normalized-integer format with 16-bit channels.s
    RGBA16SInt,                   ///< Four-component 64-bit signed-integer format with 16-bit channels.
    RG32Typeless,                 ///< Two-component 64-bit typeless format with 32-bit channels.
    RG32Float,                    ///< Two-component 64-bit floating-point format with 32-bit channels.
    RG32UInt,                     ///< Two-component 64-bit unsigned-integer format with 32-bit channels.
    RG32SInt,                     ///< Two-component 64-bit signed-integer format with 32-bit channels.
    R32G8X24Typeless,             ///< Two-component 64-bit typeless format with 32 bits for the R channel and 8 bits for the G channel.
    D32FloatS8X24UInt,            ///< Two-component 64-bit format with 32-bit floating-point depth channel and 8-bit stencil channel.
    R32FloatX8X24Typeless,        ///< Two-component 64-bit format with 32-bit floating-point R channel ad 8+24 bits of typeless data.
    X32TypelessG8X24UInt,         ///< Two-component 64-bit format with 32-bit typeless data and 8-bit G channel.
    RGB10A2Typeless,              ///< Four-component 32-bit typeles format with 10 bits for the RGB channels and 2 bits for the alpha channel.
    RGB10A2UNormalized,           ///< Four-component 32-bit unsigned-normalized-integer format with 10 bits for the RGB channels and 2 bits for the alpha channel.
    RGB10A2UInt,                  ///< Four-component 32-bit unsigned-integer format with 10 bits for the RGB channels and 2 bits for the alpha channel.
    RG11B10Float,                 ///< Three-component 32-bit format encoding three partial precision channels using 11 bits for the red and green channels, and 10-bits for the blue channel.
    RGBA8Typeless,                ///< Four-component 32-bit typeless format with 8-bit channels.
    RGBA8UNormalized,             ///< Four-component 32-bit unsigned-normalized-integer format with 8-bit channels.
    RGBA8UNormalizedSRGB,         ///< Four-component 32-bit unsigned-normalized-integer sRGB format with 8-bit channels.
    RGBA8UInt,                    ///< Four-component 32-bit unsigned-integer format with 8-bit channels.
    RGBA8SNormalized,             ///< Four-component 32-bit signed-normalized-integer format with 8-bit channels.
    RGBA8SInt,                    ///< Four-component 32-bit signed-integer format with 8-bit channels.
    RG16Typeless,                 ///< Two-component 32-bit typeless format with 16-bit channels.
    RG16Float,                    ///< Two-component 32-bit half-precision floating-point format with 16-bit channels.
    RG16UNormalized,              ///< Two-component 32-bit unsigned-normalized-integer format with 16-bit channels.
    RG16UInt,                     ///< Two-component 32-bit unsigned-integer format with 16-bit channels.
    RG16SNormalized,              ///< Two-component 32-bit signed-normalized-integer format with 16-bit channels.
    RG16SInt,                     ///< Two-component 32-bit signed-integer format with 16-bit channels.
    R32Typeless,                  ///< Single-component 32-bit typeless format.
    D32Float,                     ///< Single-component 32-bit floating-point depth format.
    R32Float,                     ///< Single-component 32-bit floating-point format.
    R32UInt,                      ///< Single-component 32-bit unsigned-integer format.
    R32SInt,                      ///< Single-component 32-bit signed-integer format.
    R24G8Typeless,                ///< Two component 32-bit format with 24 bits for the R channel and 8 bits for the G channel.
    D24UNormalizedS8UInt,         ///< Two component 32-bit format with 24 bits for the unsigned-normalized-integer depth and 8 bits for the stencil.
    R24UNormalizedX8Typeless,     ///< Two component 32-bit format with 24 bits for the unsigned-normalized-integer data and 8 bits of unreferenced data.
    X24TypelessG8UInt,            ///< Two-component 32-bit format with 24 bits of unreferenced data and 8 bits of unsigned-integer data.
    RG8Typeless,                  ///< Two-component 16-bit typeless format with 8-bit channels.
    RG8UNormalized,               ///< Two-component 16-bit unsigned-normalized-integer format with 8-bit channels.
    RG8UInt,                      ///< Two-component 16-bit unsigned-integer format with 8-bit channels.
    RG8SNormalized,               ///< Two-component 16-bit signed-normalized-integer format with 8-bit channels.
    RG8SInt,                      ///< Two-component 16-bit signed-integer format with 8-bit channels.
    R16Typeless,                  ///< Single-component 16-bit typeless format.
    R16Float,                     ///< Single-component 16-bit half-precision floating-point format.
    D16UNormalized,               ///< Single-component 16-bit unsigned-normalized-integer depth format.
    R16UNormalized,               ///< Single-component 16-bit unsigned-normalized-integer format.
    R16UInt,                      ///< Single-component 16-bit unsigned-integer format.
    R16SNormalized,               ///< Single-component 16-bit signed-normalized-integer format.
    R16SInt,                      ///< Single-component 16-bit signed-integer format.
    R8Typeless,                   ///< Single-component 8-bit typeless format.
    R8UNormalized,                ///< Single-component 8-bit unsigned-normalized-integer format.
    R8UInt,                       ///< Single-component 8-bit unsigned-integer format.
    R8SNormalized,                ///< Single-component 8-bit signed-normalized-integer format.
    R8SInt,                       ///< Single-component 8-bit signed-integer-format format.
    A8UNormalized,                ///< Single-component 8-bit unsigned-normalized-integer format for alpha channel only.
    R1UNormalized,                ///< Single-component 1-bit format.
    RGB9E5SharedExponent,         ///< Three partial-precision floating-point numbers sharing single exponent encoded into a 32-bit value.
    RG8BG8UNormalized,            ///< Four-component unsigned-normalized-integer format analogous to UYVY encoding.
    GR8GB8UNormalized,            ///< Four-component unsigned-normalized-integer format analogous to YUY2 encoding.
    BC1Typeless,                  ///< Four-component typeless block-compression format with 1:8 compression ratio.
    BC1UNormalized,               ///< Four-component unsigned-normalized-integer block-compression format with 5 bits for R, 6 bits for G, 5 bits for B, and 0 or 1 bit for A channels. The pixel data is encoded using 8 bytes per 4x4 block (4 bits per pixel) providing 1:8 compression ratio against RGBA8 format.
    BC1UNormalizedSRGB,           ///< Four-component unsigned-normalized-integer block-compression sRGB format with 5 bits for R, 6 bits for G, 5 bits for B, and 0 or 1 bit for A channels.  The pixel data is encoded using 8 bytes per 4x4 block (4 bits per pixel) providing 1:8 compression ratio against RGBA8 format.
    BC2Typeless,                  ///< Four-component typeless block-compression format with 1:4 compression ratio.
    BC2UNormalized,               ///< Four-component unsigned-normalized-integer block-compression format with 5 bits for R, 6 bits for G, 5 bits for B channels and 4 bits for low-coherent separate A channel. The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:4 compression ratio against RGBA8 format.
    BC2UNormalizedSRGB,           ///< Four-component signed-normalized-integer block-compression sRGB format with 5 bits for R, 6 bits for G, 5 bits for B channels, and 4 bits for low-coherent separate A channel. The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:4 compression ratio against RGBA8 format.
    BC3Typeless,                  ///< Four-component typeless block-compression format with 1:4 compression ratio.
    BC3UNormalized,               ///< Four-component unsigned-normalized-integer block-compression format with 5 bits for R, 6 bits for G, 5 bits for B channels, and 8 bits for highly-coherent A channel. The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:4 compression ratio against RGBA8 format.
    BC3UNormalizedSRGB,           ///< Four-component unsigned-normalized-integer block-compression sRGB format with 5 bits for R, 6 bits for G, 5 bits for B, and 8 bits for highly-coherent A channel. The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:4 compression ratio against RGBA8 format.
    BC4Typeless,                  ///< One-component typeless block-compression format with 1:2 compression ratio.
    BC4UNormalized,               ///< One-component unsigned-normalized-integer block-compression format with 8 bits for R channel. The pixel data is encoded using 8 bytes per 4x4 block (4 bits per pixel) providing 1:2 compression ratio against R8 format.
    BC4SNormalized,               ///< One-component signed-normalized-integer block-compression format with 8 bits for R channel. The pixel data is encoded using 8 bytes per 4x4 block (4 bits per pixel) providing 1:2 compression ratio against R8 format.
    BC5Typeless,                  ///< Two-component typeless block-compression format with 1:2 compression ratio.
    BC5UNormalized,               ///< Two-component unsigned-normalized-integer block-compression format with 8 bits for R and 8 bits for G channel. The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:2 compression ratio against RG8 format.
    BC5SNormalized,               ///< Two-component signed-normalized-integer block-compression format with 8 bits for R and 8 bits for G channel. The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:2 compression ratio against RG8 format.
    B5G6R5UNormalized,            ///< Three-component 16-bit unsigned-normalized-integer format with 5 bits for B, 6 bits for G, and 5 bits for R channels.
    B5G5R5A1UNormalized,          ///< Four-component 16-bit unsigned-normalized-integer format with 5 bits for each color channel and a 1-bit alpha channel.
    BGRA8UNormalized,             ///< Four-component 32-bit unsigned-normalized-integer format with 8 bits for each channel.
    BGRX8UNormalized,             ///< Four-component 32-bit unsigned-normalized-integer format with 8 bits for each color channel and 8 bits unused.
    R10G10B10XRBiasA2UNormalized, ///< Four-component 32-bit 2.8-biased fixed-point format with 10 bits for each color channel and a 2-bit alpha channel.
    BGRA8Typeless,                ///< Four-component 32-bit typeless format with 8 bits for each channel.
    BGRA8UNormalizedSRGB,         ///< Four-component 32-bit unsigned-normalized sRGB format with 8 bits for each channel.
    BGRX8Typeless,                ///< Four-component 32-bit typeless format that with 8 bits for each color channel, and 8 bits are unused.
    BGRX8UNormalizedSRGB,         ///< Four-component 32-bit unsigned-normalized sRGB format with 8 bits for each color channel, and 8 bits are unused.
    BC6HTypeless,                 ///< Three-component typeless block-compression format.
    BC6HUF16,                     ///< Three-component unsigned half-precision floating-point format with 16 bits for each channel.
    BC6HSF16,                     ///< Three-channel signed half-precision floating-point format with 16 bits per each channel.
    BC7Typeless,                  ///< Four-component typeless block-compression format.
    BC7UNormalized,               ///< Four-component block-compression unsigned-normalized-integer format with 4 to 7 bits per color channel and 0 to 8 bits of the alpha channel.
    BC7UNormalizedSRGB,           ///< Four-component block-compression unsigned-normalized-integer sRGB format with 4 to 7 bits per color channel and 0 to 8 bits of the alpha channel.
    NV12,                         ///< A multi-planar YUV 4:2:0 format with an 8-bit Y (luminance) plane followed by an interleaved 8-bit-per-channel UV plane at half width and half height. Common hardware decode output. Not usable as a render target, but can be sampled as a shader resource.
    P010,                         ///< A multi-planar YUV 4:2:0 format with a 10-bit Y plane (stored in 16 bits per sample) followed by an interleaved 10-bit-per-channel UV plane (stored in 16 bits per channel) at half resolution. Used for HDR video. Not usable as a render target, but can be sampled as a shader resource.
    P016,                         ///< A multi-planar YUV 4:2:0 format with a 16-bit Y plane followed by an interleaved 16-bit-per-channel UV plane at half resolution. Used for high-quality video pipelines. Not usable as a render target, but can be sampled as a shader resource.
    YUY2,                         ///< A packed YUV 4:2:2 format with 8-bit samples arranged as Y0 U0 Y1 V0. Not usable as a render target, but can be sampled as a shader resource.
    AYUV,                         ///< A packed YUV 4:4:4 format with 8-bit A, Y, U, and V channels. Not usable as a render target, but can be sampled as a shader resource.
    P216,                         ///< A multi-planar YUV 4:2:2 format with a 16-bit Y plane followed by an interleaved 16-bit-per-channel UV plane at half horizontal resolution. Not usable as a render target, but can be sampled as a shader resource.
    P416,                         ///< A multi-planar YUV 4:4:4 format with a 16-bit Y plane followed by separate 16-bit U and V planes. Not usable as a render target, but can be sampled as a shader resource.

    ENUM_COUNT,

    Default = Unknown
  };

  /// Returns whether the given texture format is a depth format.
  XII_ALWAYS_INLINE static bool IsDepthFormat(xiiGALResourceFormat::Enum format) { return format == D16UNormalized || format == D24UNormalizedS8UInt || format == D32Float || format == D32FloatS8X24UInt; }

  /// Returns whether the given texture format is a stencil format
  XII_ALWAYS_INLINE static bool IsStencilFormat(xiiGALResourceFormat::Enum format) { return format == D24UNormalizedS8UInt || format == D32FloatS8X24UInt; }

  /// Returns whether the given texture format is a sRGB format.
  XII_ALWAYS_INLINE static bool IsSrgb(xiiGALResourceFormat::Enum format) { return format == RGBA8UNormalizedSRGB || format == BGRX8UNormalizedSRGB || format == BGRA8UNormalizedSRGB || format == BC1UNormalizedSRGB || format == BC2UNormalizedSRGB || format == BC3UNormalizedSRGB || format == BC7UNormalizedSRGB; }

  /// Returns true if the given texture format is a typeless format, otherwise returns false.
  XII_ALWAYS_INLINE static bool IsTypeless(xiiGALResourceFormat::Enum format)
  {
    switch (format)
    {
      case RGBA32Typeless:
      case RGB32Typeless:
      case RGBA16Typeless:
      case RG32Typeless:
      case R32G8X24Typeless:
      case RGB10A2Typeless:
      case RGBA8Typeless:
      case RG16Typeless:
      case R32Typeless:
      case R24G8Typeless:
      case RG8Typeless:
      case R16Typeless:
      case R8Typeless:
      case BC1Typeless:
      case BC2Typeless:
      case BC3Typeless:
      case BC4Typeless:
      case BC5Typeless:
      case BGRA8Typeless:
      case BGRX8Typeless:
      case BC6HTypeless:
      case BC7Typeless:
        return true;
      default:
        return false;
    }
  }

  /// Returns true if the given texture format is a multi-planar format, otherwise returns false.
  XII_ALWAYS_INLINE static bool IsMultiplanar(xiiGALResourceFormat::Enum format)
  {
    switch (format)
    {
      case NV12:
      case P010:
      case P016:
      case P216:
      case P416:
        return true;
      default:
        return false;
    }
  }

  /// Returns the linear (non-sRGB) version of the given format if it exists, otherwise returns the given format.
  XII_ALWAYS_INLINE static xiiGALResourceFormat::Enum AsLinear(xiiGALResourceFormat::Enum format)
  {
    switch (format)
    {
      case RGBA8UNormalizedSRGB:
        return RGBA8UNormalized;
      case BGRA8UNormalizedSRGB:
        return BGRA8UNormalized;
      case BGRX8UNormalizedSRGB:
        return BGRX8UNormalized;
      case BC1UNormalizedSRGB:
        return BC1UNormalized;
      case BC2UNormalizedSRGB:
        return BC2UNormalized;
      case BC3UNormalizedSRGB:
        return BC3UNormalized;
      case BC7UNormalizedSRGB:
        return BC7UNormalized;
      default:
        return format;
    }
  }

  /// Returns the sRGB version of the given format if it exists, otherwise returns the given format.
  XII_ALWAYS_INLINE static xiiGALResourceFormat::Enum AsSrgb(xiiGALResourceFormat::Enum format)
  {
    switch (format)
    {
      case RGBA8UNormalized:
        return RGBA8UNormalizedSRGB;
      case BGRA8UNormalized:
        return BGRA8UNormalizedSRGB;
      case BGRX8UNormalized:
        return BGRX8UNormalizedSRGB;
      case BC1UNormalized:
        return BC1UNormalizedSRGB;
      case BC2UNormalized:
        return BC2UNormalizedSRGB;
      case BC3UNormalized:
        return BC3UNormalizedSRGB;
      case BC7UNormalized:
        return BC7UNormalizedSRGB;
      default:
        return format;
    }
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALResourceFormat);

/// This describes the filter type.
///
/// \note On D3D11, comparison filters only work with textures that have the following formats
/// R32_FLOAT_X8X24_TYPELESS, R32_FLOAT, R24_UNORM_X8_TYPELESS, R16_UNORM.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALFilterType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U,          ///< Unknown filter type.
    Point,                 ///< Point filtering.
    Linear,                ///< Linear filtering.
    Anisotropic,           ///< Anisotropic filtering.
    ComparisonPoint,       ///< Comparison-point filtering.
    ComparisonLinear,      ///< Comparison-linear filtering.
    ComparisonAnisotropic, ///< Comparison-anisotropic filtering.
    MinimumPoint,          ///< Minimum-point filtering (D3D12 Specific)
    MinimumLinear,         ///< Minimum-linear filtering (D3D12 Specific)
    MinimumAnisotropic,    ///< Minimum-anisotropic filtering (D3D12 Specific)
    MaximumPoint,          ///< Maximum-point filtering (D3D12 Specific)
    MaximumLinear,         ///< Maximum-linear filtering (D3D12 Specific)
    MaximumAnisotropic,    ///< Maximum-anisotropic filtering (D3D12 Specific)

    ENUM_COUNT,

    Default = Unknown
  };

  /// Returns true if the given filter type is a comparison filter type, else false.
  XII_ALWAYS_INLINE static bool IsComparisonFilter(xiiGALFilterType::Enum e) { return e == ComparisonPoint || e == ComparisonLinear || e == ComparisonAnisotropic; }

  /// Returns true if the given filter type is an anisotropic filter type, else false.
  XII_ALWAYS_INLINE static bool IsAnisotropicFilter(xiiGALFilterType::Enum e) { return e == Anisotropic || e == ComparisonAnisotropic || e == MinimumAnisotropic || e == MaximumAnisotropic; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALFilterType);

/// [D3D11_TEXTURE_ADDRESS_MODE]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_texture_address_mode
/// [D3D12_TEXTURE_ADDRESS_MODE]: https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_texture_address_mode
///
/// This describes the texture address mode. It defines a technique for resolving texture coordinates that
/// are outside  of the boundries of a texture. The enumeration generally mirrors [D3D11_TEXTURE_ADDRESS_MODE][]/[D3D12_TEXTURE_ADDRESS_MODE][] enumeration.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureAddressMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U, ///< Unknown texture address mode.
    Wrap,         ///< Tile the texture at every integer junction.
    Mirror,       ///< Flip the texture at every integer junctions.
    Clamp,        ///< Texture coordinates outside the range [0.0, 1.0] are set to the texture color at 0.0 or 1.0 respectively.
    Border,       ///< Texture coordinates outside the range [0.0, 1.0] are set to the border color.
    MirrorOnce,   ///< Similar to Mirror and Clamp. This takes the absolute value of the texture coordinate (thus mirroring around 0), then clamps to the the maximum value.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALTextureAddressMode);

/// [D3D11_COMPARISON_FUNC]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_comparison_func
/// [D3D12_COMPARISON_FUNC]: https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_comparison_func
///
/// This describes a comparison function.
/// This enumeration defines a comparison function. It generally mirrors [D3D11_COMPARISON_FUNC]/[D3D12_COMPARISON_FUNC] enumeration.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALComparisonFunction
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U, ///< Unknown comparison function.
    Never,        ///< Comparison never passes.
    Less,         ///< Comparison passes if the source data is less than the destination data.
    Equal,        ///< Comparison passes if the source data is equal to the destination data.
    LessEqual,    ///< Comparison passes if the source data is less than or equal to the destination data.
    Greater,      ///< Comparison passes if the source data is greater than the destination data.
    NotEqual,     ///< Comparison passes if the source data is not equal to the destination data.
    GreaterEqual, ///< Comparison passes if the source data is greater than or equal to the destination data.
    Always,       ///< Comparison always passes.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALComparisonFunction);

/// This describes the topology of how vertices are interpreted by the pipeline.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPrimitiveTopology
{
  using StorageType = xiiInt8;

  enum Enum : StorageType
  {
    Undefined = -1,          ///< Undefined topology.
    PointList,               ///< Interpret the vertex data as a list of points.
    LineList,                ///< Interpret the vertex data as a list of lines.
    TriangleList,            ///< Interpret the vertex data as a list of triangles.
    TriangleStrip,           ///< Interpret the vertex data as a triangle strip.
    LineStrip,               ///< Interpret the vertex data as a line strip.
    TriangleListAdjacent,    ///< Interpret the vertex data as a list of triangles with adjacency data.
    TriangleStripAdjacent,   ///< Interpret the vertex data as a triangle strip with adjacency data.
    LineListAdjacent,        ///< Interpret the vertex data as a list of lines with adjacency data.
    LineStripAdjacent,       ///< Interpret the vertex data as a line strip with adjacency data.
    ControlPointPatchList1,  ///< Interpret the vertex data as a list of one control point patches.
    ControlPointPatchList2,  ///< Interpret the vertex data as a list of two control point patches.
    ControlPointPatchList3,  ///< Interpret the vertex data as a list of three control point patches.
    ControlPointPatchList4,  ///< Interpret the vertex data as a list of four control point patches.
    ControlPointPatchList5,  ///< Interpret the vertex data as a list of five control point patches.
    ControlPointPatchList6,  ///< Interpret the vertex data as a list of six control point patches.
    ControlPointPatchList7,  ///< Interpret the vertex data as a list of seven control point patches.
    ControlPointPatchList8,  ///< Interpret the vertex data as a list of eight control point patches.
    ControlPointPatchList9,  ///< Interpret the vertex data as a list of nine control point patches.
    ControlPointPatchList10, ///< Interpret the vertex data as a list of ten control point patches.
    ControlPointPatchList11, ///< Interpret the vertex data as a list of 11 control point patches.
    ControlPointPatchList12, ///< Interpret the vertex data as a list of 12 control point patches.
    ControlPointPatchList13, ///< Interpret the vertex data as a list of 13 control point patches.
    ControlPointPatchList14, ///< Interpret the vertex data as a list of 14 control point patches.
    ControlPointPatchList15, ///< Interpret the vertex data as a list of 15 control point patches.
    ControlPointPatchList16, ///< Interpret the vertex data as a list of 16 control point patches.
    ControlPointPatchList17, ///< Interpret the vertex data as a list of 17 control point patches.
    ControlPointPatchList18, ///< Interpret the vertex data as a list of 18 control point patches.
    ControlPointPatchList19, ///< Interpret the vertex data as a list of 19 control point patches.
    ControlPointPatchList20, ///< Interpret the vertex data as a list of 20 control point patches.
    ControlPointPatchList21, ///< Interpret the vertex data as a list of 21 control point patches.
    ControlPointPatchList22, ///< Interpret the vertex data as a list of 22 control point patches.
    ControlPointPatchList23, ///< Interpret the vertex data as a list of 23 control point patches.
    ControlPointPatchList24, ///< Interpret the vertex data as a list of 24 control point patches.
    ControlPointPatchList25, ///< Interpret the vertex data as a list of 25 control point patches.
    ControlPointPatchList26, ///< Interpret the vertex data as a list of 26 control point patches.
    ControlPointPatchList27, ///< Interpret the vertex data as a list of 27 control point patches.
    ControlPointPatchList28, ///< Interpret the vertex data as a list of 28 control point patches.
    ControlPointPatchList29, ///< Interpret the vertex data as a list of 29 control point patches.
    ControlPointPatchList30, ///< Interpret the vertex data as a list of 30 control point patches.
    ControlPointPatchList31, ///< Interpret the vertex data as a list of 31 control point patches.
    ControlPointPatchList32, ///< Interpret the vertex data as a list of 32 control point patches.

    ENUM_COUNT,

    Default = Undefined
  };

  XII_ALWAYS_INLINE static xiiUInt32 VerticesPerPrimitive(xiiGALPrimitiveTopology::Enum e)
  {
    switch (e)
    {
      case xiiGALPrimitiveTopology::TriangleList:
        return 3;
      case xiiGALPrimitiveTopology::TriangleStrip:
        return 3;
      case xiiGALPrimitiveTopology::PointList:
        return 1;
      case xiiGALPrimitiveTopology::LineList:
        return 2;
      case xiiGALPrimitiveTopology::LineStrip:
        return 2;
      case xiiGALPrimitiveTopology::TriangleListAdjacent:
        return 6;
      case xiiGALPrimitiveTopology::TriangleStripAdjacent:
        return 6;
      case xiiGALPrimitiveTopology::LineListAdjacent:
        return 4;
      case xiiGALPrimitiveTopology::LineStripAdjacent:
        return 4;
      case xiiGALPrimitiveTopology::ControlPointPatchList1:
        return 1;
      case xiiGALPrimitiveTopology::ControlPointPatchList2:
        return 4;
      case xiiGALPrimitiveTopology::ControlPointPatchList3:
        return 9;
      case xiiGALPrimitiveTopology::ControlPointPatchList4:
        return 16;
      case xiiGALPrimitiveTopology::ControlPointPatchList5:
        return 25;
      case xiiGALPrimitiveTopology::ControlPointPatchList6:
        return 36;
      case xiiGALPrimitiveTopology::ControlPointPatchList7:
        return 49;
      case xiiGALPrimitiveTopology::ControlPointPatchList8:
        return 64;
      case xiiGALPrimitiveTopology::ControlPointPatchList9:
        return 81;
      case xiiGALPrimitiveTopology::ControlPointPatchList10:
        return 100;
      case xiiGALPrimitiveTopology::ControlPointPatchList11:
        return 121;
      case xiiGALPrimitiveTopology::ControlPointPatchList12:
        return 144;
      case xiiGALPrimitiveTopology::ControlPointPatchList13:
        return 169;
      case xiiGALPrimitiveTopology::ControlPointPatchList14:
        return 196;
      case xiiGALPrimitiveTopology::ControlPointPatchList15:
        return 225;
      case xiiGALPrimitiveTopology::ControlPointPatchList16:
        return 256;
      case xiiGALPrimitiveTopology::ControlPointPatchList17:
        return 289;
      case xiiGALPrimitiveTopology::ControlPointPatchList18:
        return 324;
      case xiiGALPrimitiveTopology::ControlPointPatchList19:
        return 361;
      case xiiGALPrimitiveTopology::ControlPointPatchList20:
        return 400;
      case xiiGALPrimitiveTopology::ControlPointPatchList21:
        return 441;
      case xiiGALPrimitiveTopology::ControlPointPatchList22:
        return 484;
      case xiiGALPrimitiveTopology::ControlPointPatchList23:
        return 529;
      case xiiGALPrimitiveTopology::ControlPointPatchList24:
        return 576;
      case xiiGALPrimitiveTopology::ControlPointPatchList25:
        return 625;
      case xiiGALPrimitiveTopology::ControlPointPatchList26:
        return 676;
      case xiiGALPrimitiveTopology::ControlPointPatchList27:
        return 729;
      case xiiGALPrimitiveTopology::ControlPointPatchList28:
        return 784;
      case xiiGALPrimitiveTopology::ControlPointPatchList29:
        return 841;
      case xiiGALPrimitiveTopology::ControlPointPatchList30:
        return 900;
      case xiiGALPrimitiveTopology::ControlPointPatchList31:
        return 961;
      case xiiGALPrimitiveTopology::ControlPointPatchList32:
        return 1024;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
    return 0;
  }

  XII_ALWAYS_INLINE static xiiUInt32 GetIndexCount(xiiGALPrimitiveTopology::Enum e, xiiUInt32 uiPrimitiveCount)
  {
    if (uiPrimitiveCount == 0)
      return 0;

    if (e >= xiiGALPrimitiveTopology::ControlPointPatchList1 && e <= xiiGALPrimitiveTopology::ControlPointPatchList32)
    {
      xiiUInt32 uiVerticesPerPrimitive = VerticesPerPrimitive(e);
      return uiPrimitiveCount * uiVerticesPerPrimitive; // Each primitive requires 'uiVerticesPerPrimitive' indices.
    }

    switch (e)
    {
      case xiiGALPrimitiveTopology::PointList:
        return uiPrimitiveCount; // 1 index per primitive.

      case xiiGALPrimitiveTopology::LineList:
      case xiiGALPrimitiveTopology::LineStrip:
        return uiPrimitiveCount * 2; // 2 indices per primitive.

      case xiiGALPrimitiveTopology::TriangleList:
        return uiPrimitiveCount * 3; // 3 indices per primitive.

      case xiiGALPrimitiveTopology::TriangleStrip:
        return uiPrimitiveCount + 2; // First two indices are shared.

      case xiiGALPrimitiveTopology::TriangleListAdjacent:
        return uiPrimitiveCount * 6; // 6 indices per primitive.

      case xiiGALPrimitiveTopology::TriangleStripAdjacent:
        return uiPrimitiveCount + 4; // Adjusted for adjacency.

      case xiiGALPrimitiveTopology::LineListAdjacent:
      case xiiGALPrimitiveTopology::LineStripAdjacent:
        return uiPrimitiveCount * 4; // 4 indices per primitive.

      default:
        return 0;
    }
  }

  static const char* Names[ENUM_COUNT];
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALPrimitiveTopology);

/// This describes memory property flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMemoryPropertyFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown      = 0U,         ///< The memory properties are unknown.
    HostCoherent = XII_BIT(0), ///< The device (GPU) memory is coherent with the host (CPU), meaning that CPU writes are automatically available to the GPU and vice versa. If memory is not coherent, it must be explicitly flushed after being modified by the CPU, or invalidated before being read by the CPU.

    Default = Unknown
  };

  struct Bits
  {
    StorageType HostCoherent : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALMemoryPropertyFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALMemoryPropertyFlags);

/// This describes the hardware adapter type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceAdapterType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U, ///< Unknown adapter type.
    Software,     ///< Software adapter.
    Integrated,   ///< Integrated hardware adapter.
    Discrete,     ///< Discrete hardware adapter.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALDeviceAdapterType);

/// This describes the method the raster uses to create an image on a surface.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSwapChainUsageFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None            = 0U,         ///< No allowed usage.
    RenderTarget    = XII_BIT(0), ///< The swap chain images can be used as render target outputs.
    ShaderResource  = XII_BIT(1), ///< The swap chain images can be used as shader resources.
    InputAttachment = XII_BIT(2), ///< The swap chain images can be used as input attachments.
    CopySource      = XII_BIT(3), ///< The swap chain images can be used as the source of a copy operation.

    Default = None
  };

  struct Bits
  {
    StorageType RenderTarget : 1;
    StorageType ShaderResource : 1;
    StorageType InputAttachment : 1;
    StorageType CopySource : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALSwapChainUsageFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALSwapChainUsageFlags);

/// This describes the transform applied to the image content prior to presentation.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSurfaceTransform
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Optimal = 0U,              ///< Use the most optimal surface transform.
    Identity,                  ///< The image content is presented without being transformed.
    Rotate90,                  ///< The image content is rotated 90 degrees clockwise.
    Rotate180,                 ///< The image content is rotated 180 degrees clockwise.
    Rotate270,                 ///< The image content is rotated 270 degrees clockwise.
    HorizontalMirror,          ///< The image is mirrored horizontally.
    HorizontalMirrorRotate90,  ///< The image is mirrored horizontally, then rotated 90 degrees clockwise.
    HorizontalMirrorRotate180, ///< The image is mirrored horizontally, then rotated 180 degrees clockwise.
    HorizontalMirrorRotate270, ///< The image is mirrored horizontally, then rotated 270 degrees clockwise.

    ENUM_COUNT,

    Default = Optimal
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALSurfaceTransform);

/// This describes a query type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALQueryType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0U,     ///< The query type is undefined.
    Occlusion,          ///< Number of samples that passed the depth and stencil test between begin and end (on a comand list).
    BinaryOcclusion,    ///< Acts like Occlusion. Returns true if at least one sample passed.
    Timestamp,          ///< Requests the GPU timestamp, similar to an EndQuery call.
    PipelineStatistics, ///< Gets the pipeline statistics such as the number of pixel shader invocations.
    Duration,           ///< Gets the number of high-frequency counter ticks between BeginQuery and EndQuery calls.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALQueryType);

/// This describes the wave feature types.
///
/// In Vulkan backend, you should check which features are supported by device.
/// In DirectX12 backend, all shader model 6.0 wave functions are supported if WaveOp feature is enabled.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALWaveFeature
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    Unknown         = 0U,         ///<
    Basic           = XII_BIT(0), ///<
    Vote            = XII_BIT(1), ///<
    Arithmetic      = XII_BIT(2), ///<
    Ballot          = XII_BIT(3), ///<
    Shuffle         = XII_BIT(4), ///<
    ShuffleRelative = XII_BIT(5), ///<
    Clustered       = XII_BIT(6), ///<
    Quad            = XII_BIT(7), ///<

    Default = Unknown
  };

  struct Bits
  {
    StorageType Basic : 1;
    StorageType Vote : 1;
    StorageType Arithmetic : 1;
    StorageType Ballot : 1;
    StorageType Shuffle : 1;
    StorageType ShuffleRelative : 1;
    StorageType Clustered : 1;
    StorageType Quad : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALWaveFeature);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALWaveFeature);

/// This describes the ray tracing capability flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingCapabilityFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None               = 0U,         ///< No ray tracing capabilities.
    StandaloneShaders  = XII_BIT(0), ///< The device supports standalone ray tracing shaders (e.g. ray generation, closest hit, any hit, etc.). When this feature is disabled, inline ray tracing may still be supported where rays can be traced from graphics or compute shaders.
    InlineRayTracing   = XII_BIT(1), ///< The device supports inline ray tracing in graphics or compute shaders.
    IndirectRayTracing = XII_BIT(2), ///< The device supports indirect ray tracing commands.

    Default = None
  };

  struct Bits
  {
    StorageType StandaloneShaders : 1;
    StorageType InlineRayTracing : 1;
    StorageType IndirectRayTracing : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALRayTracingCapabilityFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALRayTracingCapabilityFlags);

/// This describes common validation flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALValidationFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None                  = 0U,         ///< Extra validations are disabled.
    CheckShaderBufferSize = XII_BIT(0), ///< Verify that constant or structured buffer size is not smaller than what is expected by the shader.
                                        ///<
                                        ///< \remarks This flag only has effect in Debug/Development builds. This type of validation is never performed in Shipping builds.
                                        ///<
                                        ///< \note This option is currently supported by Vulkan backend only.

    Default = None
  };

  struct Bits
  {
    StorageType CheckShaderBufferSize : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALValidationFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALValidationFlags);

/// This describes the command queue type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueueFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None          = 0U,                    ///< Queue flags are unknown.
    Transfer      = XII_BIT(0),            ///< Command queue that only supports memory transfer operations.
    Compute       = XII_BIT(1) | Transfer, ///< Command queue that supports compute, ray tracing and transfer commands.
    Graphics      = XII_BIT(2) | Compute,  ///< Command queue that supports graphics, compute, ray tracing and transfer commands.
    SparseBinding = XII_BIT(3),            ///< Command queue that supports sparse binding commands.

    PrimaryType = Transfer | Compute | Graphics, ///< Mask to extract primary command queue type.

    Default = None
  };

  struct Bits
  {
    StorageType Transfer : 1;
    StorageType Compute : 1;
    StorageType Graphics : 1;
    StorageType SparseBinding : 1;

    StorageType PrimaryType : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALCommandQueueFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALCommandQueueFlags);

/// This describes the queue priority.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueuePriority
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U, ///< Queue priority is unknown.
    Low,          ///<
    Medium,       ///<
    High,         ///<
    RealTime,     ///< Additional system privileges required to use this priority, read documentation for specific platform.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALCommandQueuePriority);

/// This describes how shading rates coming from the different sources (base rate, primitive rate and VRS image rate) are combined.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShadingRateCombinerFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    PassThrough      = XII_BIT(0), ///< Returns the original shading rate value.
    CombinerOverride = XII_BIT(1), ///< Returns the new shading rate value.
    CombinerMin      = XII_BIT(2), ///< Returns the minimum shading rate value.
    CombinerMax      = XII_BIT(3), ///< Returns the maximum shading rate value.
    CombinerSum      = XII_BIT(4), ///< Returns the sum of the shading rates.
    CombinerMul      = XII_BIT(5), ///< Returns the product of the shading rates

    Default = PassThrough
  };

  struct Bits
  {
    StorageType PassThrough : 1;
    StorageType CombinerOverride : 1;
    StorageType CombinerMin : 1;
    StorageType CombinerMax : 1;
    StorageType CombinerSum : 1;
    StorageType CombinerMul : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALShadingRateCombinerFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShadingRateCombinerFlags);

/// This describes the shading rate texture format supported by the device.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShadingRateFormat
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U,     ///< Variable shading rate is not supported.
    Palette,          ///< Single channel 8-bit surface that contains shading rate values. Only 2D and 2D array textures with R8UNormalized format are allowed.
    RG8UNormalized,   ///< RG8UNormalized texture that defines the shading rate (0.5, 0.25 etc.). The R channel is used for X axis, G channel is used for Y axis.
    ColumnRowFloat32, ///< This format is only used in Metal when shading rate is defined by column/row rates instead of a texture. The values are 32-bit floating point values in 0 to 1 range (0.5, 0.25 etc.).

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShadingRateFormat);

/// This describes the base shading rate along a horizontal or vertical axis.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShadingRateAxis
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    X1, ///< Default shading rate.
    X2, ///< 2x resolution per axis.
    X4, ///< 4x resolution per axis.

    ENUM_COUNT,

    Default = X1
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShadingRateAxis);

/// This describes the shading rate for both the horizontal and vertical axes.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShadingRateFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    _1X1 = ((xiiGALShadingRateAxis::X1 << XII_GAL_SHADING_RATE_X_SHIFT) | xiiGALShadingRateAxis::X1), ///< Specifies no change to the shading rate.
    _1X2 = ((xiiGALShadingRateAxis::X1 << XII_GAL_SHADING_RATE_X_SHIFT) | xiiGALShadingRateAxis::X2), ///< Specifies default horizontal rate and 1/2 vertical shading rate.
    _1X4 = ((xiiGALShadingRateAxis::X1 << XII_GAL_SHADING_RATE_X_SHIFT) | xiiGALShadingRateAxis::X4), ///< Specifies default horizontal rate and 1/4 vertical shading rate.
    _2X1 = ((xiiGALShadingRateAxis::X2 << XII_GAL_SHADING_RATE_X_SHIFT) | xiiGALShadingRateAxis::X1), ///< Specifies 1/2 horizontal shading rate and default vertical rate.
    _2X2 = ((xiiGALShadingRateAxis::X2 << XII_GAL_SHADING_RATE_X_SHIFT) | xiiGALShadingRateAxis::X2), ///< Specifies 1/2 horizontal and 1/2 vertical shading rate.
    _2X4 = ((xiiGALShadingRateAxis::X2 << XII_GAL_SHADING_RATE_X_SHIFT) | xiiGALShadingRateAxis::X4), ///< Specifies 1/2 horizontal and 1/4 vertical shading rate.
    _4X1 = ((xiiGALShadingRateAxis::X4 << XII_GAL_SHADING_RATE_X_SHIFT) | xiiGALShadingRateAxis::X1), ///< Specifies 1/4 horizontal and default vertical rate.
    _4X2 = ((xiiGALShadingRateAxis::X4 << XII_GAL_SHADING_RATE_X_SHIFT) | xiiGALShadingRateAxis::X2), ///< Specifies 1/4 horizontal and 1/2 vertical rate.
    _4X4 = ((xiiGALShadingRateAxis::X4 << XII_GAL_SHADING_RATE_X_SHIFT) | xiiGALShadingRateAxis::X4), ///< Specifies 1/4 horizontal and 1/4 vertical shading rate.

    Default = _1X1
  };

  struct Bits
  {
    StorageType _1X1 : 1;
    StorageType _1X2 : 1;
    StorageType _1X4 : 1;
    StorageType _2X1 : 1;
    StorageType _2X2 : 1;
    StorageType _2X4 : 1;
    StorageType _4X1 : 1;
    StorageType _4X2 : 1;
    StorageType _4X4 : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALShadingRateFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShadingRateFlags);

/// This describes the sample count.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSampleCount
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None             = 0U,  ///< No samples.
    OneSample        = 1U,  ///< One sample.
    TwoSamples       = 2U,  ///< Two samples.
    FourSamples      = 4U,  ///< Four samples.
    EightSamples     = 8U,  ///< Eight samples.
    SixteenSamples   = 16U, ///< Sixteen samples.
    ThirtyTwoSamples = 32U, ///< Thirty-two samples.
    SixtyFourSamples = 64U, ///< Sixty-four samples.

    AllSamples = (SixtyFourSamples << 1U) - 1U, ///< All possible samples.

    Default = OneSample
  };

  struct Bits
  {
    StorageType OneSample : 1;
    StorageType TwoSamples : 1;
    StorageType FourSamples : 1;
    StorageType EightSamples : 1;
    StorageType SixteenSamples : 1;
    StorageType ThirtyTwoSamples : 1;
    StorageType SixtyFourSamples : 1;
    StorageType AllSamples : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALSampleCount);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALSampleCount);

/// This describes the shading rate capability flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShadingRateCapabilityFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None                              = 0U,          ///< No shading rate capabilities.
    PerDraw                           = XII_BIT(0),  ///< Shading rate can be specified for the whole draw call.
    PerPrimitive                      = XII_BIT(1),  ///< Shading rate can be specified in the vertex shader for each primitive and combined with the base rate.
    TextureBased                      = XII_BIT(2),  ///< Shading rate is specified by a texture, each texel defines a shading rate for the tile.
    SampleMask                        = XII_BIT(3),  ///< Allows to set zero bits in graphics pipeline sample mask with the enabled variable rate shading.
    ShaderSampleMask                  = XII_BIT(4),  ///< Allows to get or set SampleMask in the shader with enabled variable rate shading.
    ShaderDepthStencilWrite           = XII_BIT(5),  ///< Allows to write depth and stencil from the pixel shader.
    PerPrimitiveWithMultipleViewports = XII_BIT(6),  ///< Allows to use per primitive shading rate when multiple viewports are used.
    SameTextureForWholeRenderPass     = XII_BIT(7),  ///< Shading rate attachment for render pass must be the same for all sub passes.
    TextureArray                      = XII_BIT(8),  ///< Allows to use texture 2D array for shading rate.
    ShadingRateShaderInput            = XII_BIT(9),  ///< Allows to read current shading rate in the pixel shader.
    AdditionalInvocations             = XII_BIT(10), ///< Indicates that driver may generate additional fragment shader invocations in order to make transitions between fragment areas with different shading rates more smooth.
    NonSubSampledRenderTarget         = XII_BIT(11), ///< Indicates that there are no additional requirements for render targets that are used in texture-based VRS rendering.
    SubSampledRenderTarget            = XII_BIT(12), ///< Indicates that render targets that are used in texture-based VRS rendering must be created with the subsampled flag.
                                                     ///< Intermediate targets must be scaled to the final resolution in a separate pass.
                                                     ///< If supported, rendering to the subsampled render targets may be more optimal.
                                                     ///<
                                                     ///< \note Both non-subsampled and subsampled modes may be supported by a device.

    Default = None
  };

  struct Bits
  {
    StorageType PerDraw : 1;
    StorageType PerPrimitive : 1;
    StorageType TextureBased : 1;
    StorageType SampleMask : 1;
    StorageType ShaderSampleMask : 1;
    StorageType ShaderDepthStencilWrite : 1;
    StorageType PerPrimitiveWithMultipleViewports : 1;
    StorageType SameTextureForWholeRenderPass : 1;
    StorageType TextureArray : 1;
    StorageType ShadingRateShaderInput : 1;
    StorageType AdditionalInvocations : 1;
    StorageType NonSubSampledRenderTarget : 1;
    StorageType SubSampledRenderTarget : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALShadingRateCapabilityFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShadingRateCapabilityFlags);

/// This describes the access pattern of the shading rate texture.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShadingRateTextureAccess
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U,      ///< Shading rate texture access type is unknown.
    OnGPU,             ///< Shading rate texture is accessed by the GPU when command buffer is executed.
    OnSubmit,          ///< Shading rate texture is accessed by the CPU when command buffer is submitted for execution. An application is not allowed to modify the texture until the command buffer is executed by the GPU. Fences or other synchronization methods must be used to control the access to the texture.
    OnSetRenderTarget, ///< Shading rate texture is accessed by the CPU when the render target is set. An application is not allowed to modify the texture until the command buffer is executed by GPU. Fences or other synchronization methods must be used to control the access to the texture.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShadingRateTextureAccess);

/// This describes the draw command compatibilty flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDrawCommandCapabilityFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None                      = 0U,         ///< No draw command capabilities.
    BaseVertex                = XII_BIT(1), ///< Indicates that device supports non-zero base vertex for an indexed draw.
    DrawIndirect              = XII_BIT(2), ///< Indicates that device supports indirect draw/dispatch commands.
    DrawIndirectFirstInstance = XII_BIT(3), ///< Indicates that first instance location of the indirect draw command can be greater than zero.
    NativeMultiDrawIndirect   = XII_BIT(4), ///< Indicates that device natively supports indirect draw commands with draw count greater than 1. When this flag is not set, the commands will be emulated on the host, which will produce correct results, but will be slower.
    DrawIndirectCounterBuffer = XII_BIT(5), ///< Indicates that indirect and indexed indirect draw commands may take non-null counter buffer. If this flag is not set, the number of draw commands must be specified through the command attributes.

    Default = None
  };

  struct Bits
  {
    StorageType BaseVertex : 1;
    StorageType DrawIndirect : 1;
    StorageType DrawIndirectFirstInstance : 1;
    StorageType NativeMultiDrawIndirect : 1;
    StorageType DrawIndirectCounterBuffer : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALDrawCommandCapabilityFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALDrawCommandCapabilityFlags);

/// Sparse memory capability flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSparseResourceCapabilityFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None                     = 0U,          ///< No sparse resource capabilities.
    ShaderResourceResidency  = XII_BIT(0),  ///< Specifies whether texture operations that return resource residency information are supported in shader code.
    Buffer                   = XII_BIT(1),  ///< Specifies whether the device supports sparse buffers.
    Texture2D                = XII_BIT(2),  ///< Specifies whether the device supports sparse 2D textures with 1 sample per pixel.
    Texture3D                = XII_BIT(3),  ///< Specifies whether the device supports sparse 3D textures.
    Texture2Samples          = XII_BIT(4),  ///< Specifies whether the device supports sparse 2D textures with 2 samples per pixel.
    Texture4Samples          = XII_BIT(5),  ///< Specifies whether the device supports sparse 2D textures with 4 samples per pixel.
    Texture8Samples          = XII_BIT(6),  ///< Specifies whether the device supports sparse 2D textures with 8 samples per pixel.
    Texture16Samples         = XII_BIT(7),  ///< Specifies whether the device supports sparse 2D textures with 16 samples per pixel.
    Aliased                  = XII_BIT(8),  ///< Specifies whether the device can correctly access memory aliased into multiple locations, and reading physical memory from multiple aliased locations will return the same value.
    Standard2DTileShape      = XII_BIT(9),  ///< Specifies whether the device accesses single-sample 2D sparse textures using the standard sparse texture tile shapes.
    Standard2DMSTileShape    = XII_BIT(10), ///< Specifies whether the device accesses multi-sample 2D sparse textures using the standard sparse texture tile shapes.
    Standard3DTileShape      = XII_BIT(11), ///< Specifies whether the device accesses 3D sparse textures using the standard sparse texture tile shapes.
    AlignedMipSize           = XII_BIT(12), ///< Specifies if textures with mip level dimensions that are not integer multiples of the corresponding dimensions of the sparse texture tile may be placed in the mip tail.
    NonResidentStrict        = XII_BIT(13), ///< Specifies whether the device can consistently access non-resident (without bound memory) regions of a resource.
    Texture2DArrayMipTail    = XII_BIT(14), ///< Specifies whether the device supports sparse texture arrays with mip levels whose dimensions are less than the tile size.
    BufferStandardBlock      = XII_BIT(15), ///< Indicates that sparse buffers use the standard block, see SparseResourceProperties::StandardBlockSize.
    NonResidentSafe          = XII_BIT(16), ///< Reads or writes from unbound memory must not cause device removal.
    MixedResourceTypeSupport = XII_BIT(17), ///< Indicates that single device memory object can be used to bind memory for different resource types.

    Default = None
  };

  struct Bits
  {
    StorageType ShaderResourceResidency : 1;
    StorageType Buffer : 1;
    StorageType Texture2D : 1;
    StorageType Texture3D : 1;
    StorageType Texture2Samples : 1;
    StorageType Texture4Samples : 1;
    StorageType Texture8Samples : 1;
    StorageType Texture16Samples : 1;
    StorageType Aliased : 1;
    StorageType Standard2DTileShape : 1;
    StorageType Standard2DMSTileShape : 1;
    StorageType Standard3DTileShape : 1;
    StorageType AlignedMipSize : 1;
    StorageType NonResidentStrict : 1;
    StorageType Texture2DArrayMipTail : 1;
    StorageType BufferStandardBlock : 1;
    StorageType NonResidentSafe : 1;
    StorageType MixedResourceTypeSupport : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALSparseResourceCapabilityFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALSparseResourceCapabilityFlags);

/// This describes the texture format component type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALResourceFormatComponentType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0U,         ///< Undefined component type.
    Float,                  ///< Floating point component type.
    SignedNormalized,       ///< Signed-normalized-integer component type.
    UnsignedNormalized,     ///< Unsigned-normalized-integer component type.
    UnsignedNormalizedSRGB, ///< Unsigned-normalized-integer sRGB component type.
    SignedInteger,          ///< Signed-integer component type.
    UnsignedInteger,        ///< Unsigned-integer component type.
    Depth,                  ///< Depth component type.
    DepthStencil,           ///< Depth-stencil component type.
    Compound,               ///< Compound component type. (eg. RG11B10Float or RGB9E5SharedEXP)
    Compressed,             ///< Compressed component type.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALResourceFormatComponentType);

/// This describes the device support of a particular resource dimension for a given texture format.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALResourceDimensionCapabilityFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None             = 0U,                                                 ///< The device does not support any resources for this format.
    Buffer           = XII_BIT(xiiGALResourceDimension::Buffer),           ///< Indicates if the device supports buffer resources for a particular texture format.
    Texture1D        = XII_BIT(xiiGALResourceDimension::Texture1D),        ///< Indicates if the device supports 1D textures for a particular texture format.
    Texture1DArray   = XII_BIT(xiiGALResourceDimension::Texture1DArray),   ///< Indicates if the device supports 1D texture arrays for a particular texture format.
    Texture2D        = XII_BIT(xiiGALResourceDimension::Texture2D),        ///< Indicates if the device supports 2D textures for a particular texture format.
    Texture2DArray   = XII_BIT(xiiGALResourceDimension::Texture2DArray),   ///< Indicates if the device supports 2D texture arrays for a particular texture format.
    Texture3D        = XII_BIT(xiiGALResourceDimension::Texture3D),        ///< Indicates if the device supports 3D textures for a particular texture format.
    TextureCube      = XII_BIT(xiiGALResourceDimension::TextureCube),      ///< Indicates if the device supports cube textures for a particular texture format.
    TextureCubeArray = XII_BIT(xiiGALResourceDimension::TextureCubeArray), ///< Indicates if the device supports cube texture arrays for a particular texture format.

    Default = None
  };

  struct Bits
  {
    StorageType Buffer : 1;
    StorageType Texture1D : 1;
    StorageType Texture1DArray : 1;
    StorageType Texture2D : 1;
    StorageType Texture2DArray : 1;
    StorageType Texture3D : 1;
    StorageType TextureCube : 1;
    StorageType TextureCubeArray : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALResourceDimensionCapabilityFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALResourceDimensionCapabilityFlags);

/// This describes the sparse texture packing mode.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSparseTextureFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None                 = 0U,         ///< No supported sparse texture packing modes.
    SingleMipTail        = XII_BIT(0), ///< Specifies that the texture uses a single mip tail region for all array layers.
    AlignedMipSize       = XII_BIT(1), ///< Specifies that the first mip level whose dimensions are not integer multiples of the corresponding dimensions of the sparse texture tile begins the mip tail region.
    NonStandardBlockSize = XII_BIT(2), ///< Specifies that the texture uses non-standard sparse texture tile dimensions, and the TileSize values do not match the standard sparse texture tile dimensions.

    Default = None
  };

  struct Bits
  {
    StorageType SingleMipTail : 1;
    StorageType AlignedMipSize : 1;
    StorageType NonStandardBlockSize : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALSparseTextureFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALSparseTextureFlags);

/// This describes the pipeline stage flags.
///
/// These flags mirror [VkPipelineStageFlagBits](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkPipelineStageFlagBits)
/// enum and only have effect in a Vulkan graphics implementation.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineStageFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    Undefined                  = 0U,          ///< Undefined pipeline stage.
    TopOfPipeline              = XII_BIT(0),  ///< The top of the pipeline.
    DrawIndirect               = XII_BIT(1),  ///< The stage of the pipeline where Draw/DispatchIndirect data structures are resolved.
    VertexInput                = XII_BIT(2),  ///< The stage of the pipeline where vertex and index buffers are resolved.
    VertexShader               = XII_BIT(3),  ///< The Vertex shader stage.
    HullShader                 = XII_BIT(4),  ///< The Hull shader stage.
    DomainShader               = XII_BIT(5),  ///< The Domain shader stage.
    GeometryShader             = XII_BIT(6),  ///< The Geometry shader stage.
    PixelShader                = XII_BIT(7),  ///< The Pixel shader stage.
    EarlyFragmentTests         = XII_BIT(8),  ///< The stage of the pipeline where early fragment tests (depth and stencil tests before fragment shading) are performed. This stage also includes subpass load operations for framebuffer attachments with a depth/stencil format.
    LateFragmentTests          = XII_BIT(9),  ///< The stage of the pipeline where late fragment tests (depth and stencil tests after fragment shading) are performed. This stage also includes subpass store operations for framebuffer attachments with a depth/stencil format.
    RenderTarget               = XII_BIT(11), ///< The stage of the pipeline after blending where the final color values are output from the pipeline. This stage also includes subpass load and store operations and multisample resolve operations for framebuffer attachments with a color or depth/stencil format.
    ComputeShader              = XII_BIT(12), ///< The Compute shader stage.
    Transfer                   = XII_BIT(13), ///< The stage where all copy and outside-of-renderpass resolve and clear operations occur.
    BottomOfPipeline           = XII_BIT(14), ///< The bottom of the pipeline.
    Host                       = XII_BIT(15), ///< A pseudo-stage indicating execution on the host of reads/writes of device memory. This stage is not invoked by any commands recorded in a command buffer.
    ConditionalRendering       = XII_BIT(16), ///< The stage of the pipeline where the predicate of conditional rendering are resolved.
    ShadingRateTexture         = XII_BIT(17), ///< The stage of the pipeline where the shading rate texture is read to determine the shading rate for portions of a rasterized primitive.
    RayTracingShader           = XII_BIT(18), ///< The Ray-Tracing shader stage.
    AccelerationStructureBuild = XII_BIT(19), ///< The Acceleration structure build shader.
    TaskShader                 = XII_BIT(21), ///< The Task shader stage.
    MeshShader                 = XII_BIT(22), ///< The Mesh shader stage.
    FragmentDensityProcess     = XII_BIT(23), ///< The stage of the pipeline where the fragment density map is read to generate the fragment areas.
    Default                    = 0x80000000U  ///< Default pipeline stage that is determined by the resource state. For example xiiGALResourceStateFlags::RenderTarget corresponds to xiiGALPipelineStageFlags::RenderTarget.
  };

  struct Bits
  {
    StorageType TopOfPipeline : 1;
    StorageType DrawIndirect : 1;
    StorageType VertexInput : 1;
    StorageType VertexShader : 1;
    StorageType HullShader : 1;
    StorageType DomainShader : 1;
    StorageType GeometryShader : 1;
    StorageType PixelShader : 1;
    StorageType EarlyFragmentTests : 1;
    StorageType LateFragmentTests : 1;
    StorageType RenderTarget : 1;
    StorageType ComputeShader : 1;
    StorageType Transfer : 1;
    StorageType BottomOfPipeline : 1;
    StorageType Host : 1;
    StorageType ConditionalRendering : 1;
    StorageType ShadingRateTexture : 1;
    StorageType RayTracingShader : 1;
    StorageType AccelerationStructureBuild : 1;
    StorageType TaskShader : 1;
    StorageType MeshShader : 1;
    StorageType FragmentDensityProcess : 1;
    StorageType Default : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALPipelineStageFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALPipelineStageFlags);

/// This describes the access flags.
///
/// The flags mirror [VkAccessFlags](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkAccessFlags)
/// enum and only have effect in a Vulkan graphics implementation.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALAccessFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None                       = 0U,          ///< No pipeline access.
    IndirectCommandRead        = XII_BIT(0),  ///< Read access to indirect command data read as part of an indirect drawing or dispatch command.
    IndexRead                  = XII_BIT(1),  ///< Read access to an index buffer as part of an indexed drawing command.
    VertexRead                 = XII_BIT(2),  ///< Read access to a vertex buffer as part of a drawing command.
    UniformRead                = XII_BIT(3),  ///< Read access to uniform buffer.
    InputAttachmentRead        = XII_BIT(4),  ///< Read access to an input attachment within a render pass during fragment shading.
    ShaderRead                 = XII_BIT(5),  ///< Read access from a shader resource, formatted buffer, unordered access view.
    ShaderWrite                = XII_BIT(6),  ///< Write access from a shader resource to an unordered access view.
    RenderTargetRead           = XII_BIT(7),  ///< Read access to a color render target, such as via blending, logic operations, or via certain subpass load operations.
    RenderTargetWrite          = XII_BIT(8),  ///< Write access to a color render target, resolve, or depth/stencil resolve attachment during a render pass or via certain subpass load and store operations.
    DepthStencilRead           = XII_BIT(9),  ///< Read access to a depth/stencil buffer, via depth or stencil operations or via certain subpass load operations.
    DepthStencilWrite          = XII_BIT(10), ///< Write access to a depth/stencil buffer, via depth or stencil operations or via certain subpass load and store operations.
    CopySource                 = XII_BIT(11), ///< Read access to a texture or buffer in a copy operation.
    CopyDestination            = XII_BIT(12), ///< Write access to a texture or buffer in a copy operation.
    HostRead                   = XII_BIT(13), ///< Read access by a host operation. Accesses of this type are not performed through a resource, but directly on memory.
    HostWrite                  = XII_BIT(14), ///< Write access by a host operation. Accesses of this type are not performed through a resource, but directly on memory.
    MemoryRead                 = XII_BIT(15), ///< All read accesses. It is always valid in any access mask, and is treated as equivalent to setting Read access flags that are valid where it is used.
    MemoryWrite                = XII_BIT(16), ///< All write accesses. It is always valid in any access mask, and is treated as equivalent to setting all Write access flags that are valid where it is used.
    ConditionalRenderingRead   = XII_BIT(17), ///< Read access to a predicate as part of conditional rendering.
    ShadingRateTextureRead     = XII_BIT(18), ///< Read access to a shading rate texture as part of a drawing command.
    AccelerationStructureRead  = XII_BIT(19), ///< Read access to an acceleration structure as part of a trace or build command.
    AccelerationStructureWrite = XII_BIT(20), ///< Write access to an acceleration structure or acceleration structure scratch buffer as part of a build command.
    FragmentDensityMapRead     = XII_BIT(21), ///< Read access to a fragment density map attachment during dynamic fragment density map operations.
    Default                    = 0x80000000U  ///< Default access type that is determined by the resource state. For example xiiGALResourceStateFlags::RenderTarget corresponds to xiiGALAccessFlags::RenderTargetWrite.
  };

  struct Bits
  {
    StorageType TopOfPipeline : 1;
    StorageType DrawIndirect : 1;
    StorageType VertexInput : 1;
    StorageType VertexShader : 1;
    StorageType HullShader : 1;
    StorageType DomainShader : 1;
    StorageType GeometryShader : 1;
    StorageType PixelShader : 1;
    StorageType EarlyFragmentTests : 1;
    StorageType LateFragmentTests : 1;
    StorageType RenderTarget : 1;
    StorageType ComputeShader : 1;
    StorageType Transfer : 1;
    StorageType BottomOfPipeline : 1;
    StorageType Host : 1;
    StorageType ConditionalRendering : 1;
    StorageType ShadingRateTexture : 1;
    StorageType RayTracingShader : 1;
    StorageType AccelerationStructureBuild : 1;
    StorageType TaskShader : 1;
    StorageType MeshShader : 1;
    StorageType FragmentDensityProcess : 1;
    StorageType Default : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALAccessFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALAccessFlags);

/// This describes the resource usage state.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALResourceStateFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    Unknown            = 0U,          ///< The resource state is not known directly.
    Undefined          = XII_BIT(0),  ///< The resource state is known to the engine, but is undefined. A resource is typically in an undefined state right after initialization.
    VertexBuffer       = XII_BIT(1),  ///< The resource is accessed as a vertex buffer.
    ConstantBuffer     = XII_BIT(2),  ///< The resource is accessed as a constant (uniform) buffer.
    IndexBuffer        = XII_BIT(3),  ///< The resource is accessed as an index buffer.
    RenderTarget       = XII_BIT(4),  ///< The resource is accessed as a render target.
    UnorderedAccess    = XII_BIT(5),  ///< The resource is used for unordered access.
    DepthWrite         = XII_BIT(6),  ///< The resource is used in a writable depth-stencil view or in clear operation.
    DepthRead          = XII_BIT(7),  ///< The resource is used in a read-only depth-stencil view.
    ShaderResource     = XII_BIT(8),  ///< The resource is accessed from a shader.
    StreamOut          = XII_BIT(9),  ///< The resource is used as the destination for stream output.
    IndirectArgument   = XII_BIT(10), ///< The resource is used as an indirect draw/dispatch arguments buffer.
    CopyDestination    = XII_BIT(11), ///< The resource is used as the destination in a copy operation.
    CopySource         = XII_BIT(12), ///< The resource is used as the source in a copy operation.
    ResolveDestination = XII_BIT(13), ///< The resource is used as the destination in a resolve operation.
    ResolveSource      = XII_BIT(14), ///< The resource is used as the source in a resolve operation.
    InputAttachment    = XII_BIT(15), ///< The resource is used as an input attachment in a render pass sub pass.
    Present            = XII_BIT(16), ///< The resource is used for present.
    BuildASRead        = XII_BIT(17), ///< The resource is used as vertex/index/instance buffer in an amplification shader building operation or as an acceleration structure source in an amplification shader copy operation.
    BuildASWrite       = XII_BIT(18), ///< The resource is used as the target for AS building or AS copy operations.
    RayTracing         = XII_BIT(19), ///< The resource is used as a top-level AS shader resource in a trace rays operation.
    Common             = XII_BIT(20), ///< The resource state is used for read operations, but access to the resource may be slower compared to the specialized state. A transition to the common state is always a pipeline stall and can often induce a cache flush and render target decompress operation.
    ShadingRate        = XII_BIT(21), ///< The resource is used as the source when variable shading rate rendering.

    GenericRead = VertexBuffer | ConstantBuffer | IndexBuffer | ShaderResource | IndirectArgument | CopySource,

    Default = Unknown
  };

  struct Bits
  {
    StorageType Undefined : 1;
    StorageType VertexBuffer : 1;
    StorageType ConstantBuffer : 1;
    StorageType IndexBuffer : 1;
    StorageType RenderTarget : 1;
    StorageType UnorderedAccess : 1;
    StorageType DepthWrite : 1;
    StorageType DepthRead : 1;
    StorageType ShaderResource : 1;
    StorageType StreamOut : 1;
    StorageType IndirectArgument : 1;
    StorageType CopyDestination : 1;
    StorageType CopySource : 1;
    StorageType ResolveDestination : 1;
    StorageType ResolveSource : 1;
    StorageType InputAttachment : 1;
    StorageType Present : 1;
    StorageType BuildASRead : 1;
    StorageType BuildASWrite : 1;
    StorageType RayTracing : 1;
    StorageType Common : 1;
    StorageType ShadingRate : 1;

    StorageType GenericRead : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALResourceStateFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALResourceStateFlags);

/// This describes the swap chain present mode.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPresentMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Immediate = 0U,
    VSync,

    ENUM_COUNT,

    Default = VSync
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALPresentMode);

/// This describes the external memory kind.
///
/// These kinds mirror [VkExternalMemoryFeatureFlagBits](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkExternalMemoryFeatureFlagBits).
///
/// \note Not all external memory kinds may be supported on all platforms. For example, ImportedExportable kind may not be supported on some platforms.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALExternalMemoryKind
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None               = 0U,                    ///< Normal engine-owned memory.
    Imported           = 1U,                    ///< Created from external memory.
    Exportable         = 2U,                    ///< Can be exported to an external handle.
    ImportedExportable = Imported | Exportable, ///< Both imported and exportable.

    ENUM_COUNT,

    Default = None
  };

  struct Bits
  {
    StorageType Imported : 1;
    StorageType Exportable : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALExternalMemoryKind);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALExternalMemoryKind);

/// This describes the external memory usage flags.
///
/// These flags mirror [VkExternalMemoryHandleTypeFlagBits](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkExternalMemoryHandleTypeFlagBits).
///
/// \note Not all flags may be supported for all external memory types. For example, UserPointer external memory type may only support None flag.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALExternalMemoryFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None         = 0U,
    ReadOnly     = XII_BIT(0), ///< The external memory is immutable and will not be written by the GAL.
    WriteOnly    = XII_BIT(1), ///< The external memory may be written by the GAL.
    SharedAccess = XII_BIT(2), ///< The external memory will be accessed by both the GAL and an external producer/consumer.

    Default = None
  };

  struct Bits
  {
    StorageType ReadOnly : 1;
    StorageType WriteOnly : 1;
    StorageType SharedAccess : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALExternalMemoryFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALExternalMemoryFlags);

namespace xiiGAL
{
  struct ModifiedRange
  {
    XII_ALWAYS_INLINE void Reset()
    {
      m_uiMin = xiiInvalidIndex;
      m_uiMax = 0;
    }

    XII_FORCE_INLINE void SetToIncludeValue(xiiUInt32 uiValue)
    {
      m_uiMin = xiiMath::Min(m_uiMin, uiValue);
      m_uiMax = xiiMath::Max(m_uiMax, uiValue);
    }

    XII_FORCE_INLINE void SetToIncludeRange(xiiUInt32 uiMin, xiiUInt32 uiMax)
    {
      m_uiMin = xiiMath::Min(m_uiMin, uiMin);
      m_uiMax = xiiMath::Max(m_uiMax, uiMax);
    }

    XII_FORCE_INLINE bool HasIncludeValue(xiiUInt32 uiValue)
    {
      if (!IsValid())
        return false;

      return uiValue >= m_uiMin && uiValue <= m_uiMax;
    }

    XII_ALWAYS_INLINE bool IsValid() const { return m_uiMin <= m_uiMax; }

    XII_ALWAYS_INLINE xiiUInt32 GetCount() const { return m_uiMax - m_uiMin + 1; }

    xiiUInt32 m_uiMin = xiiInvalidIndex;
    xiiUInt32 m_uiMax = 0;
  };
} // namespace xiiGAL
