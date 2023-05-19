#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERFOUNDATION_LIB
#    define XII_RENDERERFOUNDATION_DLL XII_DECL_EXPORT
#  else
#    define XII_RENDERERFOUNDATION_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_RENDERERFOUNDATION_DLL
#endif

////////// Definitions //////////

/// The maximum number of bound constant buffers.
#define XII_GAL_MAX_CONSTANT_BUFFER_COUNT 16

/// The maximum number of bound samplers.
#define XII_GAL_MAX_SAMPLER_COUNT 16

/// The maximum number of bound vertex buffers.
#define XII_GAL_MAX_VERTEX_BUFFER_COUNT 16

/// The maximum number of bound render targets.
#define XII_GAL_MAX_RENDERTARGET_COUNT 8

/// The maximum number of shading rate modes.
#define XII_GAL_MAX_SHADING_RATE 9

/// The bit shift for the shading X-Axis rate.
#define XII_GAL_SHADING_RATE_X_SHIFT 2

// Forward declarations

struct xiiGALDeviceCreationDescription;
struct xiiGALSwapChainCreationDescription;
struct xiiGALWindowSwapChainCreationDescription;
struct xiiGALShaderCreationDescription;
struct xiiGALTextureCreationDescription;
struct xiiGALBufferCreationDescription;
struct xiiGALDepthStencilStateCreationDescription;
struct xiiGALBlendStateCreationDescription;
struct xiiGALRasterizerStateCreationDescription;
struct xiiGALVertexDeclarationCreationDescription;
struct xiiGALQueryCreationDescription;
struct xiiGALSamplerStateCreationDescription;
struct xiiGALResourceViewCreationDescription;
struct xiiGALRenderTargetViewCreationDescription;
struct xiiGALUnorderedAccessViewCreationDescription;
struct xiiGALCommandEncoderState;
struct xiiGALCommandEncoderRenderState;

class xiiGALSwapChain;
class xiiGALShader;
class xiiGALResourceBase;
class xiiGALTexture;
class xiiGALBuffer;
class xiiGALDepthStencilState;
class xiiGALBlendState;
class xiiGALRasterizerState;
class xiiGALRenderTargetSetup;
class xiiGALVertexDeclaration;
class xiiGALQuery;
class xiiGALSamplerState;
class xiiGALResourceView;
class xiiGALRenderTargetView;
class xiiGALUnorderedAccessView;
class xiiGALDevice;
class xiiGALPass;
class xiiGALCommandEncoder;
class xiiGALRenderCommandEncoder;
class xiiGALComputeCommandEncoder;

////////// Enumerations //////////

/// \brief Defines the graphics device.
struct XII_RENDERERFOUNDATION_DLL xiiGraphicsDeviceType
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    Undefined = 0, ///< Undefined graphics device type.

    D3D11,    ///< DirectX 11 graphics device.
    D3D12,    ///< DirectX 12 graphics device.
    Vulkan,   ///< Vulkan graphics device.
    Metal,    ///< Metal graphics device.
    OpenGL,   ///< OpenGL graphics device.
    OpenGLES, ///< OpenGLES graphics device.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGraphicsDeviceType);

/// \brief Defines the primitive type.
struct XII_RENDERERFOUNDATION_DLL xiiGALPrimitiveTopology
{
  using StorageType = xiiInt8;

  enum Enum : xiiInt8
  {
    /// Note: Preserve this order, it is used to allocate the desired number of indices in xiiMeshBufferResourceDescriptor::AllocateStreams

    Undefined = -1, ///< Undefined topology. No primitive indices.
    Points,         ///< Interpret the vertex data as a list of points. 1 index per primitive.
    Lines,          ///< Interpret the vertex data as a list of lines. 2 indices per primitive.
    Triangles,      ///< Interpret the vertex data as a list of triangles. 3 indices per primitive.

    ENUM_COUNT,

    Default = Triangles
  };

  static xiiUInt32 VerticesPerPrimitive(xiiGALPrimitiveTopology::Enum e) { return (xiiUInt32)e + 1; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALPrimitiveTopology);

/// \brief Defines the primitive index type.
struct XII_RENDERERFOUNDATION_DLL xiiGALIndexType
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    None,   ///< The indices are not used, vertices are only used in order to form primitives.
    UShort, ///< 16 bit indices are used to select which vertices shall form a primitive, thus meshes can only use up to 65535 vertices.
    UInt,   ///< 32 bit indices are used to select which vertices shall form a primitive.

    ENUM_COUNT,

    Default = None
  };

  /// \brief The size in bytes of a single element of the given index format.
  static xiiUInt8 GetSize(xiiGALIndexType::Enum format) { return s_Size[format]; }

private:
  static const xiiUInt8 s_Size[xiiGALIndexType::ENUM_COUNT];
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALIndexType);

/// \brief Defines the writable components of a render target.
struct XII_RENDERERFOUNDATION_DLL xiiGALColorWriteMask
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    None = 0x0, ///< Do not write to any components.

    Red   = XII_BIT(0), ///< Write to the red component.
    Green = XII_BIT(1), ///< Write to the green component.
    Blue  = XII_BIT(2), ///< Write to the blue component.
    Alpha = XII_BIT(3), ///< Write to the alpha component.

    RG   = Red | Green,                ///< Write to the red and green components.
    RGB  = Red | Green | Blue,         ///< Write to the red, green and blue components.
    RGBA = Red | Green | Blue | Alpha, ///< Write to the red, green, blue, and alpha components.

    Default = (((Red | Green) | Blue) | Alpha) ///< Write to all components.
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALColorWriteMask);

/// \brief Defines the pipeline shader stage.
struct XII_RENDERERFOUNDATION_DLL xiiGALShaderStage
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    VertexShader = 0,      ///< Vertex shader stage.
    PixelShader,           ///< Pixel (fragment) shader stage.
    GeometryShader,        ///< Geometry shader stage.
    HullShader,            ///< Hull (tessellation control) shader stage.
    DomainShader,          ///< Domain (tessellation evaluation) shader stage.
    ComputeShader,         ///< Compute shader stage.
    AmplificationShader,   ///< Amplification shader stage.
    MeshShader,            ///< Mesh shader stage.
    RayGenShader,          ///< Ray generation shader stage.
    RayMissShader,         ///< Ray miss shader stage.
    RayAnyHitShader,       ///< Ray any hit shader stage.
    RayClosestHitShader,   ///< Ray closest hit shader stage.
    RayIntersectionShader, ///< Ray intersection shader stage.
    CallableShader,        ///< Callable shader stage.

    ENUM_COUNT,

    Default = VertexShader
  };

  static const char* Names[ENUM_COUNT];
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALShaderStage);

/// \brief Defines the multisample anti-aliasing count.
struct XII_RENDERERFOUNDATION_DLL xiiGALMSAASampleCount
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    None         = 1, ///< No multisampling.
    TwoSamples   = 2, ///< Two samples per pixel.
    FourSamples  = 4, ///< Four samples per pixel.
    EightSamples = 8, ///< Eight samples per pixel.

    ENUM_COUNT = 4,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALMSAASampleCount);

/// \brief Defines the texture type.
struct XII_RENDERERFOUNDATION_DLL xiiGALTextureType
{
  using StorageType = xiiInt8;

  enum Enum : xiiInt8
  {
    Invalid = -1,

    Texture1D = 0,       ///< One dimensional texture.
    Texture1DArray,      ///< One dimensional texture array.
    Texture2D,           ///< Two dimensional texture.
    Texture2DArray,      ///< Two dimensional texture array.
    TextureCube,         ///< Two dimensional texture array that contains 6 textures, one for each face of the cube.
    TextureCubeArray,    ///< An array of texture cubes.
    Texture3D,           ///< Three dimensional texture.
    Texture2DProxy,      ///< Proxy texture to a two dimensional texture.
    Texture2DProxyArray, ///< Proxy texture array to an array of two dimensional textures.

    ENUM_COUNT,

    Default = Texture2D
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALTextureType);

/// \brief Defines the blend factors for alpha-blending.
struct XII_RENDERERFOUNDATION_DLL xiiGALBlendFactor
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    Undefined = 0,     ///< Undefined blend factor.
    Zero,              ///< The blend factor is Zero.
    One,               ///< The blend factor is One.
    SrcColor,          ///< The blend factor is RGB data from a pixel shader.
    InvSrcColor,       ///< The blend factor is One minus RGB, where RGB is the data from a pixel shader.
    SrcAlpha,          ///< The blend factor is Alpha (A) data from a pixel shader.
    InvSrcAlpha,       ///< The blend factor is One minus Alpha, where Alpha is alpha data from a pixel shader.
    DestAlpha,         ///< The blend factor is Alpha data from a render target.
    InvDestAlpha,      ///< The blend factor is One minus Alpha, where Alpha is alpha data from a render target.
    DestColor,         ///< The blend factor is RGB data from a render target.
    InvDestColor,      ///< The blend factor is One minus RGB, where RGB is the data from a render target.
    SrcAlphaSaturated, ///< The blend factor is (f, f, f, 1), where f = min(As, 1 - Ad). As is alpha data from a pixel shader, and Ad is alpha from a render target.
    BlendFactor,       ///< The blend factor is the constant blend factor set with the xiiGALRenderCommandEncoder::SetBlendState(...).
    InvBlendFactor,    ///< The blend factor is One minus the constant blend factor set with the xiiGALRenderCommandEncoder::SetBlendState(...).
    SrcOneColor,       ///< The blend factor is the second RGB data output from a pixel shader.
    InvSrcOneColor,    ///< The blend factor is One minus RGB, where RGB is the second RGB data output from a pixel shader.
    SrcOneAlpha,       ///< The blend factor is second Alpha (A) data output from a pixel shader.
    InvSrcOneAlpha,    ///< The blend factor is One minus Alpha, where Alpha is the second alpha data output from a pixel shader.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALBlendFactor);

/// \brief Defines the blend operation for RGB or Alpha channels.
struct XII_RENDERERFOUNDATION_DLL xiiGALBlendOperation
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    Undefined = 0, ///< Undefined blend operation.
    Add,           ///< Add source and destination color components.
    Subtract,      ///< Subtract destination color components from source color components.
    RevSubtract,   ///< Subtract source color components from destination color components.
    Min,           ///< Compute the minimum of source and destination color components.
    Max,           ///< Compute the maximum of source and destination color components.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALBlendOperation);

/// Defines the stencil operation.
struct XII_RENDERERFOUNDATION_DLL xiiGALStencilOperation
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    Undefined = 0,      ///< Undefined stencil operation.
    Keep,               ///< Keep the existing stencil data.
    Zero,               ///< Set the stencil data to Zero.
    Replace,            ///< Set the stencil data to the reference value set by calling xiiGALCommandEncoder::SetStencilRef(...).
    IncrementSaturated, ///< Increment the current stencil value, and clamp to the maximum representable unsigned value.
    DecrementSaturated, ///< Decrement the current stencil value, and clamp to Zero.
    Invert,             ///< Bitwise invert the current stencil buffer value.
    IncrementWrap,      ///< Increment the current stencil value, and wrap to Zero when incrementing.
    DecrementWrap,      ///< Decrement the current stencil value, and wrap the value to the maximum representable unsigned value when decrementing a value of Zero.

    ENUM_COUNT,

    Default = Keep
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALStencilOperation);

/// \brief Defines the comparison function.
struct XII_RENDERERFOUNDATION_DLL xiiGALCompareFunc
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    Undefined = 0, ///< Undefined comparison function.
    Never,         ///< Comparison never passes.
    Less,          ///< Comparison passes if the source data is less than the destination data.
    Equal,         ///< Comparison passes if the source data is equal to the destination data.
    LessEqual,     ///< Comparison passes if the source data is less than or equal to the destination data.
    Greater,       ///< Comparison passes if the source data is greater than the destination data.
    NotEqual,      ///< Comparison passes if the source data is not equal to the destination data.
    GreaterEqual,  ///< Comparison passes if the source data is greater than or equal to the destination data.
    Always,        ///< Comparison always passes.

    ENUM_COUNT,

    Default = Never
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALCompareFunc);

/// \brief Defines which triangles are not drawn during the rasterization stage.
struct XII_RENDERERFOUNDATION_DLL xiiGALCullMode
{
  using StorageType = xiiUInt8;

  /// \brief Defines which sides of a polygon gets culled by the graphics card
  enum Enum : xiiUInt8
  {
    Undefined = 0, ///< Undefined cull mode.
    None,          ///< Draw all triangles.
    Front,         ///< Do not draw trangles that are front facing.
    Back,          ///< Do not draw triangles that are back facing.

    ENUM_COUNT,

    Default = Back
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALCullMode);

/// \brief Defines the filter mode.
///
/// \note On D3D11, comparison filters only work with textures that have the following formats
/// R32_FLOAT_X8X24_TYPELESS, R32_FLOAT, R24_UNORM_X8_TYPELESS, R16_UNORM.
struct XII_RENDERERFOUNDATION_DLL xiiGALTextureFilterMode
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    Undefined = 0,         ///< Undefined filter mode.
    Point,                 ///< Pointer filtering.
    Linear,                ///< Linear filtering.
    Anisotropic,           ///< Anisotropic filtering.
    ComparisonPoint,       ///< Comparison-point filtering.
    ComparisonLinear,      ///< Comparison-linear filtering.
    ComparisonAnisotropic, ///< Comparison-anisotropic filtering.

    ENUM_COUNT,

    Default = Linear
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALTextureFilterMode);

/// \brief Defines the update mode.
struct XII_RENDERERFOUNDATION_DLL xiiGALUpdateMode
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    Undefined         = 0x0,        ///< Undefined update mode.
    DoNotWait         = XII_BIT(0), ///< Do not wait another previous command using the resource completes. Map returns null pointer if the resource is still in use.
    Discard           = XII_BIT(1), ///< Discard the previous contents of the resource. Thus, making its contents undefined.
    NoOverWrite       = XII_BIT(2), ///< The system will not synchronize pending operations before mapping the buffer.

    ENUM_COUNT = 5,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALUpdateMode);

/// \brief Defines the update type.
struct XII_RENDERERFOUNDATION_DLL xiiGALUpdateType
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    Read      = XII_BIT(0), ///< Read the resource.
    Write     = XII_BIT(1), ///< Write to the resource.
    ReadWrite = XII_BIT(2), ///< Read and Write to the resource.

    ENUM_COUNT = 3,

    Default = Write,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALUpdateType);

////////// Basic Structs //////////

struct XII_RENDERERFOUNDATION_DLL xiiGALTextureSubresource
{
  xiiUInt32 m_uiMipLevel   = 0; ///< The mip level to use.
  xiiUInt32 m_uiArraySlice = 0; ///< The number of array slices.
};

struct XII_RENDERERFOUNDATION_DLL xiiGALSystemMemoryDescription
{
  void*     m_pData        = nullptr; ///< Pointer to the data.
  xiiUInt32 m_uiRowPitch   = 0;       ///< Row pitch.
  xiiUInt32 m_uiSlicePitch = 0;       ///< Slice pitch.
};

/// \brief Base class for GAL objects, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class xiiGALObject : public xiiRefCounted
{
public:
  xiiGALObject(const CreationDescription& Description) :
    m_Description(Description)
  {
  }

  XII_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  const CreationDescription m_Description;
};

// Handles
namespace xiiGAL
{
  typedef xiiGenericId<16, 16> xii16_16Id;
  typedef xiiGenericId<18, 14> xii18_14Id;
  typedef xiiGenericId<20, 12> xii20_12Id;
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

struct xiiGALTimestampHandle
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64 m_uiIndex;
  xiiUInt64 m_uiFrameCounter;
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

    XII_ALWAYS_INLINE bool IsValid() const { return m_uiMin <= m_uiMax; }

    XII_ALWAYS_INLINE xiiUInt32 GetCount() const { return m_uiMax - m_uiMin + 1; }

    xiiUInt32 m_uiMin = xiiInvalidIndex;
    xiiUInt32 m_uiMax = 0;
  };
} // namespace xiiGAL
