#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/Pipeline/Declarations.h>

class xiiRasterizerObject;

/// \brief Base class for all render data. Render data must contain all information that is needed to render the corresponding object.
class XII_GRAPHICSCORE_DLL xiiRenderData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderData, xiiReflectedClass);

public:
  struct Category
  {
    Category();
    explicit Category(xiiUInt16 uiValue);

    bool operator==(const Category& other) const;
    bool operator!=(const Category& other) const;

    xiiUInt16 m_uiValue = 0xFFFF;
  };

  struct Caching
  {
    enum Enum
    {
      Never,
      IfStatic
    };
  };

  /// \brief This function generates a 64bit sorting key for the given render data. Data with lower sorting key is rendered first.
  using SortingKeyFunc = xiiUInt64 (*)(const xiiRenderData*, const xiiCamera&);

  static Category RegisterCategory(xiiStringView sCategoryName, SortingKeyFunc sortingKeyFunc);
  static Category FindCategory(xiiTempHashedString sCategoryName);

  static void GetAllCategoryNames(xiiDynamicArray<xiiHashedString>& out_categoryNames);

  static const xiiRenderer* GetCategoryRenderer(Category category, const xiiRTTI* pRenderDataType);

  static xiiHashedString GetCategoryName(Category category);

  xiiUInt64 GetCategorySortingKey(Category category, const xiiCamera& camera) const;

  xiiTransform         m_GlobalTransform = xiiTransform::IdentityTransform();
  xiiBoundingBoxSphere m_GlobalBounds;

  xiiUInt32 m_uiBatchId           = 0; ///< BatchId is used to group render data in batches.
  xiiUInt32 m_uiSortingKey        = 0;
  float     m_fSortingDepthOffset = 0.0f;

  xiiGameObjectHandle m_hOwner;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiGameObject* m_pOwner = nullptr; ///< Debugging only. It is not allowed to access the game object during rendering.
#endif

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RenderData);

  static void PluginEventHandler(const xiiPluginEvent& e);
  static void UpdateRendererTypes();

  static void CreateRendererInstances();
  static void ClearRendererInstances();

  struct CategoryData
  {
    xiiHashedString m_sName;
    SortingKeyFunc  m_sortingKeyFunc;

    xiiHashTable<const xiiRTTI*, xiiUInt32> m_TypeToRendererIndex;
  };

  static xiiHybridArray<CategoryData, 32> s_CategoryData;

  static xiiHybridArray<const xiiRTTI*, 16>         s_RendererTypes;
  static xiiDynamicArray<xiiUniquePtr<xiiRenderer>> s_RendererInstances;
  static bool                                       s_bRendererInstancesDirty;
};

/// \brief Creates render data that is only valid for this frame. The data is automatically deleted after the frame has been rendered.
template <typename T>
static T* xiiCreateRenderDataForThisFrame(const xiiGameObject* pOwner);

struct XII_GRAPHICSCORE_DLL xiiDefaultRenderDataCategories
{
  static xiiRenderData::Category Light;
  static xiiRenderData::Category Decal;
  static xiiRenderData::Category ReflectionProbe;
  static xiiRenderData::Category Sky;
  static xiiRenderData::Category LitOpaque;
  static xiiRenderData::Category LitMasked;
  static xiiRenderData::Category LitTransparent;
  static xiiRenderData::Category LitForeground;
  static xiiRenderData::Category LitScreenFX;
  static xiiRenderData::Category SimpleOpaque;
  static xiiRenderData::Category SimpleTransparent;
  static xiiRenderData::Category SimpleForeground;
  static xiiRenderData::Category Selection;
  static xiiRenderData::Category GUI;
};

#define xiiInvalidRenderDataCategory xiiRenderData::Category()

struct XII_GRAPHICSCORE_DLL xiiMsgExtractRenderData : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractRenderData, xiiMessage);

  const xiiView*          m_pView            = nullptr;
  xiiRenderData::Category m_OverrideCategory = xiiInvalidRenderDataCategory;

  /// \brief Adds render data for the current view. This data can be cached depending on the specified caching behavior.
  /// Non-cached data is only valid for this frame. Cached data must be manually deleted using the xiiRenderWorld::DeleteCachedRenderData
  /// function.
  void AddRenderData(const xiiRenderData* pRenderData, xiiRenderData::Category category, xiiRenderData::Caching::Enum cachingBehavior);

private:
  friend class xiiExtractor;

  struct Data
  {
    const xiiRenderData* m_pRenderData = nullptr;
    xiiUInt16            m_uiCategory  = 0;
  };

  xiiHybridArray<Data, 16> m_ExtractedRenderData;
  xiiUInt32                m_uiNumCacheIfStatic = 0;
};

struct XII_GRAPHICSCORE_DLL xiiMsgExtractOccluderData : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractOccluderData, xiiMessage);

  void AddOccluder(const xiiRasterizerObject* pObject, const xiiTransform& transform)
  {
    auto& d       = m_ExtractedOccluderData.ExpandAndGetRef();
    d.m_pObject   = pObject;
    d.m_Transform = transform;
  }

private:
  friend class xiiRenderPipeline;

  struct Data
  {
    const xiiRasterizerObject* m_pObject = nullptr;
    xiiTransform               m_Transform;
  };

  xiiHybridArray<Data, 16> m_ExtractedOccluderData;
};

#include <GraphicsCore/Pipeline/Implementation/RenderData_inl.h>
