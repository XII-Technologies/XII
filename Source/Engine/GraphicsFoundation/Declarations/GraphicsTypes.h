#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>

#include <GraphicsFoundation/Declarations/Constants.h>

/// \brief Defines the graphics device.
struct XII_GRAPHICSFOUNDATION_DLL xiiGraphicsDeviceType
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

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGraphicsDeviceType);

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
    Unknown = 0,           ///< Unknown format.
    RGBA32Typeless,        ///< Four component 128-bit typeless format with 32-bit channels.
    RGBA32Float,           ///< Four-component 128-bit floating-point format with 32-bit channels.
    RGBA32UInt,            ///< Four-component 128-bit unsigned-integer format with 32-bit channels.
    RGBA32SInt,            ///< Four-component 128-bit signed-integer format with 32-bit channels.
    RGB32Typeless,         ///< Three-component 96-bit typeless format with 32-bit channels.
    RGB32Float,            ///< Three-component 96-bit floating-point format with 32-bit channels.
    RGB32UInt,             ///< Three-component 96-bit unsigned-integer format with 32-bit channels.
    RGB32SInt,             ///< Three-component 96-bit signed-integer format with 32-bit channels.
    RGBA16Typeless,        ///< Four-component 64-bit typeless format with 16-bit channels.
    RGBA16Float,           ///< Four-component 64-bit floating-point format with 16-bit channels.
    RGBA16UNormalized,     ///< Four-component 64-bit unsigned-normalized-integer format with 16-bit channels.
    RGBA16UInt,            ///< Four-component 64-bit unsigned-integer format with 16-bit channels.
    RGBA16SNormalized,     ///< Four-component 64-bit signed-normalized-integer format with 16-bit channels.s
    RGBA16SInt,            ///< Four-component 64-bit signed-integer format with 16-bit channels.
    RG32Typeless,          ///< Two-component 64-bit typeless format with 32-bit channels.
    RG32Float,             ///< Two-component 64-bit floating-point format with 32-bit channels.
    RG32UInt,              ///< Two-component 64-bit unsigned-integer format with 32-bit channels.
    RG32SInt,              ///< Two-component 64-bit signed-integer format with 32-bit channels.
    R32G8X24Typeless,      ///< Two-component 64-bit typeless format with 32-bits for the R channel and 8-bits for the G channel.
    D32FloatS8X24UInt,     ///< Two-component 64-bit format with 32-bit floating-point depth channel and 8-bit stencil channel.
    R32FloatX8X24Typeless, ///< Two-component 64-bit format with 32-bit floating-point R channel ad 8+24-bits of typeless data.
    X32TypelessG8X24UInt,  ///< Two-component 64-bit format with 32-bit typeless data and 8-bit G channel.
    RGB10A2Typeless,       ///< Four-component 32-bit typeles format with 10-bits for the RGB channels and 2-bits for the alpha channel.
    RGB10A2UNormalized,    ///< Four-component 32-bit unsigned-normalized-integer format with 10-bits for the RGB channels and 2-bits for the alpha channel.
    RGB10A2UInt,           ///< Four-component 32-bit unsigned-integer format with 10-bits for the RGB channels and 2-bits for the alpha channel.
    RG11B10Float,          ///< Three-component 32-bit format encoding three partial precision channels using 11-bits for the red and green channels, and 10-bits for the blue channel.
    RGBA8Typeless,         ///< Four-component 32-bit typeless format with 8-bit channels.
    RGBA8UNormalized,      ///< Four-component 32-bit unsigned-normalized-integer format with 8-bit channels.
    RGBA8UNormalizedSRGB,  ///< Four-component 32-bit unsigned-normalized-integer sRGB format with 8-bit channels.
    RGBA8UInt,             ///< Four-component 32-bit unsigned-integer format with 8-bit channels.
    RGBA8SNormalized,      ///< Four-component 32-bit signed-normalized-integer format with 8-bit channels.
    RGBA8SInt,             ///< Four-component 32-bit signed-integer format with 8-bit channels.
    RG16Typeless,          ///< Two-component 32-bit typeless format with 16-bit channels.
    RG16Float,             ///< Two-component 32-bit half-precision floating-point format with 16-bit channels.
    RG16UNormalized,       ///< Two-component 32-bit unsigned-normalized-integer format with 16-bit channels.
    RG16UInt,              ///< Two-component 32-bit unsigned-integer format with 16-bit channels.
    RG16SNormalized,       ///< Two-component signed-normalized-integer format with 16-bit channels.
    RG16SInt,              ///< Two-component 32-bit signed-integer format with 16-bit channels.
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALTextureFormat);
