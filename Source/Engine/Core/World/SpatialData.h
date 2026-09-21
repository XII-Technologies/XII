/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Strings/HashedString.h>

struct xiiSpatialData
{
  struct Flags
  {
    using StorageType = xiiUInt8;

    enum Enum
    {
      None            = 0,
      FrequentChanges = XII_BIT(0), ///< Indicates that objects in this category change their bounds frequently. Spatial System implementations can use that as hint for internal optimizations.

      Default = None
    };

    struct Bits
    {
      StorageType FrequentUpdates : 1;
    };
  };

  struct Category
  {
    XII_ALWAYS_INLINE Category() :
      m_uiValue(xiiSmallInvalidIndex)
    {
    }

    XII_ALWAYS_INLINE explicit Category(xiiUInt16 uiValue) :
      m_uiValue(uiValue)
    {
    }

    XII_ALWAYS_INLINE bool operator==(const Category& other) const { return m_uiValue == other.m_uiValue; }
    XII_ALWAYS_INLINE bool operator!=(const Category& other) const { return m_uiValue != other.m_uiValue; }

    xiiUInt16 m_uiValue;

    XII_ALWAYS_INLINE xiiUInt32 GetBitmask() const { return m_uiValue != xiiSmallInvalidIndex ? static_cast<xiiUInt32>(XII_BIT(m_uiValue)) : 0; }
  };

  /// Registers a spatial data category under the given name.
  ///
  /// If the same category was already registered before, it returns that instead.
  /// Asserts that there are no more than 32 unique categories.
  XII_CORE_DLL static Category RegisterCategory(xiiStringView sCategoryName, const xiiBitflags<Flags>& flags);

  /// Returns either an existing category with the given name or xiiInvalidSpatialDataCategory.
  XII_CORE_DLL static Category FindCategory(xiiStringView sCategoryName);

  /// Returns the name of the given category.
  XII_CORE_DLL static const xiiHashedString& GetCategoryName(Category category);

  /// Returns the flags for the given category.
  XII_CORE_DLL static const xiiBitflags<Flags>& GetCategoryFlags(Category category);

private:
  struct CategoryData
  {
    xiiHashedString    m_sName;
    xiiBitflags<Flags> m_Flags;
  };

  static xiiHybridArray<xiiSpatialData::CategoryData, 32>& GetCategoryData();
};

struct XII_CORE_DLL xiiDefaultSpatialDataCategories
{
  static xiiSpatialData::Category RenderStatic;
  static xiiSpatialData::Category RenderDynamic;
  static xiiSpatialData::Category OcclusionStatic;
  static xiiSpatialData::Category OcclusionDynamic;
};

/// When an object is 'seen' by a view and thus tagged as 'visible', this enum describes what kind of observer triggered this.
///
/// This is used to determine how important certain updates, such as animations, are to execute.
/// E.g. when a 'shadow view' or 'reflection view' is the only thing that observes an object, animations / particle effects and so on,
/// can be updated less frequently.
struct xiiVisibilityState
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Invisible = 0, ///< The object isn't visible to any view.
    Indirect  = 1, ///< The object is seen by a view that only indirectly makes the object visible (shadow / reflection / render target).
    Direct    = 2, ///< The object is seen directly by a main view and therefore it needs to be updated at maximum frequency.

    Default = Invisible
  };
};

#define xiiInvalidSpatialDataCategory xiiSpatialData::Category()
