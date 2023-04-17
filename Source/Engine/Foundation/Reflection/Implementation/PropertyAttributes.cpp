#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Reflection.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPropertyAttribute, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReadOnlyAttribute, 1, xiiRTTIDefaultAllocator<xiiReadOnlyAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHiddenAttribute, 1, xiiRTTIDefaultAllocator<xiiHiddenAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTemporaryAttribute, 1, xiiRTTIDefaultAllocator<xiiTemporaryAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiDependencyFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiDependencyFlags::Package, xiiDependencyFlags::Thumbnail, xiiDependencyFlags::Transform)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCategoryAttribute, 1, xiiRTTIDefaultAllocator<xiiCategoryAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Category", m_sCategory),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInDevelopmentAttribute, 1, xiiRTTIDefaultAllocator<xiiInDevelopmentAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Phase", m_Phase),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;


const char* xiiInDevelopmentAttribute::GetString() const
{
  switch (m_Phase)
  {
  case Phase::Alpha:
    return "ALPHA";

  case Phase::Beta:
    return "BETA";

    XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return "";
}

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTitleAttribute, 1, xiiRTTIDefaultAllocator<xiiTitleAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Title", m_sTitle),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiColorAttribute, 1, xiiRTTIDefaultAllocator<xiiColorAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_Color),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiColor),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExposeColorAlphaAttribute, 1, xiiRTTIDefaultAllocator<xiiExposeColorAlphaAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSuffixAttribute, 1, xiiRTTIDefaultAllocator<xiiSuffixAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Suffix", m_sSuffix),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMinValueTextAttribute, 1, xiiRTTIDefaultAllocator<xiiMinValueTextAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Text", m_sText),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDefaultValueAttribute, 1, xiiRTTIDefaultAllocator<xiiDefaultValueAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Value", m_Value),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const xiiVariant&),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClampValueAttribute, 1, xiiRTTIDefaultAllocator<xiiClampValueAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Min", m_MinValue),
    XII_MEMBER_PROPERTY("Max", m_MaxValue),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const xiiVariant&, const xiiVariant&),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGroupAttribute, 1, xiiRTTIDefaultAllocator<xiiGroupAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Group", m_sGroup),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, float),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, float),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGroupAttribute::xiiGroupAttribute()
{

}

xiiGroupAttribute::xiiGroupAttribute(const char* szGroup, float fOrder)
{
  m_sGroup = szGroup;
  m_fOrder = fOrder;
}

xiiGroupAttribute::xiiGroupAttribute(const char* szGroup, const char* szIconName, float fOrder)
{
  m_sGroup = szGroup;
  m_sIconName = szIconName;
  m_fOrder = fOrder;
}

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeWidgetAttribute, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiContainerWidgetAttribute, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTagSetWidgetAttribute, 1, xiiRTTIDefaultAllocator<xiiTagSetWidgetAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Filter", m_sTagFilter),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExposedParametersAttribute, 1, xiiRTTIDefaultAllocator<xiiExposedParametersAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ParametersSource", m_sParametersSource),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicDefaultValueAttribute, 1, xiiRTTIDefaultAllocator<xiiDynamicDefaultValueAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ClassSource", m_sClassSource),
    XII_MEMBER_PROPERTY("ClassType", m_sClassType),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiContainerAttribute, 1, xiiRTTIDefaultAllocator<xiiContainerAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CanAdd", m_bCanAdd),
    XII_MEMBER_PROPERTY("CanDelete", m_bCanDelete),
    XII_MEMBER_PROPERTY("CanMove", m_bCanMove),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(bool, bool, bool),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConstrainPointerAttribute, 1, xiiRTTIDefaultAllocator<xiiConstrainPointerAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ConstantName", m_sConstantName),
    XII_MEMBER_PROPERTY("ConstantValue", m_sConstantValueProperty),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFileBrowserAttribute, 1, xiiRTTIDefaultAllocator<xiiFileBrowserAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Title", m_sDialogTitle),
    XII_MEMBER_PROPERTY("Filter", m_sTypeFilter),
    XII_MEMBER_PROPERTY("CustomAction", m_sCustomAction),
    XII_BITFLAGS_MEMBER_PROPERTY("DependencyFlags", xiiDependencyFlags, m_DependencyFlags),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetBrowserAttribute, 1, xiiRTTIDefaultAllocator<xiiAssetBrowserAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Filter", m_sTypeFilter),
    XII_BITFLAGS_MEMBER_PROPERTY("DependencyFlags", xiiDependencyFlags, m_DependencyFlags),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicEnumAttribute, 1, xiiRTTIDefaultAllocator<xiiDynamicEnumAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
   XII_MEMBER_PROPERTY("DynamicEnum", m_sDynamicEnumName),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
   XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicStringEnumAttribute, 1, xiiRTTIDefaultAllocator<xiiDynamicStringEnumAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DynamicEnum", m_sDynamicEnumName),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiManipulatorAttribute, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Property1", m_sProperty1),
    XII_MEMBER_PROPERTY("Property2", m_sProperty2),
    XII_MEMBER_PROPERTY("Property3", m_sProperty3),
    XII_MEMBER_PROPERTY("Property4", m_sProperty4),
    XII_MEMBER_PROPERTY("Property5", m_sProperty5),
    XII_MEMBER_PROPERTY("Property6", m_sProperty6),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiManipulatorAttribute::xiiManipulatorAttribute(const char* szProperty1, const char* szProperty2 /*= nullptr*/, const char* szProperty3 /*= nullptr*/, const char* szProperty4 /*= nullptr*/, const char* szProperty5 /*= nullptr*/, const char* szProperty6 /*= nullptr*/)
{
  m_sProperty1 = szProperty1;
  m_sProperty2 = szProperty2;
  m_sProperty3 = szProperty3;
  m_sProperty4 = szProperty4;
  m_sProperty5 = szProperty5;
  m_sProperty6 = szProperty6;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSphereManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiSphereManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSphereManipulatorAttribute::xiiSphereManipulatorAttribute() :
  xiiManipulatorAttribute(nullptr)
{
}

xiiSphereManipulatorAttribute::xiiSphereManipulatorAttribute(const char* szOuterRadius, const char* szInnerRadius) :
  xiiManipulatorAttribute(szOuterRadius, szInnerRadius)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCapsuleManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiCapsuleManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCapsuleManipulatorAttribute::xiiCapsuleManipulatorAttribute() :
  xiiManipulatorAttribute(nullptr)
{
}

xiiCapsuleManipulatorAttribute::xiiCapsuleManipulatorAttribute(const char* szLength, const char* szRadius) :
  xiiManipulatorAttribute(szLength, szRadius)
{
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBoxManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiBoxManipulatorAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("scale", m_fSizeScale),
    XII_MEMBER_PROPERTY("recenter", m_bRecenterParent),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, bool, float),
    XII_CONSTRUCTOR_PROPERTY(const char*, bool, float),
    XII_CONSTRUCTOR_PROPERTY(const char*, bool, float, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, bool, float, const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBoxManipulatorAttribute::xiiBoxManipulatorAttribute() :
  xiiManipulatorAttribute(nullptr)
{
}

xiiBoxManipulatorAttribute::xiiBoxManipulatorAttribute(const char* szSizeProperty, float fSizeScale, bool bRecenterParent, const char* szOffsetProperty, const char* szRotationProperty) :
  xiiManipulatorAttribute(szSizeProperty, szOffsetProperty, szRotationProperty)
{
  m_bRecenterParent = bRecenterParent;
  m_fSizeScale      = fSizeScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNonUniformBoxManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiNonUniformBoxManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const char*, const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiNonUniformBoxManipulatorAttribute::xiiNonUniformBoxManipulatorAttribute() :
  xiiManipulatorAttribute(nullptr)
{
}

xiiNonUniformBoxManipulatorAttribute::xiiNonUniformBoxManipulatorAttribute(
  const char* szNegXProp,
  const char* szPosXProp,
  const char* szNegYProp,
  const char* szPosYProp,
  const char* szNegZProp,
  const char* szPosZProp) :
  xiiManipulatorAttribute(szNegXProp, szPosXProp, szNegYProp, szPosYProp, szNegZProp, szPosZProp)
{
}

xiiNonUniformBoxManipulatorAttribute::xiiNonUniformBoxManipulatorAttribute(const char* szSizeX, const char* szSizeY, const char* szSizeZ) :
  xiiManipulatorAttribute(szSizeX, szSizeY, szSizeZ)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConeLengthManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiConeLengthManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiConeLengthManipulatorAttribute::xiiConeLengthManipulatorAttribute() :
  xiiManipulatorAttribute(nullptr)
{
}

xiiConeLengthManipulatorAttribute::xiiConeLengthManipulatorAttribute(const char* szRadiusProperty) :
  xiiManipulatorAttribute(szRadiusProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConeAngleManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiConeAngleManipulatorAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("scale", m_fScale),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, float),
    XII_CONSTRUCTOR_PROPERTY(const char*, float, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiConeAngleManipulatorAttribute::xiiConeAngleManipulatorAttribute() :
  xiiManipulatorAttribute(nullptr)
{
  m_fScale = 1.0f;
}

xiiConeAngleManipulatorAttribute::xiiConeAngleManipulatorAttribute(const char* szAngleProperty, float fScale, const char* szRadiusProperty) :
  xiiManipulatorAttribute(szAngleProperty, szRadiusProperty)
{
  m_fScale = fScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTransformManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiTransformManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTransformManipulatorAttribute::xiiTransformManipulatorAttribute() :
  xiiManipulatorAttribute(nullptr)
{
}

xiiTransformManipulatorAttribute::xiiTransformManipulatorAttribute(
  const char* szTranslateProperty,
  const char* szRotateProperty,
  const char* szScaleProperty,
  const char* szOffsetTranslation,
  const char* szOffsetRotation) :
  xiiManipulatorAttribute(szTranslateProperty, szRotateProperty, szScaleProperty, szOffsetTranslation, szOffsetRotation)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBoneManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiBoneManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBoneManipulatorAttribute::xiiBoneManipulatorAttribute() :
  xiiManipulatorAttribute(nullptr)
{
}

xiiBoneManipulatorAttribute::xiiBoneManipulatorAttribute(const char* szTransformProperty, const char* szBindTo) :
  xiiManipulatorAttribute(szTransformProperty, szBindTo)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiVisualizerAnchor, 1)
  XII_BITFLAGS_CONSTANTS(xiiVisualizerAnchor::Center, xiiVisualizerAnchor::PosX, xiiVisualizerAnchor::NegX, xiiVisualizerAnchor::PosY, xiiVisualizerAnchor::NegY, xiiVisualizerAnchor::PosZ, xiiVisualizerAnchor::NegZ)
XII_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualizerAttribute, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Property1", m_sProperty1),
    XII_MEMBER_PROPERTY("Property2", m_sProperty2),
    XII_MEMBER_PROPERTY("Property3", m_sProperty3),
    XII_MEMBER_PROPERTY("Property4", m_sProperty4),
    XII_MEMBER_PROPERTY("Property5", m_sProperty5),
    XII_BITFLAGS_MEMBER_PROPERTY("Anchor", xiiVisualizerAnchor, m_Anchor),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualizerAttribute::xiiVisualizerAttribute(const char* szProperty1, const char* szProperty2 /*= nullptr*/, const char* szProperty3 /*= nullptr*/, const char* szProperty4 /*= nullptr*/, const char* szProperty5 /*= nullptr*/)
{
  m_sProperty1 = szProperty1;
  m_sProperty2 = szProperty2;
  m_sProperty3 = szProperty3;
  m_sProperty4 = szProperty4;
  m_sProperty5 = szProperty5;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBoxVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiBoxVisualizerAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_Color),
    XII_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
    XII_MEMBER_PROPERTY("SizeScale", m_fSizeScale),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, float, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>, xiiVec3, const char*, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, float, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>, xiiVec3, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, float, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>, xiiVec3),
    XII_CONSTRUCTOR_PROPERTY(const char*, float, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>),
    XII_CONSTRUCTOR_PROPERTY(const char*, float, const xiiColor&, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, float, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(const char*, float),
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBoxVisualizerAttribute::xiiBoxVisualizerAttribute() :
  xiiVisualizerAttribute(nullptr)
{
}

xiiBoxVisualizerAttribute::xiiBoxVisualizerAttribute(const char* szSizeProperty, float fSizeScale, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, xiiBitflags<xiiVisualizerAnchor> anchor /*= xiiVisualizerAnchor::Center*/, xiiVec3 offsetOrScale /*= xiiVec3::ZeroVector*/, const char* szOffsetProperty /*= nullptr*/, const char* szRotationProperty /*= nullptr*/) :
  xiiVisualizerAttribute(szSizeProperty, szColorProperty, szOffsetProperty, szRotationProperty)
{
  m_Color          = fixedColor;
  m_vOffsetOrScale = offsetOrScale;
  m_Anchor         = anchor;
  m_fSizeScale     = fSizeScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSphereVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiSphereVisualizerAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_Color),
    XII_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>, xiiVec3, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>, xiiVec3),
    XII_CONSTRUCTOR_PROPERTY(const char*, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>),
    XII_CONSTRUCTOR_PROPERTY(const char*, const xiiColor&, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSphereVisualizerAttribute::xiiSphereVisualizerAttribute() :
  xiiVisualizerAttribute(nullptr)
{
}

xiiSphereVisualizerAttribute::xiiSphereVisualizerAttribute(const char* szRadiusProperty, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, xiiBitflags<xiiVisualizerAnchor> anchor /*= xiiVisualizerAnchor::Center*/, xiiVec3 offsetOrScale /*= xiiVec3::ZeroVector*/, const char* szOffsetProperty /*= nullptr*/) :
  xiiVisualizerAttribute(szRadiusProperty, szColorProperty, szOffsetProperty)
{
  m_Color          = fixedColor;
  m_vOffsetOrScale = offsetOrScale;
  m_Anchor         = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCapsuleVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiCapsuleVisualizerAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_Color),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const xiiColor&, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCapsuleVisualizerAttribute::xiiCapsuleVisualizerAttribute() :
  xiiVisualizerAttribute(nullptr)
{
}

xiiCapsuleVisualizerAttribute::xiiCapsuleVisualizerAttribute(const char* szHeightProperty, const char* szRadiusProperty, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, xiiBitflags<xiiVisualizerAnchor> anchor /*= xiiVisualizerAnchor::Center*/) :
  xiiVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty)
{
  m_Color  = fixedColor;
  m_Anchor = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCylinderVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiCylinderVisualizerAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_Color),
    XII_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
    XII_ENUM_MEMBER_PROPERTY("Axis", xiiBasisAxis, m_Axis),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, const char*, const char*, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>, xiiVec3, const char*),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, const char*, const char*, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>, xiiVec3),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, const char*, const char*, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, const char*, const char*, const xiiColor&, const char*),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, const char*, const char*, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, const char*, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>, xiiVec3, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>, xiiVec3),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const xiiColor&, const char*, xiiBitflags<xiiVisualizerAnchor>),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const xiiColor&, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCylinderVisualizerAttribute::xiiCylinderVisualizerAttribute() :
  xiiVisualizerAttribute(nullptr)
{
}

xiiCylinderVisualizerAttribute::xiiCylinderVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, const char* szHeightProperty, const char* szRadiusProperty, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, xiiBitflags<xiiVisualizerAnchor> anchor /*= xiiVisualizerAnchor::Center*/, xiiVec3 offsetOrScale /*= xiiVec3::ZeroVector*/, const char* szOffsetProperty /*= nullptr*/) :
  xiiVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty, szOffsetProperty)
{
  m_Color          = fixedColor;
  m_vOffsetOrScale = offsetOrScale;
  m_Axis           = axis;
  m_Anchor         = anchor;
}

xiiCylinderVisualizerAttribute::xiiCylinderVisualizerAttribute(const char* szAxisProperty, const char* szHeightProperty, const char* szRadiusProperty, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, xiiBitflags<xiiVisualizerAnchor> anchor /*= xiiVisualizerAnchor::Center*/, xiiVec3 offsetOrScale /*= xiiVec3::ZeroVector()*/, const char* szOffsetProperty /*= nullptr*/) :
  xiiVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty, szOffsetProperty, szAxisProperty)
{
  m_Color          = fixedColor;
  m_vOffsetOrScale = offsetOrScale;
  m_Axis           = xiiBasisAxis::Default;
  m_Anchor         = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDirectionVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiDirectionVisualizerAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Axis", xiiBasisAxis, m_Axis),
    XII_MEMBER_PROPERTY("Color", m_Color),
    XII_MEMBER_PROPERTY("Scale", m_fScale)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, float, const xiiColor&, const char*, const char*),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, float, const xiiColor&, const char*),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, float, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, float),
    XII_CONSTRUCTOR_PROPERTY(const char*, float, const xiiColor&, const char*, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, float, const xiiColor&, const char*),
    XII_CONSTRUCTOR_PROPERTY(const char*, float, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(const char*, float),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDirectionVisualizerAttribute::xiiDirectionVisualizerAttribute() :
  xiiVisualizerAttribute(nullptr)
{
  m_Axis   = xiiBasisAxis::PositiveX;
  m_fScale = 1.0f;
  m_Color  = xiiColor::White;
}

xiiDirectionVisualizerAttribute::xiiDirectionVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, float fScale, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, const char* szLengthProperty /*= nullptr*/) :
  xiiVisualizerAttribute(szColorProperty, szLengthProperty)
{
  m_Axis   = axis;
  m_fScale = fScale;
  m_Color  = fixedColor;
}

xiiDirectionVisualizerAttribute::xiiDirectionVisualizerAttribute(const char* szAxisProperty, float fScale, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, const char* szLengthProperty /*= nullptr*/) :
  xiiVisualizerAttribute(szColorProperty, szLengthProperty, szAxisProperty)
{
  m_Axis   = xiiBasisAxis::PositiveX;
  m_fScale = fScale;
  m_Color  = fixedColor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConeVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiConeVisualizerAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Axis", xiiBasisAxis, m_Axis),
    XII_MEMBER_PROPERTY("Color", m_Color),
    XII_MEMBER_PROPERTY("Scale", m_fScale),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, const char*, float, const char*, const xiiColor&, const char*),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, const char*, float, const char*, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, const char*, float, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiConeVisualizerAttribute::xiiConeVisualizerAttribute() :
  xiiVisualizerAttribute(nullptr)
{
  m_Axis   = xiiBasisAxis::PositiveX;
  m_Color  = xiiColor::Red;
  m_fScale = 1.0f;
}

xiiConeVisualizerAttribute::xiiConeVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, const char* szAngleProperty, float fScale, const char* szRadiusProperty, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, const char* szColorProperty) :
  xiiVisualizerAttribute(szAngleProperty, szRadiusProperty, szColorProperty)
{
  m_Axis   = axis;
  m_Color  = fixedColor;
  m_fScale = fScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCameraVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiCameraVisualizerAttribute>)
{
  //XII_BEGIN_PROPERTIES
  //XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCameraVisualizerAttribute::xiiCameraVisualizerAttribute() :
  xiiVisualizerAttribute(nullptr)
{
}

xiiCameraVisualizerAttribute::xiiCameraVisualizerAttribute(const char* szModeProperty, const char* szFovProperty, const char* szOrthoDimProperty, const char* szNearPlaneProperty, const char* szFarPlaneProperty) :
  xiiVisualizerAttribute(szModeProperty, szFovProperty, szOrthoDimProperty, szNearPlaneProperty, szFarPlaneProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaxArraySizeAttribute, 1, xiiRTTIDefaultAllocator<xiiMaxArraySizeAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MaxSize", m_uiMaxSize),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiUInt32),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPreventDuplicatesAttribute, 1, xiiRTTIDefaultAllocator<xiiPreventDuplicatesAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAutoGenVisScriptMsgSender, 1, xiiRTTIDefaultAllocator<xiiAutoGenVisScriptMsgSender>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAutoGenVisScriptMsgHandler, 1, xiiRTTIDefaultAllocator<xiiAutoGenVisScriptMsgHandler>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScriptableFunctionAttribute, 1, xiiRTTIDefaultAllocator<xiiScriptableFunctionAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Arg1", m_sArg1),
    XII_MEMBER_PROPERTY("Arg2", m_sArg2),
    XII_MEMBER_PROPERTY("Arg3", m_sArg3),
    XII_MEMBER_PROPERTY("Arg4", m_sArg4),
    XII_MEMBER_PROPERTY("Arg5", m_sArg5),
    XII_MEMBER_PROPERTY("Arg6", m_sArg6),
    XII_MEMBER_PROPERTY("ArgType1", m_ArgType1),
    XII_MEMBER_PROPERTY("ArgType2", m_ArgType2),
    XII_MEMBER_PROPERTY("ArgType3", m_ArgType3),
    XII_MEMBER_PROPERTY("ArgType4", m_ArgType4),
    XII_MEMBER_PROPERTY("ArgType5", m_ArgType5),
    XII_MEMBER_PROPERTY("ArgType6", m_ArgType6),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiScriptableFunctionAttribute::xiiScriptableFunctionAttribute(ArgType ArgType1 /*= In*/, const char* szArg1 /*= nullptr*/, ArgType ArgType2 /*= In*/, const char* szArg2 /*= nullptr*/, ArgType ArgType3 /*= In*/, const char* szArg3 /*= nullptr*/, ArgType ArgType4 /*= In*/, const char* szArg4 /*= nullptr*/, ArgType ArgType5 /*= In*/, const char* szArg5 /*= nullptr*/, ArgType ArgType6 /*= In*/, const char* szArg6 /*= nullptr*/)
{
  m_sArg1 = szArg1;
  m_sArg2 = szArg2;
  m_sArg3 = szArg3;
  m_sArg4 = szArg4;
  m_sArg5 = szArg5;
  m_sArg6 = szArg6;

  m_ArgType1 = ArgType1;
  m_ArgType2 = ArgType2;
  m_ArgType3 = ArgType3;
  m_ArgType4 = ArgType4;
  m_ArgType5 = ArgType5;
  m_ArgType6 = ArgType6;
}

const char* xiiScriptableFunctionAttribute::GetArgumentName(xiiUInt32 index) const
{
  switch (index)
  {
    case 0:
      return m_sArg1;
    case 1:
      return m_sArg2;
    case 2:
      return m_sArg3;
    case 3:
      return m_sArg4;
    case 4:
      return m_sArg5;
    case 5:
      return m_sArg6;
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

xiiScriptableFunctionAttribute::ArgType xiiScriptableFunctionAttribute::GetArgumentType(xiiUInt32 index) const
{
  switch (index)
  {
    case 0:
      return (ArgType)m_ArgType1;
    case 1:
      return (ArgType)m_ArgType2;
    case 2:
      return (ArgType)m_ArgType3;
    case 3:
      return (ArgType)m_ArgType4;
    case 4:
      return (ArgType)m_ArgType5;
    case 5:
      return (ArgType)m_ArgType6;
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return ArgType::In;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisScriptMappingAttribute, 1, xiiRTTIDefaultAllocator<xiiVisScriptMappingAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Mapping", m_iMapping)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLongOpAttribute, 1, xiiRTTIDefaultAllocator<xiiLongOpAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_sOpTypeName),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(),
    XII_CONSTRUCTOR_PROPERTY(const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameObjectReferenceAttribute, 1, xiiRTTIDefaultAllocator<xiiGameObjectReferenceAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

XII_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_PropertyAttributes);
