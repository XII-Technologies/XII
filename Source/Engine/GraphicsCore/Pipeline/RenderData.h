#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/Declarations.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

/// \brief A strongly typed identifier for a render data category.
struct XII_GRAPHICSCORE_DLL xiiRenderDataCategory
{
  xiiUInt16 m_uiValue = 0xFFFF;

  XII_ALWAYS_INLINE bool IsValid() const { return m_uiValue != 0xFFFF; }
  XII_ALWAYS_INLINE bool operator==(const xiiRenderDataCategory& other) const { return m_uiValue == other.m_uiValue; }
};

inline constexpr xiiRenderDataCategory xiiInvalidRenderDataCategory = xiiRenderDataCategory{};

/// \brief Bitflags that describe where a render-data instance participates in type + predicate pass routing.
struct XII_GRAPHICSCORE_DLL xiiRenderDataRoutingFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None = 0,

    Light             = XII_BIT(0),
    Decal             = XII_BIT(1),
    ReflectionProbe   = XII_BIT(2),
    Sky               = XII_BIT(3),
    Opaque            = XII_BIT(4),
    Masked            = XII_BIT(5),
    Transparent       = XII_BIT(6),
    Foreground        = XII_BIT(7),
    ScreenFX          = XII_BIT(8),
    SimpleOpaque      = XII_BIT(9),
    SimpleTransparent = XII_BIT(10),
    Selection         = XII_BIT(11),
    GUI               = XII_BIT(12),

    Default = None
  };

  struct Bits
  {
    StorageType Light : 1;
    StorageType Decal : 1;
    StorageType ReflectionProbe : 1;
    StorageType Sky : 1;
    StorageType Opaque : 1;
    StorageType Masked : 1;
    StorageType Transparent : 1;
    StorageType Foreground : 1;
    StorageType ScreenFX : 1;
    StorageType SimpleOpaque : 1;
    StorageType SimpleTransparent : 1;
    StorageType Selection : 1;
    StorageType GUI : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiRenderDataRoutingFlags);

/// \brief Base class for components to push generic render data.
class XII_GRAPHICSCORE_DLL xiiRenderData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderData, xiiReflectedClass);

public:
  using Category = xiiRenderDataCategory;

  struct Caching
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      Never,
      IfStatic,

      Default = Never
    };
  };

  /// \brief Registers a custom category with a given name. Categories are automatically assigned sequential IDs.
  static xiiRenderDataCategory RegisterCategory(xiiStringView sCategoryName);

  /// \brief Finds an existing category by name. Note that this requires a string lookup.
  static xiiRenderDataCategory FindCategory(xiiStringView sCategoryName);

  /// \brief Returns the name of the given category.
  static xiiStringView GetCategoryName(xiiRenderDataCategory category);

  /// \brief Returns an array of all known category names. The index in the array matches the category ID.
  static const xiiArrayPtr<xiiStringView> GetAllCategoryNames();

  /// \brief Clears all registered categories. Should be called during engine shutdown.
  static void ClearAllCategories();

  /// \brief Converts a legacy category value into route flags used by type + predicate pass selection.
  static xiiBitflags<xiiRenderDataRoutingFlags> RoutingFlagsFromLegacyCategory(xiiRenderDataCategory category);

  xiiRenderData()          = default;
  virtual ~xiiRenderData() = default;

  xiiMat4              m_GlobalTransform = xiiMat4::MakeIdentity();
  xiiBoundingBoxSphere m_GlobalBounds;

  xiiGameObjectHandle m_hOwnerObject;
  xiiComponentHandle  m_hOwnerComponent;

  xiiUInt64                               m_uiSortingKey = 0;
  xiiBitflags<xiiRenderDataRoutingFlags> m_RoutingFlags;
};
