#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Base class of all attributes can be used to decorate a RTTI property.
class XII_FOUNDATION_DLL xiiPropertyAttribute : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPropertyAttribute, xiiReflectedClass);
};

/// \brief A property attribute that indicates that the property may not be modified through the UI
class XII_FOUNDATION_DLL xiiReadOnlyAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiReadOnlyAttribute, xiiPropertyAttribute);
};

/// \brief A property attribute that indicates that the property is not to be shown in the UI
class XII_FOUNDATION_DLL xiiHiddenAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHiddenAttribute, xiiPropertyAttribute);
};

/// \brief A property attribute that indicates that the property is not to be serialized
/// and whatever it points to only exists temporarily while running or in editor.
class XII_FOUNDATION_DLL xiiTemporaryAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTemporaryAttribute, xiiPropertyAttribute);
};

/// \brief Used to categorize types (e.g. add component menu)
class XII_FOUNDATION_DLL xiiCategoryAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCategoryAttribute, xiiPropertyAttribute);

public:
  xiiCategoryAttribute() = default;
  xiiCategoryAttribute(const char* szCategory) :
    m_sCategory(szCategory)
  {
  }

  const char* GetCategory() const { return m_sCategory; }

private:
  xiiUntrackedString m_sCategory;
};

/// \brief A property attribute that indicates that this feature is still in development and should not be shown to all users.
class XII_FOUNDATION_DLL xiiInDevelopmentAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInDevelopmentAttribute, xiiPropertyAttribute);

public:
  enum Phase
  {
    Alpha,
    Beta
  };

  xiiInDevelopmentAttribute() = default;
  xiiInDevelopmentAttribute(xiiInt32 iPhase) :
    m_Phase(iPhase)
  {
  }

  const char* GetString() const;

  xiiInt32 m_Phase = Phase::Beta;
};

/// \brief Used for dynamic titles of visual script nodes.
/// E.g. "Set Bool Property '{Name}'" will allow the title to by dynamic
/// by reading the current value of the 'Name' property.
class XII_FOUNDATION_DLL xiiTitleAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTitleAttribute, xiiPropertyAttribute);

public:
  xiiTitleAttribute() = default;
  xiiTitleAttribute(const char* szTitle) :
    m_sTitle(szTitle)
  {
  }

  const char* GetTitle() const { return m_sTitle; }

private:
  xiiUntrackedString m_sTitle;
};

/// \brief Used to colorize types
class XII_FOUNDATION_DLL xiiColorAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiColorAttribute, xiiPropertyAttribute);

public:
  xiiColorAttribute() = default;
  xiiColorAttribute(const xiiColor& color) :
    m_Color(color)
  {
  }

  const xiiColor& GetColor() const { return m_Color; }

private:
  xiiColor m_Color;
};

/// \brief A property attribute that indicates that the alpha channel of a xiiColorGammaUB or xiiColor should be exposed in the UI.
class XII_FOUNDATION_DLL xiiExposeColorAlphaAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExposeColorAlphaAttribute, xiiPropertyAttribute);
};

/// \brief Used for any property shown as a line edit (int, float, vector etc).
class XII_FOUNDATION_DLL xiiSuffixAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSuffixAttribute, xiiPropertyAttribute);

public:
  xiiSuffixAttribute() = default;
  xiiSuffixAttribute(const char* szSuffix) :
    m_sSuffix(szSuffix)
  {
  }

  const char* GetSuffix() const { return m_sSuffix; }

private:
  xiiUntrackedString m_sSuffix;
};

/// \brief Used to show a text instead of the minimum value of a property.
class XII_FOUNDATION_DLL xiiMinValueTextAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMinValueTextAttribute, xiiPropertyAttribute);

public:
  xiiMinValueTextAttribute() = default;
  xiiMinValueTextAttribute(const char* szText) :
    m_sText(szText)
  {
  }

  const char* GetText() const { return m_sText; }

private:
  xiiUntrackedString m_sText;
};

/// \brief Sets the default value of the property.
class XII_FOUNDATION_DLL xiiDefaultValueAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDefaultValueAttribute, xiiPropertyAttribute);

public:
  xiiDefaultValueAttribute() = default;
  xiiDefaultValueAttribute(const xiiVariant& value) :
    m_Value(value)
  {
  }

  const xiiVariant& GetValue() const { return m_Value; }

private:
  xiiVariant m_Value;
};

/// \brief A property attribute that allows to define min and max values for the UI. Min or max may be set to an invalid variant to indicate
/// unbounded values in one direction.
class XII_FOUNDATION_DLL xiiClampValueAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClampValueAttribute, xiiPropertyAttribute);

public:
  xiiClampValueAttribute() = default;
  xiiClampValueAttribute(const xiiVariant& min, const xiiVariant& max) :
    m_MinValue(min), m_MaxValue(max)
  {
  }

  const xiiVariant& GetMinValue() const { return m_MinValue; }
  const xiiVariant& GetMaxValue() const { return m_MaxValue; }

private:
  xiiVariant m_MinValue;
  xiiVariant m_MaxValue;
};

/// \brief Used to categorize properties into groups
class XII_FOUNDATION_DLL xiiGroupAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGroupAttribute, xiiPropertyAttribute);

public:
  xiiGroupAttribute();
  xiiGroupAttribute(const char* szGroup, float fOrder = -1.0f);
  xiiGroupAttribute(const char* szGroup, const char* szIconName, float fOrder = -1.0f);

  const char* GetGroup() const { return m_sGroup; }
  const char* GetIconName() const { return m_sIconName; }
  float       GetOrder() const { return m_fOrder; }

private:
  xiiUntrackedString m_sGroup;
  xiiUntrackedString m_sIconName;
  float              m_fOrder = -1.0f;
};

/// \brief Derive from this class if you want to define an attribute that replaces the property type widget.
///
/// Using this attribute affects both member properties as well as elements in a container but not the container widget.
/// When creating a property widget, the property grid will look for an attribute of this type and use
/// its type to look for a factory creator in xiiRttiMappedObjectFactory<xiiQtPropertyWidget>.
/// E.g. xiiRttiMappedObjectFactory<xiiQtPropertyWidget>::RegisterCreator(xiiGetStaticRTTI<xiiFileBrowserAttribute>(), FileBrowserCreator);
/// will replace the property widget for all properties that use xiiFileBrowserAttribute.
class XII_FOUNDATION_DLL xiiTypeWidgetAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeWidgetAttribute, xiiPropertyAttribute);
};

/// \brief Derive from this class if you want to define an attribute that replaces the property widget of containers.
///
/// Using this attribute affects the container widget but not container elements.
/// Only derive from this class if you want to replace the container widget itself, in every other case
/// prefer to use xiiTypeWidgetAttribute.
class XII_FOUNDATION_DLL xiiContainerWidgetAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiContainerWidgetAttribute, xiiPropertyAttribute);
};

/// \brief Add this attribute to a tag set member property to make it use the tag set editor
/// and define the categories it will use as a ; separated list of category names.
///
/// Usage: XII_SET_MEMBER_PROPERTY("Tags", m_Tags)->AddAttributes(new xiiTagSetWidgetAttribute("Category1;Category2")),
class XII_FOUNDATION_DLL xiiTagSetWidgetAttribute : public xiiContainerWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTagSetWidgetAttribute, xiiContainerWidgetAttribute);

public:
  xiiTagSetWidgetAttribute() = default;
  xiiTagSetWidgetAttribute(const char* szTagFilter) :
    m_sTagFilter(szTagFilter)
  {
  }

  const char* GetTagFilter() const { return m_sTagFilter; }

private:
  xiiUntrackedString m_sTagFilter;
};

/// \brief This attribute indicates that a widget should not use temporary transactions when changing the value.
class XII_FOUNDATION_DLL xiiNoTemporaryTransactionsAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNoTemporaryTransactionsAttribute, xiiPropertyAttribute);
};

/// \brief Add this attribute to a variant map property to make it map to the exposed parameters
/// of an asset. For this, the member property name of the asset reference needs to be passed in.
/// The exposed parameters of the currently set asset on that property will be used as the source.
///
/// Usage:
/// XII_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new xiiAssetBrowserAttribute("Particle
/// Effect")), XII_MAP_ACCESSOR_PROPERTY("Parameters",...)->AddAttributes(new xiiExposedParametersAttribute("Effect")),
class XII_FOUNDATION_DLL xiiExposedParametersAttribute : public xiiContainerWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExposedParametersAttribute, xiiContainerWidgetAttribute);

public:
  xiiExposedParametersAttribute() = default;
  xiiExposedParametersAttribute(const char* szParametersSource) :
    m_sParametersSource(szParametersSource)
  {
  }

  const char* GetParametersSource() const { return m_sParametersSource; }

private:
  xiiUntrackedString m_sParametersSource;
};

/// \brief Add this attribute to an embedded class or container property to make it retrieve its default values from a dynamic meta info object on an asset.
///
/// The default values are retrieved from the asset meta data of the currently set asset on that property.
///
/// Usage:
/// XII_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new xiiAssetBrowserAttribute("Skeleton")),
///
/// // Use this if the embedded class m_SkeletonMetaData is of type xiiSkeletonMetaData.
/// XII_MEMBER_PROPERTY("SkeletonMetaData", m_SkeletonMetaData)->AddAttributes(new xiiDynamicDefaultValueAttribute("Skeleton", "xiiSkeletonMetaData")),
///
/// // Use this if you don't want embed the entire meta object but just some container of it. In this case the LocalBones container must match in type to the property 'BonesArrayNameInMetaData' in the meta data type 'xiiSkeletonMetaData'.
/// XII_MAP_MEMBER_PROPERTY("LocalBones", m_Bones)->AddAttributes(new xiiDynamicDefaultValueAttribute("Skeleton", "xiiSkeletonMetaData", "BonesArrayNameInMetaData")),
class XII_FOUNDATION_DLL xiiDynamicDefaultValueAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicDefaultValueAttribute, xiiTypeWidgetAttribute);

public:
  xiiDynamicDefaultValueAttribute() = default;
  xiiDynamicDefaultValueAttribute(const char* szClassSource, const char* szClassType, const char* szClassProperty = nullptr) :
    m_sClassSource(szClassSource), m_sClassType(szClassType), m_sClassProperty(szClassProperty)
  {
  }

  const char* GetClassSource() const { return m_sClassSource; }
  const char* GetClassType() const { return m_sClassType; }
  const char* GetClassProperty() const { return m_sClassProperty; }

private:
  xiiUntrackedString m_sClassSource;
  xiiUntrackedString m_sClassType;
  xiiUntrackedString m_sClassProperty;
};


/// \brief Sets the allowed actions on a container.
class XII_FOUNDATION_DLL xiiContainerAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiContainerAttribute, xiiPropertyAttribute);

public:
  xiiContainerAttribute() = default;
  xiiContainerAttribute(bool bCanAdd, bool bCanDelete, bool bCanMove) :
    m_bCanAdd(bCanAdd), m_bCanDelete(bCanDelete), m_bCanMove(bCanMove)
  {
  }

  bool CanAdd() const { return m_bCanAdd; }
  bool CanDelete() const { return m_bCanDelete; }
  bool CanMove() const { return m_bCanMove; }

private:
  bool m_bCanAdd    = false;
  bool m_bCanDelete = false;
  bool m_bCanMove   = false;
};

/// \brief Defines how a reference set by xiiFileBrowserAttribute and xiiAssetBrowserAttribute is treated.
///
/// A few examples to explain the flags:
/// ## Input for a mesh: **Transform | Thumbnail**
/// * The input (e.g. fbx) is obviously needed for transforming the asset.
/// * We also can't generate a thumbnail without it.
/// * But we don't need to package it with the final game as it is not used by the runtime.
///
/// ## Material on a mesh: **Thumbnail | Package**
/// * The default material on a mesh asset is not needed to transform the mesh. As only the material reference is stored in the mesh asset, any changes to the material do not affect the transform output of the mesh.
/// * It is obviously needed for the thumbnail as that is what is displayed in it.
/// * We also need to package this reference as otherwise the runtime would fail to instantiate the mesh without errors.
///
/// ## Surface on hit prefab: **Package**
/// * Transforming a surface is not affected if the prefab it spawns on impact changes. Only the reference is stored.
/// * The set prefab does not show up in the thumbnail so it is not needed.
/// * We do however need to package it or otherwise the runtime would fail to spawn the prefab on impact.
///
/// As a rule of thumb (also the default for each):
/// * xiiFileBrowserAttribute are mostly Transform and Thumbnail.
/// * xiiAssetBrowserAttribute are mostly Thumbnail and Package.
struct xiiDependencyFlags
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    None      = 0,          ///< The reference is not needed for anything in production. An example of this is editor references that are only used at edit time, e.g. a default animation clip for a skeleton.
    Thumbnail = XII_BIT(0), ///< This reference is a dependency to generating a thumbnail. The material references of a mesh for example.
    Transform = XII_BIT(1), ///< This reference is a dependency to transforming this asset. The input model of a mesh for example.
    Package   = XII_BIT(2), ///< This reference is needs to be packaged as it is used at runtime by this asset. All sounds or debris generated on impact of a surface are common examples of this.

    Default = None
  };

  struct Bits
  {
    StorageType Thumbnail : 1;
    StorageType Transform : 1;
    StorageType Package : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiDependencyFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiDependencyFlags);

/// \brief A property attribute that indicates that the string property should display a file browsing button.
///
/// Allows to specify the title for the browse dialog and the allowed file types.
/// Usage: XII_MEMBER_PROPERTY("File", m_sFilePath)->AddAttributes(new xiiFileBrowserAttribute("Choose a File", "*.txt")),
class XII_FOUNDATION_DLL xiiFileBrowserAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFileBrowserAttribute, xiiTypeWidgetAttribute);

public:
  // Predefined common type filters
  static constexpr const char* Meshes            = "*.obj;*.fbx;*.gltf;*.glb";
  static constexpr const char* SkeletalMeshes    = "*.fbx;*.gltf;*.glb";
  static constexpr const char* ImagesLdrOnly     = "*.dds;*.tga;*.png;*.jpg;*.jpeg";
  static constexpr const char* ImagesHdrOnly     = "*.hdr;*.exr";
  static constexpr const char* ImagesLdrAndHdr   = "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr;*.exr";
  static constexpr const char* CubemapsLdrAndHdr = "*.dds;*.hdr";

  xiiFileBrowserAttribute() = default;
  xiiFileBrowserAttribute(const char* szDialogTitle, const char* szTypeFilter, const char* szCustomAction = nullptr, xiiBitflags<xiiDependencyFlags> dependencyFlags = xiiDependencyFlags::Transform | xiiDependencyFlags::Thumbnail) :
    m_sDialogTitle(szDialogTitle), m_sTypeFilter(szTypeFilter), m_sCustomAction(szCustomAction), m_DependencyFlags(dependencyFlags)
  {
  }

  const char*                     GetDialogTitle() const { return m_sDialogTitle; }
  const char*                     GetTypeFilter() const { return m_sTypeFilter; }
  const char*                     GetCustomAction() const { return m_sCustomAction; }
  xiiBitflags<xiiDependencyFlags> GetDependencyFlags() const { return m_DependencyFlags; }

private:
  xiiUntrackedString              m_sDialogTitle;
  xiiUntrackedString              m_sTypeFilter;
  xiiUntrackedString              m_sCustomAction;
  xiiBitflags<xiiDependencyFlags> m_DependencyFlags;
};

/// \brief A property attribute that indicates that the string property is actually an asset reference.
///
/// Allows to specify the allowed asset types, separated with ;
/// Usage: XII_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new xiiAssetBrowserAttribute("Texture 2D;Texture 3D")),
class XII_FOUNDATION_DLL xiiAssetBrowserAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAssetBrowserAttribute, xiiTypeWidgetAttribute);

public:
  xiiAssetBrowserAttribute() = default;
  xiiAssetBrowserAttribute(const char* szTypeFilter, xiiBitflags<xiiDependencyFlags> dependencyFlags = xiiDependencyFlags::Thumbnail | xiiDependencyFlags::Package) :
    m_DependencyFlags(dependencyFlags)
  {
    SetTypeFilter(szTypeFilter);
  }

  void SetTypeFilter(const char* szTypeFilter)
  {
    xiiStringBuilder sTemp(";", szTypeFilter, ";");
    m_sTypeFilter = sTemp;
  }
  const char*                     GetTypeFilter() const { return m_sTypeFilter; }
  xiiBitflags<xiiDependencyFlags> GetDependencyFlags() const { return m_DependencyFlags; }

private:
  xiiUntrackedString              m_sTypeFilter;
  xiiBitflags<xiiDependencyFlags> m_DependencyFlags;
};

/// \brief Can be used on integer properties to display them as enums. The valid enum values and their names may change at runtime.
///
/// See xiiDynamicEnum for details.
class XII_FOUNDATION_DLL xiiDynamicEnumAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicEnumAttribute, xiiTypeWidgetAttribute);

public:
  xiiDynamicEnumAttribute() = default;
  xiiDynamicEnumAttribute(const char* szDynamicEnumName) :
    m_sDynamicEnumName(szDynamicEnumName)
  {
  }

  const char* GetDynamicEnumName() const { return m_sDynamicEnumName; }

private:
  xiiUntrackedString m_sDynamicEnumName;
};

/// \brief Can be used on string properties to display them as enums. The valid enum values and their names may change at runtime.
///
/// See xiiDynamicStringEnum for details.
class XII_FOUNDATION_DLL xiiDynamicStringEnumAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicStringEnumAttribute, xiiTypeWidgetAttribute);

public:
  xiiDynamicStringEnumAttribute() = default;
  xiiDynamicStringEnumAttribute(const char* szDynamicEnumName) :
    m_sDynamicEnumName(szDynamicEnumName)
  {
  }

  const char* GetDynamicEnumName() const { return m_sDynamicEnumName; }

private:
  xiiUntrackedString m_sDynamicEnumName;
};


//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiManipulatorAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiManipulatorAttribute, xiiPropertyAttribute);

public:
  xiiManipulatorAttribute(const char* szProperty1, const char* szProperty2 = nullptr, const char* szProperty3 = nullptr, const char* szProperty4 = nullptr, const char* szProperty5 = nullptr, const char* szProperty6 = nullptr);

  xiiUntrackedString m_sProperty1;
  xiiUntrackedString m_sProperty2;
  xiiUntrackedString m_sProperty3;
  xiiUntrackedString m_sProperty4;
  xiiUntrackedString m_sProperty5;
  xiiUntrackedString m_sProperty6;
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiSphereManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSphereManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiSphereManipulatorAttribute();
  xiiSphereManipulatorAttribute(const char* szOuterRadiusProperty, const char* szInnerRadiusProperty = nullptr);

  const xiiUntrackedString& GetOuterRadiusProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetInnerRadiusProperty() const { return m_sProperty2; }
};


//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiCapsuleManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCapsuleManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiCapsuleManipulatorAttribute();
  xiiCapsuleManipulatorAttribute(const char* szHeightProperty, const char* szRadiusProperty);

  const xiiUntrackedString& GetLengthProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
};


//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiBoxManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBoxManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiBoxManipulatorAttribute();
  xiiBoxManipulatorAttribute(const char* szSizeProperty, float fSizeScale, bool bRecenterParent, const char* szOffsetProperty = nullptr, const char* szRotationProperty = nullptr);

  bool  m_bRecenterParent = false;
  float m_fSizeScale      = 1.0f;

  const xiiUntrackedString& GetSizeProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetOffsetProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetRotationProperty() const { return m_sProperty3; }
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiNonUniformBoxManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNonUniformBoxManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiNonUniformBoxManipulatorAttribute();
  xiiNonUniformBoxManipulatorAttribute(
    const char* szNegXProp,
    const char* szPosXProp,
    const char* szNegYProp,
    const char* szPosYProp,
    const char* szNegZProp,
    const char* szPosZProp);
  xiiNonUniformBoxManipulatorAttribute(const char* szSizeX, const char* szSizeY, const char* szSizeZ);

  bool HasSixAxis() const { return !m_sProperty4.IsEmpty(); }

  const xiiUntrackedString& GetNegXProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetPosXProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetNegYProperty() const { return m_sProperty3; }
  const xiiUntrackedString& GetPosYProperty() const { return m_sProperty4; }
  const xiiUntrackedString& GetNegZProperty() const { return m_sProperty5; }
  const xiiUntrackedString& GetPosZProperty() const { return m_sProperty6; }

  const xiiUntrackedString& GetSizeXProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetSizeYProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetSizeZProperty() const { return m_sProperty3; }
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiConeLengthManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiConeLengthManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiConeLengthManipulatorAttribute();
  xiiConeLengthManipulatorAttribute(const char* szRadiusProperty);

  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty1; }
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiConeAngleManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiConeAngleManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiConeAngleManipulatorAttribute();
  xiiConeAngleManipulatorAttribute(const char* szAngleProperty, float fScale = 1.0f, const char* szRadiusProperty = nullptr);

  const xiiUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty2; }

  float m_fScale = 1.0f;
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiTransformManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTransformManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiTransformManipulatorAttribute();
  xiiTransformManipulatorAttribute(const char* szTranslateProperty, const char* szRotateProperty = nullptr, const char* szScaleProperty = nullptr, const char* szOffsetTranslation = nullptr, const char* szOffsetRotation = nullptr);

  const xiiUntrackedString& GetTranslateProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetRotateProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetScaleProperty() const { return m_sProperty3; }
  const xiiUntrackedString& GetGetOffsetTranslationProperty() const { return m_sProperty4; }
  const xiiUntrackedString& GetGetOffsetRotationProperty() const { return m_sProperty5; }
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiBoneManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBoneManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiBoneManipulatorAttribute();
  xiiBoneManipulatorAttribute(const char* szTransformProperty, const char* szBindTo);

  const xiiUntrackedString& GetTransformProperty() const { return m_sProperty1; }
};

//////////////////////////////////////////////////////////////////////////

struct xiiVisualizerAnchor
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Center = 0,
    PosX   = XII_BIT(0),
    NegX   = XII_BIT(1),
    PosY   = XII_BIT(2),
    NegY   = XII_BIT(3),
    PosZ   = XII_BIT(4),
    NegZ   = XII_BIT(5),

    Default = Center
  };

  struct Bits
  {
    StorageType PosX : 1;
    StorageType NegX : 1;
    StorageType PosY : 1;
    StorageType NegY : 1;
    StorageType PosZ : 1;
    StorageType NegZ : 1;
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiVisualizerAnchor);

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiVisualizerAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualizerAttribute, xiiPropertyAttribute);

public:
  xiiVisualizerAttribute(const char* szProperty1, const char* szProperty2 = nullptr, const char* szProperty3 = nullptr, const char* szProperty4 = nullptr, const char* szProperty5 = nullptr);

  xiiUntrackedString               m_sProperty1;
  xiiUntrackedString               m_sProperty2;
  xiiUntrackedString               m_sProperty3;
  xiiUntrackedString               m_sProperty4;
  xiiUntrackedString               m_sProperty5;
  xiiBitflags<xiiVisualizerAnchor> m_Anchor;
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiBoxVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBoxVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiBoxVisualizerAttribute();
  xiiBoxVisualizerAttribute(const char* szSizeProperty, float fSizeScale = 1.0f, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), const char* szColorProperty = nullptr, xiiBitflags<xiiVisualizerAnchor> anchor = xiiVisualizerAnchor::Center, xiiVec3 vOffsetOrScale = xiiVec3::ZeroVector(), const char* szOffsetProperty = nullptr, const char* szRotationProperty = nullptr);

  const xiiUntrackedString& GetSizeProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetColorProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetOffsetProperty() const { return m_sProperty3; }
  const xiiUntrackedString& GetRotationProperty() const { return m_sProperty4; }

  float    m_fSizeScale = 1.0f;
  xiiColor m_Color;
  xiiVec3  m_vOffsetOrScale;
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiSphereVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSphereVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiSphereVisualizerAttribute();
  xiiSphereVisualizerAttribute(const char* szRadiusProperty, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), const char* szColorProperty = nullptr, xiiBitflags<xiiVisualizerAnchor> anchor = xiiVisualizerAnchor::Center, xiiVec3 vOffsetOrScale = xiiVec3::ZeroVector(), const char* szOffsetProperty = nullptr);

  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetColorProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetOffsetProperty() const { return m_sProperty3; }

  xiiColor m_Color;
  xiiVec3  m_vOffsetOrScale;
};


//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiCapsuleVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCapsuleVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiCapsuleVisualizerAttribute();
  xiiCapsuleVisualizerAttribute(const char* szHeightProperty, const char* szRadiusProperty, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), const char* szColorProperty = nullptr, xiiBitflags<xiiVisualizerAnchor> anchor = xiiVisualizerAnchor::Center);

  const xiiUntrackedString& GetHeightProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetColorProperty() const { return m_sProperty3; }

  xiiColor m_Color;
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiCylinderVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCylinderVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiCylinderVisualizerAttribute();
  xiiCylinderVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, const char* szHeightProperty, const char* szRadiusProperty, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), const char* szColorProperty = nullptr, xiiBitflags<xiiVisualizerAnchor> anchor = xiiVisualizerAnchor::Center, xiiVec3 vOffsetOrScale = xiiVec3::ZeroVector(), const char* szOffsetProperty = nullptr);
  xiiCylinderVisualizerAttribute(const char* szAxisProperty, const char* szHeightProperty, const char* szRadiusProperty, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), const char* szColorProperty = nullptr, xiiBitflags<xiiVisualizerAnchor> anchor = xiiVisualizerAnchor::Center, xiiVec3 vOffsetOrScale = xiiVec3::ZeroVector(), const char* szOffsetProperty = nullptr);

  const xiiUntrackedString& GetAxisProperty() const { return m_sProperty5; }
  const xiiUntrackedString& GetHeightProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetColorProperty() const { return m_sProperty3; }
  const xiiUntrackedString& GetOffsetProperty() const { return m_sProperty4; }

  xiiColor              m_Color;
  xiiVec3               m_vOffsetOrScale;
  xiiEnum<xiiBasisAxis> m_Axis;
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiDirectionVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDirectionVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiDirectionVisualizerAttribute();
  xiiDirectionVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, float fScale, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), const char* szColorProperty = nullptr, const char* szLengthProperty = nullptr);
  xiiDirectionVisualizerAttribute(const char* szAxisProperty, float fScale, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), const char* szColorProperty = nullptr, const char* szLengthProperty = nullptr);

  const xiiUntrackedString& GetColorProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetLengthProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetAxisProperty() const { return m_sProperty3; }

  xiiEnum<xiiBasisAxis> m_Axis;
  xiiColor              m_Color;
  float                 m_fScale;
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiConeVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiConeVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiConeVisualizerAttribute();

  /// \brief Attribute to add on an RTTI type to add a cone visualizer for specific properties.
  ///
  /// szRadiusProperty may be nullptr, in which case it is assumed to be 1
  /// fScale will be multiplied with value of szRadiusProperty to determine the size of the cone
  /// szColorProperty may be nullptr. In this case it is ignored and fixedColor is used instead.
  /// fixedColor is ignored if szColorProperty is valid.
  xiiConeVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, const char* szAngleProperty, float fScale, const char* szRadiusProperty, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), const char* szColorProperty = nullptr);

  const xiiUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetColorProperty() const { return m_sProperty3; }

  xiiEnum<xiiBasisAxis> m_Axis;
  xiiColor              m_Color;
  float                 m_fScale;
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiCameraVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCameraVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiCameraVisualizerAttribute();

  /// \brief Attribute to add on an RTTI type to add a camera cone visualizer.
  xiiCameraVisualizerAttribute(const char* szModeProperty, const char* szFovProperty, const char* szOrthoDimProperty, const char* szNearPlaneProperty, const char* szFarPlaneProperty);

  const xiiUntrackedString& GetModeProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetFovProperty() const { return m_sProperty2; }
  const xiiUntrackedString& GetOrthoDimProperty() const { return m_sProperty3; }
  const xiiUntrackedString& GetNearPlaneProperty() const { return m_sProperty4; }
  const xiiUntrackedString& GetFarPlaneProperty() const { return m_sProperty5; }
};

//////////////////////////////////////////////////////////////////////////

// Implementation moved here as it requires xiiPropertyAttribute to be fully defined.
template <typename Type>
const Type* xiiRTTI::GetAttributeByType() const
{
  for (const auto* pAttr : m_Attributes)
  {
    if (pAttr->GetDynamicRTTI()->IsDerivedFrom<Type>())
      return static_cast<const Type*>(pAttr);
  }
  if (GetParentType() != nullptr)
    return GetParentType()->GetAttributeByType<Type>();
  else
    return nullptr;
}

template <typename Type>
const Type* xiiAbstractProperty::GetAttributeByType() const
{
  for (const auto* pAttr : m_Attributes)
  {
    if (pAttr->GetDynamicRTTI()->IsDerivedFrom<Type>())
      return static_cast<const Type*>(pAttr);
  }
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

/// \brief A property attribute that specifies the max size of an array. If it is reached, no further elemets are allowed to be added.
class XII_FOUNDATION_DLL xiiMaxArraySizeAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaxArraySizeAttribute, xiiPropertyAttribute);

public:
  xiiMaxArraySizeAttribute() = default;
  xiiMaxArraySizeAttribute(xiiUInt32 uiMaxSize) :
    m_uiMaxSize(uiMaxSize)
  {
  }

  const xiiUInt32& GetMaxSize() const { return m_uiMaxSize; }

private:
  xiiUInt32 m_uiMaxSize = 0;
};

//////////////////////////////////////////////////////////////////////////

/// \brief If this attribute is set, the UI is encouraged to prevent the user from creating duplicates of the same thing.
///
/// For arrays of objects this means that multiple objects of the same type are not allowed.
class XII_FOUNDATION_DLL xiiPreventDuplicatesAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPreventDuplicatesAttribute, xiiPropertyAttribute);

public:
  xiiPreventDuplicatesAttribute() = default;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Attribute for xiiMessages to instruct the visual script framework to automatically generate a node for sending this type of
/// message
class XII_FOUNDATION_DLL xiiAutoGenVisScriptMsgSender : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAutoGenVisScriptMsgSender, xiiPropertyAttribute);
};

/// \brief Attribute for xiiMessages to instruct the visual script framework to automatically generate a node for handling this type of
/// message
class XII_FOUNDATION_DLL xiiAutoGenVisScriptMsgHandler : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAutoGenVisScriptMsgHandler, xiiPropertyAttribute);
};

/// \brief Attribute to mark a function up to be exposed to the scripting system. Arguments specify the names of the function parameters.
class XII_FOUNDATION_DLL xiiScriptableFunctionAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScriptableFunctionAttribute, xiiPropertyAttribute);

  enum ArgType : xiiUInt8
  {
    In,
    Out,
    Inout
  };

  xiiScriptableFunctionAttribute(ArgType argType1 = In, const char* szArg1 = nullptr, ArgType argType2 = In, const char* szArg2 = nullptr, ArgType argType3 = In, const char* szArg3 = nullptr, ArgType argType4 = In, const char* szArg4 = nullptr, ArgType argType5 = In, const char* szArg5 = nullptr, ArgType argType6 = In, const char* szArg6 = nullptr);

  const char* GetArgumentName(xiiUInt32 uiIndex) const { return m_ArgNames[uiIndex]; }

  ArgType GetArgumentType(xiiUInt32 uiIndex) const { return static_cast<ArgType>(m_ArgTypes[uiIndex]); };

private:
  xiiHybridArray<xiiUntrackedString, 6> m_ArgNames;
  xiiHybridArray<xiiUInt8, 6>           m_ArgTypes;
};

/// \brief Used to annotate properties to which pin or function parameter they belong (if necessary)
class XII_FOUNDATION_DLL xiiVisScriptMappingAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisScriptMappingAttribute, xiiPropertyAttribute);

  xiiVisScriptMappingAttribute() = default;
  xiiVisScriptMappingAttribute(xiiInt32 iMapping) :
    m_iMapping(iMapping)
  {
  }

  xiiInt32 m_iMapping = 0;
};

/// \brief Used to mark an array or (unsigned)int property as source for dynamic pin generation on nodes
class XII_FOUNDATION_DLL xiiDynamicPinAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicPinAttribute, xiiPropertyAttribute);

public:
  xiiDynamicPinAttribute() = default;
  xiiDynamicPinAttribute(const char* szProperty);

  const xiiUntrackedString& GetProperty() const { return m_sProperty; }

private:
  xiiUntrackedString m_sProperty;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to mark that a component provides functionality that is executed with a long operation in the editor.
///
/// \a szOpTypeName must be the class name of a class derived from xiiLongOpProxy.
/// Once a component is added to a scene with this attribute, the named long op will appear in the UI and can be executed.
///
/// The automatic registration is done by xiiLongOpsAdapter
class XII_FOUNDATION_DLL xiiLongOpAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLongOpAttribute, xiiPropertyAttribute);

public:
  xiiLongOpAttribute() = default;
  xiiLongOpAttribute(const char* szOpTypeName) :
    m_sOpTypeName(szOpTypeName)
  {
  }

  xiiUntrackedString m_sOpTypeName;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A property attribute that indicates that the string property is actually a game object reference.
class XII_FOUNDATION_DLL xiiGameObjectReferenceAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectReferenceAttribute, xiiTypeWidgetAttribute);

public:
  xiiGameObjectReferenceAttribute() = default;
};
