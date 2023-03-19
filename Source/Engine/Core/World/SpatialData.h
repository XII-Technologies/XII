#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Strings/HashedString.h>

struct xiiSpatialData
{
  struct Flags
  {
    typedef xiiUInt8 StorageType;

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
      m_uiValue(xiiInvalidIndex)
    {
    }

    XII_ALWAYS_INLINE explicit Category(xiiUInt32 uiValue) :
      m_uiValue(uiValue)
    {
    }

    XII_ALWAYS_INLINE bool operator==(const Category& other) const { return m_uiValue == other.m_uiValue; }
    XII_ALWAYS_INLINE bool operator!=(const Category& other) const { return m_uiValue != other.m_uiValue; }

    xiiUInt32 m_uiValue;

    XII_ALWAYS_INLINE xiiUInt32 GetBitmask() const { return m_uiValue != xiiInvalidIndex ? static_cast<xiiUInt32>(XII_BIT(m_uiValue)) : 0; }
  };

  /// \brief Registers a spatial data category under the given name.
  ///
  /// If the same category was already registered before, it returns that instead.
  /// Asserts that there are no more than 32 unique categories.
  XII_CORE_DLL static Category RegisterCategory(xiiStringView sCategoryName, const xiiBitflags<Flags>& flags);

  /// \brief Returns either an existing category with the given name or xiiInvalidSpatialDataCategory.
  XII_CORE_DLL static Category FindCategory(xiiStringView sCategoryName);

  /// \brief Returns the flags for the given category.
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

#define xiiInvalidSpatialDataCategory xiiSpatialData::Category()
