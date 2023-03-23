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

// Necessary array sizes
#define XII_GAL_MAX_CONSTANT_BUFFER_COUNT 16
#define XII_GAL_MAX_SAMPLER_COUNT         16
#define XII_GAL_MAX_VERTEX_BUFFER_COUNT   16
#define XII_GAL_MAX_RENDERTARGET_COUNT    8

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

// Basic enums
struct xiiGALPrimitiveTopology
{
  typedef xiiUInt8 StorageType;
  enum Enum
  {
    // keep this order, it is used to allocate the desired number of indices in xiiMeshBufferResourceDescriptor::AllocateStreams
    Points,    // 1 index per primitive
    Lines,     // 2 indices per primitive
    Triangles, // 3 indices per primitive
    ENUM_COUNT,
    Default = Triangles
  };

  static xiiUInt32 VerticesPerPrimitive(xiiGALPrimitiveTopology::Enum e) { return (xiiUInt32)e + 1; }
};

struct XII_RENDERERFOUNDATION_DLL xiiGALIndexType
{
  enum Enum
  {
    None,   // indices are not used, vertices are just used in order to form primitives
    UShort, // 16 bit indices are used to select which vertices shall form a primitive, thus meshes can only use up to 65535 vertices
    UInt,   // 32 bit indices are used to select which vertices shall form a primitive

    ENUM_COUNT
  };

  /// \brief The size in bytes of a single element of the given index format.
  static xiiUInt8 GetSize(xiiGALIndexType::Enum format) { return s_Size[format]; }

private:
  static const xiiUInt8 s_Size[xiiGALIndexType::ENUM_COUNT];
};

/// \brief Defines the writable components of a render target.
struct XII_RENDERERFOUNDATION_DLL xiiGALColorWriteMask
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    None  = 0u,       ///< Do not write to any components.
    Red   = 1u << 0u, ///< Write to the red component.
    Green = 1u << 1u, ///< Write to the green component.
    Blue  = 1u << 2u, ///< Write to the blue component.
    Alpha = 1u << 3u, ///< Write to the alpha component.

    Default = (((Red | Green) | Blue) | Alpha)
  };
};

struct XII_RENDERERFOUNDATION_DLL xiiGALShaderStage
{
  enum Enum : xiiUInt8
  {
    VertexShader,
    HullShader,
    DomainShader,
    GeometryShader,
    PixelShader,

    ComputeShader,

    ENUM_COUNT
  };

  static const char* Names[ENUM_COUNT];
};

struct XII_RENDERERFOUNDATION_DLL xiiGALMSAASampleCount
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    None         = 1,
    TwoSamples   = 2,
    FourSamples  = 4,
    EightSamples = 8,

    ENUM_COUNT = 4,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALMSAASampleCount);

struct xiiGALTextureType
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Invalid   = -1,
    Texture2D = 0,
    TextureCube,
    Texture3D,
    Texture2DProxy,

    ENUM_COUNT,

    Default = Texture2D
  };
};

struct xiiGALBlend
{
  enum Enum
  {
    Zero = 0,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DestAlpha,
    InvDestAlpha,
    DestColor,
    InvDestColor,
    SrcAlphaSaturated,
    BlendFactor,
    InvBlendFactor,

    ENUM_COUNT
  };
};

struct xiiGALBlendOp
{
  enum Enum
  {
    Add = 0,
    Subtract,
    RevSubtract,
    Min,
    Max,

    ENUM_COUNT
  };
};

struct xiiGALStencilOp
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Keep = 0,
    Zero,
    Replace,
    IncrementSaturated,
    DecrementSaturated,
    Invert,
    Increment,
    Decrement,

    ENUM_COUNT,

    Default = Keep
  };
};

struct xiiGALCompareFunc
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,

    ENUM_COUNT,

    Default = Never
  };
};

/// \brief Defines which sides of a polygon gets culled by the graphics card
struct xiiGALCullMode
{
  typedef xiiUInt8 StorageType;

  /// \brief Defines which sides of a polygon gets culled by the graphics card
  enum Enum
  {
    None  = 0, ///< Triangles do not get culled
    Front = 1, ///< When the 'front' of a triangle is visible, it gets culled. The rasterizer state defines which side is the 'front'. See
               ///< xiiGALRasterizerStateCreationDescription for details.
    Back = 2,  ///< When the 'back'  of a triangle is visible, it gets culled. The rasterizer state defines which side is the 'front'. See
               ///< xiiGALRasterizerStateCreationDescription for details.

    ENUM_COUNT,

    Default = Back
  };
};

struct xiiGALTextureFilterMode
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Point = 0,
    Linear,
    Anisotropic,
    ComparisonPoint,
    ComparisonLinear,
    ComparisonAnisotropic,

    Default = Linear
  };
};

struct xiiGALUpdateMode
{
  enum Enum
  {
    None              = XII_BIT(0),
    DoNotWait         = XII_BIT(1), ///< Do not wait another previous command using the resource completes. Map returns null pointer if the resource is still in use.
    Discard           = XII_BIT(2), ///< Discard the previous contents of the resource. Thus, making its contents undefined.
    NoOverWrite       = XII_BIT(3), ///< The system will not synchronize pending operations before mapping the buffer.
    CopyToTempStorage = XII_BIT(4)  ///< Use a temporary staging resource to upload data to the GPU.
  };
};

// Basic structs
struct xiiGALTextureSubresource
{
  xiiUInt32 m_uiMipLevel   = 0;
  xiiUInt32 m_uiArraySlice = 0;
};

struct xiiGALSystemMemoryDescription
{
  void*     m_pData        = nullptr;
  xiiUInt32 m_uiRowPitch   = 0;
  xiiUInt32 m_uiSlicePitch = 0;
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
