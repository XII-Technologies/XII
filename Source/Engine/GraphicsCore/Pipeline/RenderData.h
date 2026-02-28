#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Math/Vec3.h>
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

    XII_ALWAYS_INLINE bool IsValid() const { return m_uiValue != 0xFFFFU; }

    xiiUInt16 m_uiValue = 0xFFFF;
  };

  /// \brief This function generates a 64bit sorting key for the given render data. Data with lower sorting key is rendered first.
  using SortingKeyFunc = xiiUInt64 (*)(const xiiRenderData*, const xiiCamera&);

  static Category RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc);
  static Category RegisterDerivedCategory(const char* szCategoryName, Category baseCategory);
  static Category RegisterRedirectedCategory(const char* szCategoryName, Category staticCategory, Category dynamicCategory);
  static Category FindCategory(xiiTempHashedString sCategoryName);
  static Category ResolveCategory(Category category, bool bDynamic);

  static xiiHashedString GetCategoryName(Category category);
  static void            GetAllCategoryNames(xiiDynamicArray<xiiHashedString>& out_categoryNames);

public:
  struct Caching
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      Never = 0U,
      IfStatic,

      ENUM_COUNT,

      Default = Never
    };
  };

  struct Flags
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      Dynamic     = XII_BIT(0),
      FlipWinding = XII_BIT(1),

      Default = 0U
    };

    struct Bits
    {
      StorageType Dynamic : 1;
      StorageType FlipWinding : 1;
    };
  };

  bool IsDynamic() const;
  bool IsStatic() const;
  bool FlipWinding() const;

  /// \brief Returns the final sorting for this render data with the given category and camera.
  xiiUInt64 GetFinalSortingKey(Category category, const xiiCamera& camera) const;

  /// \brief Returns whether this render data and the other render data can be batched together, e.g. rendered in one draw call.
  /// An implementation can assume that the other render data is of the same type as this render data.
  virtual bool CanBatch(const xiiRenderData& other) const { return false; }

  xiiBitflags<Flags> m_Flags;

  xiiVec3 m_vGlobalPosition     = xiiVec3::MakeZero();
  float   m_fSortingDepthOffset = 0.0f;

  xiiUInt32 m_uiSortingKey = 0;

  xiiGameObjectHandle m_hOwner;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiGameObject* m_pOwner = nullptr; ///< Debugging only. It is not allowed to access the game object during rendering.
#endif

private:
  struct CategoryData
  {
    Category m_BaseCategory;
    Category m_StaticCategory;
    Category m_DynamicCategory;

    xiiHashedString m_sName;
    SortingKeyFunc  m_SortingKeyFunc;
  };

  static xiiHybridArray<CategoryData, 32> s_CategoryData;
};

/// \brief Base class for render data that make uses of the instance data offset buffer which will be generated during the extraction phase.
class XII_GRAPHICSCORE_DLL xiiInstanceableRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInstanceableRenderData, xiiRenderData);

public:
  struct DataOffsets
  {
    xiiUInt32 m_uiInstance       = 0U;
    xiiUInt32 m_uiCustomInstance = 0U;
    xiiUInt32 m_uiMaterial       = 0U;
    xiiUInt32 m_uiSkinning       = 0U; // TODO: this could be removed if we switch to compute shader skinning
  };

  DataOffsets m_DataOffsets;

  xiiUInt32                 m_uiNumInstances = 1;
  xiiGALDynamicBufferHandle m_hInstanceDataBuffer;

protected:
  bool CanBatchByBaseValues(const xiiInstanceableRenderData& other) const;
};

struct XII_GRAPHICSCORE_DLL xiiDefaultRenderDataCategories
{
  static xiiRenderData::Category Light;
  static xiiRenderData::Category Decal;
  static xiiRenderData::Category ReflectionProbe;
  static xiiRenderData::Category Sky;
  static xiiRenderData::Category OpaqueStatic;
  static xiiRenderData::Category OpaqueDynamic;
  static xiiRenderData::Category Opaque;
  static xiiRenderData::Category MaskedStatic;
  static xiiRenderData::Category MaskedDynamic;
  static xiiRenderData::Category Masked;
  static xiiRenderData::Category Transparent;
  static xiiRenderData::Category Foreground;
  static xiiRenderData::Category ScreenFX;
  static xiiRenderData::Category SimpleOpaque;
  static xiiRenderData::Category SimpleTransparent;
  static xiiRenderData::Category Selection;
  static xiiRenderData::Category GUI;
};

#define xiiInvalidRenderDataCategory xiiRenderData::Category()

struct XII_GRAPHICSCORE_DLL xiiMsgExtractRenderData : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractRenderData, xiiMessage);

  const xiiView*              m_pView              = nullptr;
  const xiiRenderDataManager* m_pRenderDataManager = nullptr;
  xiiRenderData::Category     m_OverrideCategory   = xiiInvalidRenderDataCategory;

  /// \brief Adds render data for the current view. This data can be cached depending on the specified caching behavior.
  ///
  /// Non-cached data is only valid for this frame. Cached data must be manually deleted using the xiiRenderWorld::DeleteCachedRenderData function.
  void AddRenderData(const xiiRenderData* pRenderData, xiiRenderData::Category category, xiiRenderData::Caching::Enum cachingBehavior);

private:
  friend class xiiExtractor;

  struct Data
  {
    const xiiRenderData*    m_pRenderData = nullptr;
    xiiRenderData::Category m_Category;
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

struct xiiInstanceDataOffset
{
  XII_DECLARE_POD_TYPE();

  xiiInstanceDataOffset() :
    m_uiOffset(xiiMath::Bitmask_LowN<xiiUInt32>(31)), m_uiIsDynamic(0)
  {
  }

  XII_ALWAYS_INLINE bool IsInvalidated() const { return m_uiOffset == xiiMath::Bitmask_LowN<xiiUInt32>(31); }

  xiiUInt32 m_uiOffset : 31;
  xiiUInt32 m_uiIsDynamic : 1;
};

struct xiiCustomInstanceDataOffset
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE bool IsInvalidated() const { return m_uiOffset == xiiInvalidIndex; }

  xiiUInt32 m_uiOffset = xiiInvalidIndex;
};

struct XII_GRAPHICSCORE_DLL xiiMsgCustomInstanceDataOffsetChanged : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgCustomInstanceDataOffsetChanged, xiiMessage);

  xiiCustomInstanceDataOffset m_NewOffset;
};

#include <GraphicsCore/Pipeline/Implementation/RenderData_inl.h>
