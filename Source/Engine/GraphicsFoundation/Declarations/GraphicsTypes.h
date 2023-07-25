#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>

#include <GraphicsFoundation/Declarations/Constants.h>

/// \brief Defines the graphics device.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsDeviceType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0, ///< Undefined graphics device type.
    D3D11,         ///< DirectX 11 graphics device.
    D3D12,         ///< DirectX 12 graphics device.
    Vulkan,        ///< Vulkan graphics device.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALGraphicsDeviceType);

/// \brief This describes the graphics device feature state.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceFeatureState
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Disabled = 0, ///< Device feature is disabled.
    Enabled,      ///< Device feature is enabled. If a feature is requested to be enabled during the initialization but is not supported by the device/driver/platform, the device will fail to be initialized.
    Optional,     ///< Device feature is optional. The device will attempt to enable the feature during initialization. If the feature is not supported by the device/driver/platform, the device will initialize successfully, but the feature will be disabled.
                  ///< The actual feature state can be queried from the device capabilities description.

    ENUM_COUNT,

    Default = Disabled
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALDeviceFeatureState);

/// \brief This describes the represented value type. It is used by the buffer description
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
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALValueType);

/// \brief This describes the shader stage.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderStage
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
    Tile            = XII_BIT(14), ///< Tile shader (Only for metal graphics device).

    ENUM_COUNT,

    AllGraphics   = Vertex | Pixel | Geometry | Hull | Domain,                                        ///< All graphics pipeline shader stages.
    AllMesh       = Amplification | Mesh | Pixel,                                                     ///< All mesh shading pipeline stages.
    AllRayTracing = RayGeneration | RayMiss | RayClosestHit | RayAnyHit | RayIntersection | Callable, ///< All ray-tracing shader stages.

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShaderStage);

/// [D3D11_BIND_FLAG]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_bind_flag
///
/// \brief This describes which parts of the pipeline a resource can be bound to.
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

    ENUM_COUNT,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALBindFlags);

/// [D3D11_USAGE]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage
///
/// This describes the expected resource usage.
/// It generally mirrors the [D3D11_USAGE][] enumeration, which is used to describe the usage for both
/// the buffer and texture descriptions.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALResourceUsage
{
  using StorageType = xiiInt8;

  enum Enum : StorageType
  {
    Immutable = 0U, ///< A resource that can only be read by the GPU. It cannot be written to by the GPU, and cannot be accessed by the CPU.
                    ///< This type of resource must be initialized when it is created, since it cannot be modified after creation.
    Default,        ///< A resource that requires read and write access by the GPU and can also be occasionally written to by the CPU.
    Dynamic,        ///< A resource that can be read by the GPU and written to, at least once per frame by the CPU.
    Staging,        ///< A resource that facilitates transferring data between the GPU and CPU.
    Unified,        ///< A resource that resides in a unified memory (eg. memory shared between the CPU and GPU), that can be read and written
                    ///< to by the GPU and can also be directly accessed by the CPU.
    Sparse,         ///< A resource that can be partially committed to physical memory.

    ENUM_COUNT,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALResourceUsage);

/// \brief This describes the allowed CPU access mode flags when mappoing a resource.
/// This is used by the buffer and texture descriptions to describe the CPU access mode for buffers and textures.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCPUAccessFlag
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None  = 0U,         ///< No CPU access.
    Read  = XII_BIT(0), ///< A resource should be mapped for reading.
    Write = XII_BIT(1), ///< A resource should be mapped for writing.

    ENUM_COUNT,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALCPUAccessFlag);

/// [D3D11_MAP]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_map
///
/// \brief This describes how a mapped resource will be accessed. It generally mirrors the [D3D11_MAP][] enumeration.
/// It is used to describe a texture or buffer mapping type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMapType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Read = 0U, ///< The resource is mapped for reading.
    Write,     ///< The resource is mapped for writing.
    ReadWrite, ///< The resource is mapped for reading and writing.

    ENUM_COUNT,

    Default = Read
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALMapType);

/// \brief This describes special arguments for a map operation. This is used to describe addition map flags
/// when mapping buffers and textures.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMapFlag
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None      = 0U,           ///< No map flag specified.
    DoNotWait = XII_BIT(0),   ///< Specifies that the map operationn should not wait until previous command that is using the same resource goes to completion.
                              ///< Map returns a null pointer if the resource is still in use.
    Discard = XII_BIT(1),     ///< Specifies that the previous contents of the resource will be discarded and undefined.
                              ///< This flag is only compatible with xiiGALMapType::Write.
    NoOverWrite = XII_BIT(2), ///< The system will not synchronize pending operations before mapping the buffer.
                              ///< It is the responsibility of the application to ensure that the buffer contents is not overwritten while it is in use by the GPU.

    ENUM_COUNT,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALMapFlag);

/// \brief This describes the resorurce dimension. This is used by the texture description to describe the texture type,
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

/// \brief This describes the texture view type used by the texture view description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureViewType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0U,  ///< Undefined texture view type.
    ShaderResource,  ///< A texture view will define the shader resource view that will be used as the source for the shader read operations.
    RenderTarget,    ///< A texture view will define a render target view that will be used as the render target for rendering operations.
    UnorderedAccess, ///< A texture view will define an unordered access view that will be used for unordered read or write operations from the shaders.
    ShadingRate,     ///< A texture view will define a variable shading rate view that will be used as the shading rate source for rendering operations.

    ENUN_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALTextureViewType);

/// \brief This describes the buffer view type used by the buffer view description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferViewType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0U,  ///< Undefined buffer view type.
    ShaderResource,  ///< A buffer view will define a shader resource view that will be used as the source for the shader read operations.
    UnorderedAccess, ///< A buffer view will define an unordered access view that will be used for unordered read or write operations from the shaders.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALBufferViewType);

/// \brief This describes the available texture formats and generally mirrors the DXGI_FORMAT enumeratinon.
/// The table below provides detailed information on each format. Most of these formats are widely supported by all modern
/// APIs (DX10+, OpenGL3.3+ and OpenGLES3.0+). Specific requirements are additionally indicated.
/// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format">DXGI_FORMAT enumeration on MSDN.</a>,
///     <a href = "https://www.opengl.org/wiki/Image_Format">OpenGL Texture Formats.</a>
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureFormat
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    Unknown = 0,                  ///< Unknown format.
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
    RG16SNormalized,              ///< Two-component signed-normalized-integer format with 16-bit channels.
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
    R16Float,                     ///< Single-component 16-bit half-precision format.
    D16UNormalized,               ///< Single-component 16-bit unsigned-normalized-integer format.
    R16UNormalized,               ///< Single-component 16-bit unsigned-normalized-integer format.
    R16UInt,                      ///< Single-component 16-bit unsigned-integer format.
    R16SNormalized,               ///< Single-component 16-bit signed-normalized-integer format.
    R16SInt,                      ///< Single-component 16-bit signed-integer format.
    R8Typeless,                   ///< Single-component 8-bit typeless format.
    R8UNormalized,                ///< Single-component 8-bit unsigned-normalized-integer format.
    R8UInt,                       ///< Single-component 8-bit unsigned-integer format.
    R8SNorm,                      ///< Single-component 8-bit signed-normalized-integer format.
    R8SInt,                       ///< Single-component 8-bit signed-integer-format format.
    A8UNormalized,                ///< Single-component 8-bit unsigned-normalized-integer format for alpha channel only.
    R1UNormalized,                ///< Single-component 1-bit format.
    RGB9E5SharedExponent,         ///< Three partial-precision floating-point numbers sharing single exponent encoded into a 32-bit value.
    RG8BG8UNormalized,            ///< Four-component unsigned-normalized-integer format analogous to UYVY encoding.
    RGR8GB8UNormalized,           ///< Four-component unsigned-normalized-integer format analogous to YUY2 encoding.
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
    BC7Typeless,                  ///< Three-component typeless block-compression format.
    BC7UNormalized,               ///< Three-component block-compression unsigned-normalized-integer format with 4 to 7 bits per color channel and 0 to 8 bits of the alpha channel.
    BC7UNormalizedSRGB,           ///< Three-component block-compression unsigned-normalized-integer sRGB format with 4 to 7 bits per color channel and 0 to 8 bits of the alpha channel.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALTextureFormat);

/// \brief This describes the filter type.
///
/// \note On D3D11, comparison filters only work with textures that have the following formats
/// R32_FLOAT_X8X24_TYPELESS, R32_FLOAT, R24_UNORM_X8_TYPELESS, R16_UNORM.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALFilterType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0,           ///< Unknown filter type.
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
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALFilterType);

/// [D3D11_TEXTURE_ADDRESS_MODE]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_texture_address_mode
/// [D3D12_TEXTURE_ADDRESS_MODE]: https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_texture_address_mode
///
/// \brief This describes the texture address mode. It defines a technique for resolving texture coordinates that
/// are outside  of the boundries of a texture. The enumeration generally mirrors [D3D11_TEXTURE_ADDRESS_MODE][]/[D3D12_TEXTURE_ADDRESS_MODE][] enumeration.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureAddressMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0, ///< Unknown texture address mode.
    Wrap,        ///< Tile the texture at every integer junction.
    Mirror,      ///< Flip the texture at every integer junction.s
    Clamp,       ///< Texture coordinates outside the range [0.0, 1.0] are set to the texture color at 0.0 or 1.0 respectively.
    Border,      ///< Texture coordinates outside the range [0.0, 1.0] are set to the border color.
    MirrorOnce,  ///< Similar to Mirror and Clamp. This takes the absolute value of the texture coordinate (thus mirroring around 0), then clamps to the the maximum value.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALTextureAddressMode);

/// [D3D11_COMPARISON_FUNC]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_comparison_func
/// [D3D12_COMPARISON_FUNC]: https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_comparison_func
///
/// \brief This describes a comparison function.
/// This enumeration defines a comparison function. It generally mirrors [D3D11_COMPARISON_FUNC]/[D3D12_COMPARISON_FUNC] enumeration.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALComparisonFunction
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0,  ///< Unknown comparison function.
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

/// \brief This describes the topology of how vertices are interpreted by the pipeline.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPrimitiveTopology
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0,           ///< Undefined topology.
    TriangleList,            ///< Interpret the vertex data as a list of triangles.
    TriangleStrip,           ///< Interpret the vertex data as a triangle strip.
    PointList,               ///< Interpret the vertex data as a list of points.
    LineList,                ///< Interpret the vertex data as a list of lines.
    LineStrip,               ///< Interpret the vertex data as a line strip.
    TriangleListAdjacent,    ///< Interpret the vertex data as a list of triangles with adjacency data.
    TrangleStripAdjacent,    ///< Interpret the vertex data as a triangle strip with adjacency data.
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
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALPrimitiveTopology);

/// \brief This describes memory property flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMemoryProperties
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown      = 0,          ///< The memory properties are unknown.
    HostCoherent = XII_BIT(0), ///< The device (GPU) memory is coherent with the host (CPU), meaning
                               ///< that CPU writes are automatically available to the GPU and vice versa.
                               ///< If memory is not coherent, it must be explicitly flushed after
                               ///< being modified by the CPU, or invalidated before being read by the CPU.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALMemoryProperties);

/// \brief This describes the hardware adapter type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceAdapterType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0, ///< Unknown adapter type.
    Software,    ///< Software adapter.
    Integrated,  ///< Integrated hardware adapter.
    Discrete,    ///< Discrete hardware adapter.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALDeviceAdapterType);

/// \brief This describes how an image is stretched to fit a given monitor's resolution.
/// \sa <a href = "https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/bb173066(v=vs.85)">DXGI_MODE_SCALING enumeration on MSDN</a>,
struct XII_GRAPHICSFOUNDATION_DLL xiiGALScalingMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unspecified = 0, ///< Unspecified scaling.
    Centered,        ///< Specifies no scaling. The image is centered on the display. This flag is typically used for a fixed-dot-pitch display (such as an LED display).
    Stretched,       ///< Specifies a stretched scaling.

    ENUM_COUNT,

    Default = Unspecified
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALScalingMode);

/// \brief This describes the method the raster uses to create an image on a surface.
/// \sa <a href = "https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/bb173067(v=vs.85)">DXGI_MODE_SCANLINE_ORDER enumeration on MSDN</a>,
struct XII_GRAPHICSFOUNDATION_DLL xiiGALScanLineOrder
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unspecified = 0, ///< Unspecified scanline order.
    Progressive,     ///< The image is created from the first scanline to the last without skipping any.
    UpperFieldFirst, ///< The image is created beginning with the upper field.
    LowerFieldFirst, ///< The image is created beginning with the lower field.

    ENUM_COUNT,

    Default = Unspecified
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALScanLineOrder);

/// \brief This describes the method the raster uses to create an image on a surface.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSwapChainUsageFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None            = 0,          ///< No allowed usage.
    RenderTarget    = XII_BIT(0), ///< The swapchain images can be used as render target outputs.
    ShaderResource  = XII_BIT(1), ///< The swapchain images can be used as shader resources.
    InputAttachment = XII_BIT(2), ///< The swapchain images can be used as input attachments.
    CopySource      = XII_BIT(3), ///< The swapchain images can be used as the source of a copy operation.

    ENUM_COUNT = 5,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALSwapChainUsageFlags);

/// \brief This describes the transform applied to the image content prior to presentation.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSurfaceTransform
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Optimal = 0,               ///< Use the most optimal surface transform.
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

/// \brief This describes a query type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALQueryType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0,      ///< The query type is undefined.
    Occlusion,          ///< Number of samples that passed the depth and stencil test between begin and end (on a context).
    BinaryOcclusion,    ///< Acts like Occlusion. Returns true if at least one sample passed.
    Timestamp,          ///< Requests the GPU timestamp, similar to an EndQuery call.
    PipelineStatistics, ///< Gets the pipeline statistics such as the number of pixel shader invocations.
    Duration,           ///< Gets the number of high-frequency counter ticks between BeginQuery and EndQuery calls.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALQueryType);

/// \brief Base class for GAL objects, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class xiiGALObject : public xiiRefCounted
{
public:
  xiiGALObject(CreationDescription& description) :
    m_Description(description)
  {
  }

  XII_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  CreationDescription m_Description;
};

namespace xiiGAL
{
  using xii16_16Id = xiiGenericId<16, 16>;
  using xii18_14Id = xiiGenericId<18, 14>;
  using xii20_12Id = xiiGenericId<20, 12>;
} // namespace xiiGAL

class xiiGALSwapChainHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALSwapChainHandle, xiiGAL::xii16_16Id);

  friend class xiiGALDevice;
};

class xiiGALShaderHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALShaderHandle, xiiGAL::xii18_14Id);

  friend class xiiGALDevice;
};

class xiiGALTextureHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALTextureHandle, xiiGAL::xii18_14Id);

  friend class xiiGALDevice;
};

class xiiGALBufferHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALBufferHandle, xiiGAL::xii18_14Id);

  friend class xiiGALDevice;
};

class xiiGALResourceViewHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALResourceViewHandle, xiiGAL::xii18_14Id);

  friend class xiiGALDevice;
};

class xiiGALUnorderedAccessViewHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALUnorderedAccessViewHandle, xiiGAL::xii18_14Id);

  friend class xiiGALDevice;
};

class xiiGALRenderTargetViewHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALRenderTargetViewHandle, xiiGAL::xii18_14Id);

  friend class xiiGALDevice;
};

class xiiGALDepthStencilStateHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALDepthStencilStateHandle, xiiGAL::xii16_16Id);

  friend class xiiGALDevice;
};

class xiiGALBlendStateHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALBlendStateHandle, xiiGAL::xii16_16Id);

  friend class xiiGALDevice;
};

class xiiGALRasterizerStateHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALRasterizerStateHandle, xiiGAL::xii16_16Id);

  friend class xiiGALDevice;
};

class xiiGALSamplerStateHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALSamplerStateHandle, xiiGAL::xii16_16Id);

  friend class xiiGALDevice;
};

class xiiGALVertexDeclarationHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALVertexDeclarationHandle, xiiGAL::xii18_14Id);

  friend class xiiGALDevice;
};

class xiiGALQueryHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGALQueryHandle, xiiGAL::xii20_12Id);

  friend class xiiGALDevice;
};

namespace xiiGAL
{
  struct ModifiedRange
  {
    XII_ALWAYS_INLINE void Reset()
    {
      m_uiMin = xiiInvalidIndex;
      m_uiMax = 0;
    }

    XII_FORCE_INLINE void SetToIncludeValue(xiiUInt32 value)
    {
      m_uiMin = xiiMath::Min(m_uiMin, value);
      m_uiMax = xiiMath::Max(m_uiMax, value);
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
