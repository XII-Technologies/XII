#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/Declarations.h>

class xiiCamera;
class xiiExtractedRenderData;
class xiiExtractor;
class xiiView;
class xiiRenderer;
class xiiRenderData;
class xiiRenderDataBatch;
class xiiRenderPipeline;
class xiiRenderPipelinePassBase;
class xiiGraphicsPipelinePass;
class xiiComputePipelinePass;
class xiiCopyPipelinePass;
class xiiPresentPipelinePass;
class xiiUtilityPipelinePass;
class xiiDebugRendererContext;

struct xiiRenderPipelineNodePin;
struct xiiRenderPipelineNodeInputBufferPin;
struct xiiRenderPipelineNodeInputColourAttachmentPin;
struct xiiRenderPipelineNodeInputDepthAttachmentPin;
struct xiiRenderPipelineNodeInputSamplerPin;
struct xiiRenderPipelineNodeInputAccelerationStructurePin;
struct xiiRenderPipelineNodeOutputBufferPin;
struct xiiRenderPipelineNodeOutputColourAttachmentPin;
struct xiiRenderPipelineNodeOutputDepthAttachmentPin;
struct xiiRenderPipelineNodeOutputSamplerPin;
struct xiiRenderPipelineNodeOutputAccelerationStructurePin;
struct xiiRenderPipelineNodePassThroughBufferPin;
struct xiiRenderPipelineNodePassThroughColourAttachmentPin;
struct xiiRenderPipelineNodePassThroughDepthAttachmentPin;
struct xiiRenderPipelineNodePassThroughSamplerPin;
struct xiiRenderPipelineNodePassThroughAccelerationStructurePin;
struct xiiRenderPipelineNodeInputBufferProviderPin;
struct xiiRenderPipelineNodeInputColourAttachmentProviderPin;
struct xiiRenderPipelineNodeInputDepthAttachmentProviderPin;
struct xiiRenderPipelineNodeInputSamplerProviderPin;
struct xiiRenderPipelineNodeInputAccelerationStructureProviderPin;
struct xiiRenderPipelineNodeOutputBufferProviderPin;
struct xiiRenderPipelineNodeOutputColourAttachmentProviderPin;
struct xiiRenderPipelineNodeOutputDepthAttachmentProviderPin;
struct xiiRenderPipelineNodeOutputSamplerProviderPin;
struct xiiRenderPipelineNodeOutputAccelerationStructureProviderPin;
struct xiiRenderPipelinePassConnection;
struct xiiViewData;

namespace xiiInternal
{
  struct RenderDataCache;

  struct RenderDataCacheEntry
  {
    XII_DECLARE_POD_TYPE();

    const xiiRenderData* m_pRenderData      = nullptr;
    xiiUInt16            m_uiCategory       = 0;
    xiiUInt16            m_uiComponentIndex = 0;
    xiiUInt16            m_uiPartIndex      = 0;

    XII_ALWAYS_INLINE bool operator==(const RenderDataCacheEntry& other) const { return m_pRenderData == other.m_pRenderData && m_uiCategory == other.m_uiCategory && m_uiComponentIndex == other.m_uiComponentIndex && m_uiPartIndex == other.m_uiPartIndex; }

    /// \brief Cache entries need to be sorted by component index and then by part index.
    XII_ALWAYS_INLINE bool operator<(const RenderDataCacheEntry& other) const
    {
      if (m_uiComponentIndex == other.m_uiComponentIndex)
        return m_uiPartIndex < other.m_uiPartIndex;

      return m_uiComponentIndex < other.m_uiComponentIndex;
    }
  };
} // namespace xiiInternal

struct XII_GRAPHICSCORE_DLL xiiSourceFormat
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Color4Channel8BitNormalized_sRGB = 0U, ///< RGBA 8-bit normalized with sRGB colorspace (default).
    Color4Channel8BitNormalized,           ///< RGBA 8-bit normalized linear.
    Color2Channel16BitFloat,               ///< RG 16-bit float.
    Color4Channel16BitFloat,               ///< RGBA 16-bit float.
    Color2Channel32BitFloat,               ///< RG 32-bit float.
    Color3Channel32BitFloat,               ///< RGB 32-bit float.
    Color4Channel32BitFloat,               ///< RGBA 32-bit float.
    Color3Channel11_11_10BitFloat,         ///< RGB 11-11-10 bit float.
    Depth16Bit,                            ///< 16-bit depth.
    Depth24BitStencil8Bit,                 ///< 24-bit depth + 8-bit stencil.
    Depth32BitFloat,                       ///< 32-bit float depth.
    BC1_RGB_DXT1,                          ///< DXT1 compression (no alpha).
    BC2_RGBA_DXT3,                         ///< DXT3 compression.
    BC3_RGBA_DXT5,                         ///< DXT5 compression.
    BC4_R_Grey_DXT5A,                      ///< Single-channel compression (DXT5a).
    BC5_RG_Grey_DXT5A,                     ///< Dual-channel compression (DXT5a).
    BC6H_RGB_Float,                        ///< HDR RGB float block compression.
    BC7_RGBA,                              ///< Modern RGBA compression with high quality.

    ENUM_COUNT,

    Default = Color4Channel8BitNormalized_sRGB
  };

  static xiiGALResourceFormat::Enum GetGALResourceFormat(xiiSourceFormat::Enum format, bool bFlipColorChannels = false);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSourceFormat);

struct XII_GRAPHICSCORE_DLL xiiRenderViewContext
{
  const xiiCamera*   m_pCamera    = nullptr;
  const xiiCamera*   m_pLodCamera = nullptr;
  const xiiViewData* m_pViewData  = nullptr;

  const xiiDebugRendererContext* m_pWorldDebugContext = nullptr;
  const xiiDebugRendererContext* m_pViewDebugContext  = nullptr;

  xiiSharedPtr<xiiGALCommandList> m_pCommandList;

  XII_ALWAYS_INLINE const xiiHashTable<xiiHashedString, xiiHashedString>& GetPermutationVariables() const { return m_PermutationVariables; }

  void SetShaderPermutationVariable(const char* szName, const xiiTempHashedString& sTempValue) const;
  void SetShaderPermutationVariable(xiiStringView sName, const xiiTempHashedString& sTempValue) const;
  void SetShaderPermutationVariable(const xiiHashedString& sName, const xiiHashedString& sValue) const;

private:
  void SetShaderPermutationVariableInternal(const xiiHashedString& sName, const xiiHashedString& sValue) const;

  mutable xiiHashTable<xiiHashedString, xiiHashedString> m_PermutationVariables;
};

using xiiViewId = xiiGenericId<24, 8>;

class xiiViewHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiViewHandle, xiiViewId);

  friend class xiiRenderWorld;
};

/// \brief HashHelper implementation so view handles can be used as key in a hashtable.
template <>
struct xiiHashHelper<xiiViewHandle>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiViewHandle value) { return value.GetInternalID().m_Data * 2654435761U; }

  XII_ALWAYS_INLINE static bool Equal(xiiViewHandle a, xiiViewHandle b) { return a == b; }
};

/// \brief Usage hint of a camera/view.
struct XII_GRAPHICSCORE_DLL xiiCameraUsageHint
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    None,
    MainView,
    EditorView,
    RenderTarget,
    Culling,
    Shadow,
    Reflection,
    Thumbnail,

    ENUM_COUNT,

    Default = None,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiCameraUsageHint);
