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
class xiiRenderPipelinePass;
class xiiDebugRendererContext;

struct xiiRenderPipelineNodePin;
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

struct xiiRenderViewContext
{
  const xiiCamera*   m_pCamera    = nullptr;
  const xiiCamera*   m_pLodCamera = nullptr;
  const xiiViewData* m_pViewData  = nullptr;

  const xiiDebugRendererContext* m_pWorldDebugContext = nullptr;
  const xiiDebugRendererContext* m_pViewDebugContext  = nullptr;

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
