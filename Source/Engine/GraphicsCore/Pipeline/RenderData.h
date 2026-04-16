#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/Declarations.h>
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

  xiiRenderData()          = default;
  virtual ~xiiRenderData() = default;

  xiiMat4              m_GlobalTransform = xiiMat4::MakeIdentity();
  xiiBoundingBoxSphere m_GlobalBounds;

  xiiGameObjectHandle m_hOwnerObject;
  xiiComponentHandle  m_hOwnerComponent;

  xiiUInt64             m_uiSortingKey = 0;
  xiiRenderDataCategory m_Category;
};

/// \brief Structure containing the standard predefined categories.
struct XII_GRAPHICSCORE_DLL xiiDefaultRenderDataCategories
{
  static xiiRenderDataCategory Light;
  static xiiRenderDataCategory Decal;
  static xiiRenderDataCategory ReflectionProbe;
  static xiiRenderDataCategory Sky;
  static xiiRenderDataCategory OpaqueStatic;
  static xiiRenderDataCategory OpaqueDynamic;
  static xiiRenderDataCategory Opaque;
  static xiiRenderDataCategory MaskedStatic;
  static xiiRenderDataCategory MaskedDynamic;
  static xiiRenderDataCategory Masked;
  static xiiRenderDataCategory Transparent;
  static xiiRenderDataCategory Foreground;
  static xiiRenderDataCategory ScreenFX;
  static xiiRenderDataCategory SimpleOpaque;
  static xiiRenderDataCategory SimpleTransparent;
  static xiiRenderDataCategory Selection;
  static xiiRenderDataCategory GUI;

  /// \brief Registers the default categories internally. Called by the renderer startup.
  static void RegisterDefaultCategories();
};
