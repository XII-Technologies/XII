/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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
  xiiCategoryAttribute(xiiStringView sCategory) :
    m_sCategory(sCategory)
  {
  }

  xiiStringView GetCategory() const { return m_sCategory; }

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

  xiiStringView GetString() const;

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
  xiiTitleAttribute(xiiStringView sTitle) :
    m_sTitle(sTitle)
  {
  }

  xiiStringView GetTitle() const { return m_sTitle; }

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
  xiiSuffixAttribute(xiiStringView sSuffix) :
    m_sSuffix(sSuffix)
  {
  }

  xiiStringView GetSuffix() const { return m_sSuffix; }

private:
  xiiUntrackedString m_sSuffix;
};

/// \brief Used to show a text instead of the minimum value of a property.
class XII_FOUNDATION_DLL xiiMinValueTextAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMinValueTextAttribute, xiiPropertyAttribute);

public:
  xiiMinValueTextAttribute() = default;
  xiiMinValueTextAttribute(xiiStringView sText) :
    m_sText(sText)
  {
  }

  xiiStringView GetText() const { return m_sText; }

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

  xiiDefaultValueAttribute(xiiInt8 value) :
    m_Value(value)
  {
  }

  xiiDefaultValueAttribute(xiiUInt8 value) :
    m_Value(value)
  {
  }

  xiiDefaultValueAttribute(xiiInt16 value) :
    m_Value(value)
  {
  }

  xiiDefaultValueAttribute(xiiUInt16 value) :
    m_Value(value)
  {
  }

  xiiDefaultValueAttribute(xiiInt32 value) :
    m_Value(value)
  {
  }

  xiiDefaultValueAttribute(xiiUInt32 value) :
    m_Value(value)
  {
  }

  xiiDefaultValueAttribute(xiiInt64 value) :
    m_Value(value)
  {
  }

  xiiDefaultValueAttribute(xiiUInt64 value) :
    m_Value(value)
  {
  }

  xiiDefaultValueAttribute(float value) :
    m_Value(value)
  {
  }

  xiiDefaultValueAttribute(double value) :
    m_Value(value)
  {
  }

  xiiDefaultValueAttribute(xiiStringView value) :
    m_Value(xiiVariant(value, false))
  {
  }

  xiiDefaultValueAttribute(const char* value) :
    m_Value(xiiVariant(xiiStringView(value), false))
  {
  }

  template <typename T>
  xiiDefaultValueAttribute(const xiiBitflags<T>& flags) :
    m_Value(flags.GetValue())
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
  xiiGroupAttribute(xiiStringView sGroup, float fOrder = -1.0f);
  xiiGroupAttribute(xiiStringView sGroup, xiiStringView sIconName, float fOrder = -1.0f);

  xiiStringView GetGroup() const { return m_sGroup; }
  xiiStringView GetIconName() const { return m_sIconName; }
  float         GetOrder() const { return m_fOrder; }

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
  xiiTagSetWidgetAttribute(xiiStringView sTagFilter) :
    m_sTagFilter(sTagFilter)
  {
  }

  xiiStringView GetTagFilter() const { return m_sTagFilter; }

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
  xiiExposedParametersAttribute(xiiStringView sParametersSource) :
    m_sParametersSource(sParametersSource)
  {
  }

  xiiStringView GetParametersSource() const { return m_sParametersSource; }

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
  xiiDynamicDefaultValueAttribute(xiiStringView sClassSource, xiiStringView sClassType, xiiStringView sClassProperty = {}) :
    m_sClassSource(sClassSource), m_sClassType(sClassType), m_sClassProperty(sClassProperty)
  {
  }

  xiiStringView GetClassSource() const { return m_sClassSource; }
  xiiStringView GetClassType() const { return m_sClassType; }
  xiiStringView GetClassProperty() const { return m_sClassProperty; }

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
/// * We do, however, need to package it or otherwise the runtime would fail to spawn the prefab on impact.
///
/// As a rule of thumb (also the default for each):
/// * xiiFileBrowserAttribute are mostly Transform and Thumbnail.
/// * xiiAssetBrowserAttribute are mostly Thumbnail and Package.
struct xiiDependencyFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None      = 0,          ///< The reference is not needed for anything in production. An example of this is editor references that are only used at edit time, e.g. a default animation clip for a skeleton.
    Thumbnail = XII_BIT(0), ///< This reference is a dependency to generating a thumbnail. The material references of a mesh for example.
    Transform = XII_BIT(1), ///< This reference is a dependency to transforming this asset. The input model of a mesh for example.
    Package   = XII_BIT(2), ///< This reference needs to be packaged as it is used at runtime by this asset. All sounds or debris generated on impact of a surface are common examples of this.

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
  static constexpr xiiStringView Meshes               = "*.obj;*.fbx;*.gltf;*.glb"_xiisv;
  static constexpr xiiStringView MeshesWithAnimations = "*.fbx;*.gltf;*.glb"_xiisv;
  static constexpr xiiStringView ImagesLdrOnly        = "*.dds;*.tga;*.png;*.jpg;*.jpeg"_xiisv;
  static constexpr xiiStringView ImagesHdrOnly        = "*.hdr;*.exr"_xiisv;
  static constexpr xiiStringView ImagesLdrAndHdr      = "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr;*.exr"_xiisv;
  static constexpr xiiStringView CubemapsLdrAndHdr    = "*.dds;*.hdr"_xiisv;

  xiiFileBrowserAttribute() = default;
  xiiFileBrowserAttribute(xiiStringView sDialogTitle, xiiStringView sTypeFilter, xiiStringView sCustomAction = {}, xiiStringView sCreateTitle = {}, xiiBitflags<xiiDependencyFlags> depencyFlags = xiiDependencyFlags::Transform | xiiDependencyFlags::Thumbnail) :
    m_sDialogTitle(sDialogTitle), m_sTypeFilter(sTypeFilter), m_sCustomAction(sCustomAction), m_sCreateTitle(sCreateTitle), m_DependencyFlags(depencyFlags)
  {
  }

  xiiStringView                   GetDialogTitle() const { return m_sDialogTitle; }
  xiiStringView                   GetTypeFilter() const { return m_sTypeFilter; }
  xiiStringView                   GetCustomAction() const { return m_sCustomAction; }
  xiiStringView                   GetCreateTitle() const { return m_sCreateTitle; }
  xiiBitflags<xiiDependencyFlags> GetDependencyFlags() const { return m_DependencyFlags; }

private:
  xiiUntrackedString              m_sDialogTitle;
  xiiUntrackedString              m_sTypeFilter;
  xiiUntrackedString              m_sCustomAction;
  xiiUntrackedString              m_sCreateTitle;
  xiiBitflags<xiiDependencyFlags> m_DependencyFlags;
};

/// \brief Indicates that the string property should allow to browse for an file (or programs) outside the project directories.
///
/// Allows to specify the title for the browse dialog and the allowed file types.
/// Usage: XII_MEMBER_PROPERTY("File", m_sFilePath)->AddAttributes(new xiiFileBrowserAttribute("Choose a File", "*.exe")),
class XII_FOUNDATION_DLL xiiExternalFileBrowserAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExternalFileBrowserAttribute, xiiTypeWidgetAttribute);

public:
  xiiExternalFileBrowserAttribute() = default;
  xiiExternalFileBrowserAttribute(xiiStringView sDialogTitle, xiiStringView sTypeFilter) :
    m_sDialogTitle(sDialogTitle), m_sTypeFilter(sTypeFilter)
  {
  }

  xiiStringView GetDialogTitle() const { return m_sDialogTitle; }
  xiiStringView GetTypeFilter() const { return m_sTypeFilter; }

private:
  xiiUntrackedString m_sDialogTitle;
  xiiUntrackedString m_sTypeFilter;
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

  xiiAssetBrowserAttribute(xiiStringView sTypeFilter, xiiBitflags<xiiDependencyFlags> depencyFlags = xiiDependencyFlags::Thumbnail | xiiDependencyFlags::Package) :
    m_DependencyFlags(depencyFlags)
  {
    SetTypeFilter(sTypeFilter);
  }

  xiiAssetBrowserAttribute(xiiStringView sTypeFilter, xiiStringView sRequiredTag, xiiBitflags<xiiDependencyFlags> depencyFlags = xiiDependencyFlags::Thumbnail | xiiDependencyFlags::Package) :
    m_DependencyFlags(depencyFlags)
  {
    SetTypeFilter(sTypeFilter);

    m_sRequiredTag = sRequiredTag;
  }

  void SetTypeFilter(xiiStringView sTypeFilter)
  {
    xiiStringBuilder sTemp(";", sTypeFilter, ";");
    m_sTypeFilter = sTemp;
  }

  xiiStringView                   GetTypeFilter() const { return m_sTypeFilter; }
  xiiBitflags<xiiDependencyFlags> GetDependencyFlags() const { return m_DependencyFlags; }

  xiiStringView GetRequiredTag() const { return m_sRequiredTag; }

private:
  xiiUntrackedString              m_sTypeFilter;
  xiiUntrackedString              m_sRequiredTag;
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
  xiiDynamicEnumAttribute(xiiStringView sDynamicEnumName) :
    m_sDynamicEnumName(sDynamicEnumName)
  {
  }

  xiiStringView GetDynamicEnumName() const { return m_sDynamicEnumName; }

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
  xiiDynamicStringEnumAttribute(xiiStringView sDynamicEnumName) :
    m_sDynamicEnumName(sDynamicEnumName)
  {
  }

  xiiStringView GetDynamicEnumName() const { return m_sDynamicEnumName; }

private:
  xiiUntrackedString m_sDynamicEnumName;
};

/// \brief Can be used on integer properties to display them as bitflags. The valid bitflags and their names may change at runtime.
class XII_FOUNDATION_DLL xiiDynamicBitflagsAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicBitflagsAttribute, xiiTypeWidgetAttribute);

public:
  xiiDynamicBitflagsAttribute() = default;
  xiiDynamicBitflagsAttribute(xiiStringView sDynamicName) :
    m_sDynamicBitflagsName(sDynamicName)
  {
  }

  xiiStringView GetDynamicBitflagsName() const { return m_sDynamicBitflagsName; }

private:
  xiiUntrackedString m_sDynamicBitflagsName;
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiManipulatorAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiManipulatorAttribute, xiiPropertyAttribute);

public:
  xiiManipulatorAttribute(xiiStringView sProperty1, xiiStringView sProperty2 = {}, xiiStringView sProperty3 = {}, xiiStringView sProperty4 = {}, xiiStringView sProperty5 = {}, xiiStringView sProperty6 = {});

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
  xiiSphereManipulatorAttribute(xiiStringView sOuterRadiusProperty, xiiStringView sInnerRadiusProperty = {});

  const xiiUntrackedString& GetOuterRadiusProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetInnerRadiusProperty() const { return m_sProperty2; }
};


//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiCapsuleManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCapsuleManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiCapsuleManipulatorAttribute();
  xiiCapsuleManipulatorAttribute(xiiStringView sHeightProperty, xiiStringView sRadiusProperty);

  const xiiUntrackedString& GetLengthProperty() const { return m_sProperty1; }
  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
};


//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiBoxManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBoxManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiBoxManipulatorAttribute();
  xiiBoxManipulatorAttribute(xiiStringView sSizeProperty, float fSizeScale, bool bRecenterParent, xiiStringView sOffsetProperty = {}, xiiStringView sRotationProperty = {});

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
  xiiNonUniformBoxManipulatorAttribute(xiiStringView sNegXProp, xiiStringView sPosXProp, xiiStringView sNegYProp, xiiStringView sPosYProp, xiiStringView sNegZProp, xiiStringView sPosZProp);
  xiiNonUniformBoxManipulatorAttribute(xiiStringView sSizeX, xiiStringView sSizeY, xiiStringView sSizeZ);

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
  xiiConeLengthManipulatorAttribute(xiiStringView sRadiusProperty);

  const xiiUntrackedString& GetRadiusProperty() const { return m_sProperty1; }
};

//////////////////////////////////////////////////////////////////////////

class XII_FOUNDATION_DLL xiiConeAngleManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiConeAngleManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiConeAngleManipulatorAttribute();
  xiiConeAngleManipulatorAttribute(xiiStringView sAngleProperty, float fScale = 1.0f, xiiStringView sRadiusProperty = {});

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
  xiiTransformManipulatorAttribute(xiiStringView sTranslateProperty, xiiStringView sRotateProperty = {}, xiiStringView sScaleProperty = {}, xiiStringView sOffsetTranslation = {}, xiiStringView sOffsetRotation = {});

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
  xiiBoneManipulatorAttribute(xiiStringView sTransformProperty, xiiStringView sBindTo);

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
  xiiVisualizerAttribute(xiiStringView sProperty1, xiiStringView sProperty2 = {}, xiiStringView sProperty3 = {}, xiiStringView sProperty4 = {}, xiiStringView sProperty5 = {});

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
  xiiBoxVisualizerAttribute(xiiStringView sSizeProperty, float fSizeScale = 1.0f, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), xiiStringView sColorProperty = {}, xiiBitflags<xiiVisualizerAnchor> anchor = xiiVisualizerAnchor::Center, xiiVec3 vOffsetOrScale = xiiVec3::MakeZero(), xiiStringView sOffsetProperty = {}, xiiStringView sRotationProperty = {});

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
  xiiSphereVisualizerAttribute(xiiStringView sRadiusProperty, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), xiiStringView sColorProperty = {}, xiiBitflags<xiiVisualizerAnchor> anchor = xiiVisualizerAnchor::Center, xiiVec3 vOffsetOrScale = xiiVec3::MakeZero(), xiiStringView sOffsetProperty = {});

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
  xiiCapsuleVisualizerAttribute(xiiStringView sHeightProperty, xiiStringView sRadiusProperty, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), xiiStringView sColorProperty = {}, xiiBitflags<xiiVisualizerAnchor> anchor = xiiVisualizerAnchor::Center);

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
  xiiCylinderVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, xiiStringView sHeightProperty, xiiStringView sRadiusProperty, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), xiiStringView sColorProperty = {}, xiiBitflags<xiiVisualizerAnchor> anchor = xiiVisualizerAnchor::Center, xiiVec3 vOffsetOrScale = xiiVec3::MakeZero(), xiiStringView sOffsetProperty = {});
  xiiCylinderVisualizerAttribute(xiiStringView sAxisProperty, xiiStringView sHeightProperty, xiiStringView sRadiusProperty, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), xiiStringView sColorProperty = {}, xiiBitflags<xiiVisualizerAnchor> anchor = xiiVisualizerAnchor::Center, xiiVec3 vOffsetOrScale = xiiVec3::MakeZero(), xiiStringView sOffsetProperty = {});

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
  xiiDirectionVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, float fScale, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), xiiStringView sColorProperty = {}, xiiStringView sLengthProperty = {});
  xiiDirectionVisualizerAttribute(xiiStringView sAxisProperty, float fScale, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), xiiStringView sColorProperty = {}, xiiStringView sLengthProperty = {});

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
  /// sRadiusProperty may be nullptr, in which case it is assumed to be 1
  /// fScale will be multiplied with value of sRadiusProperty to determine the size of the cone
  /// sColorProperty may be nullptr. In this case it is ignored and fixedColor is used instead.
  /// fixedColor is ignored if sColorProperty is valid.
  xiiConeVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, xiiStringView sAngleProperty, float fScale, xiiStringView sRadiusProperty, const xiiColor& fixedColor = xiiColorScheme::LightUI(xiiColorScheme::Grape), xiiStringView sColorProperty = {});

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
  xiiCameraVisualizerAttribute(xiiStringView sModeProperty, xiiStringView sFovProperty, xiiStringView sOrthoDimProperty, xiiStringView sNearPlaneProperty, xiiStringView sFarPlaneProperty);

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

/// \brief Attribute for types that should not be exposed to the scripting framework.
class XII_FOUNDATION_DLL xiiExcludeFromScript : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExcludeFromScript, xiiPropertyAttribute);
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

  xiiScriptableFunctionAttribute(ArgType argType1 = In, xiiStringView sArg1 = {}, ArgType argType2 = In, xiiStringView sArg2 = {}, ArgType argType3 = In, xiiStringView sArg3 = {}, ArgType argType4 = In, xiiStringView sArg4 = {}, ArgType argType5 = In, xiiStringView sArg5 = {}, ArgType argType6 = In, xiiStringView sArg6 = {}, ArgType argType7 = In, xiiStringView sArg7 = {}, ArgType argType8 = In, xiiStringView sArg8 = {}, ArgType argType9 = In, xiiStringView sArg9 = {}, ArgType argType10 = In, xiiStringView sArg10 = {}, ArgType argType11 = In, xiiStringView sArg11 = {}, ArgType argType12 = In, xiiStringView sArg12 = {}, ArgType argType13 = In, xiiStringView sArg13 = {}, ArgType argType14 = In, xiiStringView sArg14 = {}, ArgType argType15 = In, xiiStringView sArg15 = {}, ArgType argType16 = In, xiiStringView sArg16 = {}, ArgType argType17 = In, xiiStringView sArg17 = {}, ArgType argType18 = In, xiiStringView sArg18 = {}, ArgType argType19 = In, xiiStringView sArg19 = {}, ArgType argType20 = In, xiiStringView sArg20 = {}, ArgType argType21 = In, xiiStringView sArg21 = {}, ArgType argType22 = In, xiiStringView sArg22 = {}, ArgType argType23 = In, xiiStringView sArg23 = {}, ArgType argType24 = In, xiiStringView sArg24 = {}, ArgType argType25 = In, xiiStringView sArg25 = {}, ArgType argType26 = In, xiiStringView sArg26 = {}, ArgType argType27 = In, xiiStringView sArg27 = {}, ArgType argType28 = In, xiiStringView sArg28 = {}, ArgType argType29 = In, xiiStringView sArg29 = {}, ArgType argType30 = In, xiiStringView sArg30 = {}, ArgType argType31 = In, xiiStringView sArg31 = {}, ArgType argType32 = In, xiiStringView sArg32 = {});

  xiiUInt32     GetArgumentCount() const { return m_ArgNames.GetCount(); }
  xiiStringView GetArgumentName(xiiUInt32 uiIndex) const { return m_ArgNames[uiIndex]; }

  ArgType GetArgumentType(xiiUInt32 uiIndex) const { return static_cast<ArgType>(m_ArgTypes[uiIndex]); };

private:
  xiiHybridArray<xiiUntrackedString, 6> m_ArgNames;
  xiiHybridArray<xiiUInt8, 6>           m_ArgTypes;
};

/// \brief Wrapper Attribute to add an attribute to a function argument
class XII_FOUNDATION_DLL xiiFunctionArgumentAttributes : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFunctionArgumentAttributes, xiiPropertyAttribute);

  xiiFunctionArgumentAttributes() = default;
  xiiFunctionArgumentAttributes(xiiUInt32 uiArgIndex, const xiiPropertyAttribute* pAttribute1, const xiiPropertyAttribute* pAttribute2 = nullptr, const xiiPropertyAttribute* pAttribute3 = nullptr, const xiiPropertyAttribute* pAttribute4 = nullptr);
  ~xiiFunctionArgumentAttributes();

  xiiUInt32                                      GetArgumentIndex() const { return m_uiArgIndex; }
  xiiArrayPtr<const xiiPropertyAttribute* const> GetArgumentAttributes() const { return m_ArgAttributes; }

private:
  // Not pretty, but the values in the array are either created using 'new' when using this class as a reflection decoration, or created using 'XII_DEFAULT_NEW' when serialized and sent to the editor so in the dtor we need to know where these came from.
  xiiUInt32                                      m_uiArgIndex     = 0;
  bool                                           m_bUsesGlobalNew = false;
  xiiHybridArray<const xiiPropertyAttribute*, 4> m_ArgAttributes;
};

/// \brief Used to mark an array or (unsigned)int property as source for dynamic pin generation on nodes
class XII_FOUNDATION_DLL xiiDynamicPinAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicPinAttribute, xiiPropertyAttribute);

public:
  xiiDynamicPinAttribute() = default;
  xiiDynamicPinAttribute(xiiStringView sProperty);

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
  xiiLongOpAttribute(xiiStringView sOpTypeName) :
    m_sOpTypeName(sOpTypeName)
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

//////////////////////////////////////////////////////////////////////////

/// \brief Displays the value range as an image, allowing users to pick a value like on a slider.
///
/// This attribute always has to be combined with a xiiClampValueAttribute to define the min and max value range.
/// The constructor takes the name of an image generator. The generator is used to build the QImage used for the slider background.
///
/// Image generators are registered through xiiQtImageSliderWidget::s_ImageGenerators. Search the codebase for that variable
/// to determine which types of image generators are available. Custom generators can be registered as well.
class XII_FOUNDATION_DLL xiiImageSliderUiAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImageSliderUiAttribute, xiiTypeWidgetAttribute);

public:
  xiiImageSliderUiAttribute() = default;
  xiiImageSliderUiAttribute(xiiStringView sImageGenerator) :
    m_sImageGenerator(sImageGenerator)
  {
  }

  xiiUntrackedString m_sImageGenerator;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Attribute that turns a string property into a selector for an RTTI type.
///
/// The base type defines what types to display.
/// For example if "xiiComponent" is passed in, only types derived from xiiComponent are listed.
class XII_FOUNDATION_DLL xiiRttiTypeStringAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRttiTypeStringAttribute, xiiTypeWidgetAttribute);

public:
  xiiRttiTypeStringAttribute() = default;
  xiiRttiTypeStringAttribute(xiiStringView sBaseType) :
    m_sBaseType(sBaseType)
  {
  }

  xiiStringView GetBaseType() const { return m_sBaseType; }

private:
  xiiUntrackedString m_sBaseType;
};
