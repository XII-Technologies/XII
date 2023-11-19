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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
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


xiiStringView xiiInDevelopmentAttribute::GetString() const
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, float),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGroupAttribute::xiiGroupAttribute() = default;

xiiGroupAttribute::xiiGroupAttribute(xiiStringView sGroup, float fOrder)
: m_sGroup(sGroup), m_fOrder(fOrder)
{
}

xiiGroupAttribute::xiiGroupAttribute(xiiStringView sGroup, xiiStringView sIconName, float fOrder)
: m_sGroup(sGroup), m_sIconName(sIconName), m_fOrder(fOrder)
{
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNoTemporaryTransactionsAttribute, 1, xiiRTTIDefaultAllocator<xiiNoTemporaryTransactionsAttribute>)
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView),
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiStringView),
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
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
   XII_CONSTRUCTOR_PROPERTY(xiiStringView),
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicBitflagsAttribute, 1, xiiRTTIDefaultAllocator<xiiDynamicBitflagsAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
   XII_MEMBER_PROPERTY("DynamicBitflags", m_sDynamicBitflagsName),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
   XII_CONSTRUCTOR_PROPERTY(xiiStringView),
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

xiiManipulatorAttribute::xiiManipulatorAttribute(xiiStringView sProperty1, xiiStringView sProperty2 /*= nullptr*/, xiiStringView sProperty3 /*= nullptr*/, xiiStringView sProperty4 /*= nullptr*/, xiiStringView sProperty5 /*= nullptr*/, xiiStringView sProperty6 /*= nullptr*/) :
  m_sProperty1(sProperty1), m_sProperty2(sProperty2), m_sProperty3(sProperty3), m_sProperty4(sProperty4), m_sProperty5(sProperty5), m_sProperty6(sProperty6)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSphereManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiSphereManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSphereManipulatorAttribute::xiiSphereManipulatorAttribute() :
  xiiManipulatorAttribute({})
{
}

xiiSphereManipulatorAttribute::xiiSphereManipulatorAttribute(xiiStringView sOuterRadius, xiiStringView sInnerRadius) :
  xiiManipulatorAttribute(sOuterRadius, sInnerRadius)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCapsuleManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiCapsuleManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCapsuleManipulatorAttribute::xiiCapsuleManipulatorAttribute() :
  xiiManipulatorAttribute({})
{
}

xiiCapsuleManipulatorAttribute::xiiCapsuleManipulatorAttribute(xiiStringView sLength, xiiStringView sRadius) :
  xiiManipulatorAttribute(sLength, sRadius)
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, bool, float),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, bool, float),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, bool, float, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, bool, float, xiiStringView, xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBoxManipulatorAttribute::xiiBoxManipulatorAttribute() :
  xiiManipulatorAttribute({})
{
}

xiiBoxManipulatorAttribute::xiiBoxManipulatorAttribute(xiiStringView sSizeProperty, float fSizeScale, bool bRecenterParent, xiiStringView sOffsetProperty, xiiStringView sRotationProperty) :
  xiiManipulatorAttribute(sSizeProperty, sOffsetProperty, sRotationProperty), m_bRecenterParent(bRecenterParent), m_fSizeScale(fSizeScale)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNonUniformBoxManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiNonUniformBoxManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiStringView, xiiStringView, xiiStringView, xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiNonUniformBoxManipulatorAttribute::xiiNonUniformBoxManipulatorAttribute() :
  xiiManipulatorAttribute({})
{
}

xiiNonUniformBoxManipulatorAttribute::xiiNonUniformBoxManipulatorAttribute(xiiStringView sNegXProp, xiiStringView sPosXProp, xiiStringView sNegYProp, xiiStringView sPosYProp, xiiStringView sNegZProp, xiiStringView sPosZProp) :
  xiiManipulatorAttribute(sNegXProp, sPosXProp, sNegYProp, sPosYProp, sNegZProp, sPosZProp)
{
}

xiiNonUniformBoxManipulatorAttribute::xiiNonUniformBoxManipulatorAttribute(xiiStringView sSizeX, xiiStringView sSizeY, xiiStringView sSizeZ) :
  xiiManipulatorAttribute(sSizeX, sSizeY, sSizeZ)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConeLengthManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiConeLengthManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiConeLengthManipulatorAttribute::xiiConeLengthManipulatorAttribute() :
  xiiManipulatorAttribute({})
{
}

xiiConeLengthManipulatorAttribute::xiiConeLengthManipulatorAttribute(xiiStringView sRadiusProperty) :
  xiiManipulatorAttribute(sRadiusProperty)
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float, xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiConeAngleManipulatorAttribute::xiiConeAngleManipulatorAttribute() :
  xiiManipulatorAttribute({})
{
}

xiiConeAngleManipulatorAttribute::xiiConeAngleManipulatorAttribute(xiiStringView sAngleProperty, float fScale, xiiStringView sRadiusProperty) :
  xiiManipulatorAttribute(sAngleProperty, sRadiusProperty), m_fScale(fScale)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTransformManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiTransformManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTransformManipulatorAttribute::xiiTransformManipulatorAttribute() :
  xiiManipulatorAttribute({})
{
}

xiiTransformManipulatorAttribute::xiiTransformManipulatorAttribute(xiiStringView sTranslateProperty, xiiStringView sRotateProperty, xiiStringView sScaleProperty, xiiStringView sOffsetTranslation, xiiStringView sOffsetRotation) :
  xiiManipulatorAttribute(sTranslateProperty, sRotateProperty, sScaleProperty, sOffsetTranslation, sOffsetRotation)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBoneManipulatorAttribute, 1, xiiRTTIDefaultAllocator<xiiBoneManipulatorAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBoneManipulatorAttribute::xiiBoneManipulatorAttribute() :
  xiiManipulatorAttribute({})
{
}

xiiBoneManipulatorAttribute::xiiBoneManipulatorAttribute(xiiStringView sTransformProperty, xiiStringView sBindTo) :
  xiiManipulatorAttribute(sTransformProperty, sBindTo)
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

xiiVisualizerAttribute::xiiVisualizerAttribute(xiiStringView sProperty1, xiiStringView sProperty2 /*= nullptr*/, xiiStringView sProperty3 /*= nullptr*/, xiiStringView sProperty4 /*= nullptr*/, xiiStringView sProperty5 /*= nullptr*/) :
  m_sProperty1(sProperty1), m_sProperty2(sProperty2), m_sProperty3(sProperty3), m_sProperty4(sProperty4), m_sProperty5(sProperty5)
{
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>, xiiVec3, xiiStringView, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>, xiiVec3, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>, xiiVec3),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float, const xiiColor&, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBoxVisualizerAttribute::xiiBoxVisualizerAttribute() :
  xiiVisualizerAttribute({})
{
}

xiiBoxVisualizerAttribute::xiiBoxVisualizerAttribute(xiiStringView sSizeProperty, float fSizeScale, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, xiiStringView sColorProperty /*= nullptr*/, xiiBitflags<xiiVisualizerAnchor> anchor /*= xiiVisualizerAnchor::Center*/, xiiVec3 vOffsetOrScale /*= xiiVec3::ZeroVector*/, xiiStringView sOffsetProperty /*= nullptr*/, xiiStringView sRotationProperty /*= nullptr*/) :
  xiiVisualizerAttribute(sSizeProperty, sColorProperty, sOffsetProperty, sRotationProperty), m_Color(fixedColor), m_vOffsetOrScale(vOffsetOrScale), m_fSizeScale(fSizeScale)
{
  m_Anchor = anchor;
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>, xiiVec3, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>, xiiVec3),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, const xiiColor&, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSphereVisualizerAttribute::xiiSphereVisualizerAttribute() :
  xiiVisualizerAttribute({})
{
}

xiiSphereVisualizerAttribute::xiiSphereVisualizerAttribute(xiiStringView sRadiusProperty, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, xiiStringView sColorProperty /*= nullptr*/, xiiBitflags<xiiVisualizerAnchor> anchor /*= xiiVisualizerAnchor::Center*/, xiiVec3 vOffsetOrScale /*= xiiVec3::ZeroVector*/, xiiStringView sOffsetProperty /*= nullptr*/) :
  xiiVisualizerAttribute(sRadiusProperty, sColorProperty, sOffsetProperty), m_Color(fixedColor), m_vOffsetOrScale(vOffsetOrScale)
{
  m_Anchor = anchor;
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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, const xiiColor&, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCapsuleVisualizerAttribute::xiiCapsuleVisualizerAttribute() :
  xiiVisualizerAttribute({})
{
}

xiiCapsuleVisualizerAttribute::xiiCapsuleVisualizerAttribute(xiiStringView sHeightProperty, xiiStringView sRadiusProperty, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, xiiStringView sColorProperty /*= nullptr*/, xiiBitflags<xiiVisualizerAnchor> anchor /*= xiiVisualizerAnchor::Center*/) :
  xiiVisualizerAttribute(sHeightProperty, sRadiusProperty, sColorProperty), m_Color(fixedColor)
{
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
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, xiiStringView, xiiStringView, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>, xiiVec3, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, xiiStringView, xiiStringView, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>, xiiVec3),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, xiiStringView, xiiStringView, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, xiiStringView, xiiStringView, const xiiColor&, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, xiiStringView, xiiStringView, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, xiiStringView, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiStringView, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>, xiiVec3, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiStringView, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>, xiiVec3),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiStringView, const xiiColor&, xiiStringView, xiiBitflags<xiiVisualizerAnchor>),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiStringView, const xiiColor&, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiStringView, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCylinderVisualizerAttribute::xiiCylinderVisualizerAttribute() :
  xiiVisualizerAttribute({})
{
}

xiiCylinderVisualizerAttribute::xiiCylinderVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, xiiStringView sHeightProperty, xiiStringView sRadiusProperty, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, xiiStringView sColorProperty /*= nullptr*/, xiiBitflags<xiiVisualizerAnchor> anchor /*= xiiVisualizerAnchor::Center*/, xiiVec3 vOffsetOrScale /*= xiiVec3::ZeroVector*/, xiiStringView sOffsetProperty /*= nullptr*/) :
  xiiVisualizerAttribute(sHeightProperty, sRadiusProperty, sColorProperty, sOffsetProperty), m_Color(fixedColor), m_vOffsetOrScale(vOffsetOrScale), m_Axis(axis)
{
  m_Anchor = anchor;
}

xiiCylinderVisualizerAttribute::xiiCylinderVisualizerAttribute(xiiStringView sAxisProperty, xiiStringView sHeightProperty, xiiStringView sRadiusProperty, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, xiiStringView sColorProperty /*= nullptr*/, xiiBitflags<xiiVisualizerAnchor> anchor /*= xiiVisualizerAnchor::Center*/, xiiVec3 vOffsetOrScale /*= xiiVec3::ZeroVector()*/, xiiStringView sOffsetProperty /*= nullptr*/) :
  xiiVisualizerAttribute(sHeightProperty, sRadiusProperty, sColorProperty, sOffsetProperty, sAxisProperty), m_Color(fixedColor), m_vOffsetOrScale(vOffsetOrScale), m_Axis(xiiBasisAxis::Default)
{
  m_Anchor = anchor;
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
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, float, const xiiColor&, xiiStringView, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, float, const xiiColor&, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, float, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, float),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float, const xiiColor&, xiiStringView, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float, const xiiColor&, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, float),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDirectionVisualizerAttribute::xiiDirectionVisualizerAttribute() :
  xiiVisualizerAttribute({}), m_Axis(xiiBasisAxis::PositiveX), m_fScale(1.0f), m_Color(xiiColor::White)
{
}

xiiDirectionVisualizerAttribute::xiiDirectionVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, float fScale, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, xiiStringView sColorProperty /*= nullptr*/, xiiStringView sLengthProperty /*= nullptr*/) :
  xiiVisualizerAttribute(sColorProperty, sLengthProperty), m_Axis(axis), m_fScale(fScale), m_Color(fixedColor)
{
}

xiiDirectionVisualizerAttribute::xiiDirectionVisualizerAttribute(xiiStringView sAxisProperty, float fScale, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, xiiStringView sColorProperty /*= nullptr*/, xiiStringView sLengthProperty /*= nullptr*/) :
  xiiVisualizerAttribute(sColorProperty, sLengthProperty, sAxisProperty), m_Axis(xiiBasisAxis::PositiveX), m_fScale(fScale), m_Color(fixedColor)
{
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
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, xiiStringView, float, xiiStringView, const xiiColor&, xiiStringView),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, xiiStringView, float, xiiStringView, const xiiColor&),
    XII_CONSTRUCTOR_PROPERTY(xiiEnum<xiiBasisAxis>, xiiStringView, float, xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiConeVisualizerAttribute::xiiConeVisualizerAttribute() :
  xiiVisualizerAttribute({}), m_Axis(xiiBasisAxis::PositiveX), m_Color(xiiColor::Red), m_fScale(1.0f)
{
}

xiiConeVisualizerAttribute::xiiConeVisualizerAttribute(xiiEnum<xiiBasisAxis> axis, xiiStringView sAngleProperty, float fScale, xiiStringView sRadiusProperty, const xiiColor& fixedColor /*= xiiColorScheme::LightUI(xiiColorScheme::Grape)*/, xiiStringView sColorProperty) :
  xiiVisualizerAttribute(sAngleProperty, sRadiusProperty, sColorProperty), m_Axis(axis), m_Color(fixedColor), m_fScale(fScale)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCameraVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiCameraVisualizerAttribute>)
{
  // XII_BEGIN_PROPERTIES
  // XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(xiiStringView, xiiStringView, xiiStringView, xiiStringView, xiiStringView),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCameraVisualizerAttribute::xiiCameraVisualizerAttribute() :
  xiiVisualizerAttribute({})
{
}

xiiCameraVisualizerAttribute::xiiCameraVisualizerAttribute(xiiStringView sModeProperty, xiiStringView sFovProperty, xiiStringView sOrthoDimProperty, xiiStringView sNearPlaneProperty, xiiStringView sFarPlaneProperty) :
  xiiVisualizerAttribute(sModeProperty, sFovProperty, sOrthoDimProperty, sNearPlaneProperty, sFarPlaneProperty)
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
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExcludeFromScript, 1, xiiRTTIDefaultAllocator<xiiExcludeFromScript>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScriptableFunctionAttribute, 1, xiiRTTIDefaultAllocator<xiiScriptableFunctionAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("ArgNames", m_ArgNames),
    XII_ARRAY_MEMBER_PROPERTY("ArgTypes", m_ArgTypes),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiScriptableFunctionAttribute::xiiScriptableFunctionAttribute(ArgType argType1 /*= In*/, xiiStringView sArg1 /*= nullptr*/, ArgType argType2 /*= In*/, xiiStringView sArg2 /*= nullptr*/, ArgType argType3 /*= In*/, xiiStringView sArg3 /*= nullptr*/, ArgType argType4 /*= In*/, xiiStringView sArg4 /*= nullptr*/, ArgType argType5 /*= In*/, xiiStringView sArg5 /*= nullptr*/, ArgType argType6 /*= In*/, xiiStringView sArg6 /*= nullptr*/)
{
  {
    if (sArg1.IsEmpty())
      return;

    m_ArgNames.PushBack(sArg1);
    m_ArgTypes.PushBack(argType1);
  }
  {
    if (sArg2.IsEmpty())
      return;

    m_ArgNames.PushBack(sArg2);
    m_ArgTypes.PushBack(argType2);
  }
  {
    if (sArg3.IsEmpty())
      return;

    m_ArgNames.PushBack(sArg3);
    m_ArgTypes.PushBack(argType3);
  }
  {
    if (sArg4.IsEmpty())
      return;

    m_ArgNames.PushBack(sArg4);
    m_ArgTypes.PushBack(argType4);
  }
  {
    if (sArg5.IsEmpty())
      return;

    m_ArgNames.PushBack(sArg5);
    m_ArgTypes.PushBack(argType5);
  }
  {
    if (sArg6.IsEmpty())
      return;

    m_ArgNames.PushBack(sArg6);
    m_ArgTypes.PushBack(argType6);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFunctionArgumentAttributes, 1, xiiRTTIDefaultAllocator<xiiFunctionArgumentAttributes>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ArgIndex", m_uiArgIndex),
    XII_ARRAY_MEMBER_PROPERTY("ArgAttributes", m_ArgAttributes),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiFunctionArgumentAttributes::xiiFunctionArgumentAttributes(xiiUInt32 uiArgIndex, const xiiPropertyAttribute* pAttribute1, const xiiPropertyAttribute* pAttribute2 /*= nullptr*/, const xiiPropertyAttribute* pAttribute3 /*= nullptr*/, const xiiPropertyAttribute* pAttribute4 /*= nullptr*/) :
  m_uiArgIndex(uiArgIndex)
{
  {
    if (pAttribute1 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute1);
  }
  {
    if (pAttribute2 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute2);
  }
  {
    if (pAttribute3 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute3);
  }
  {
    if (pAttribute4 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute4);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicPinAttribute, 1, xiiRTTIDefaultAllocator<xiiDynamicPinAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Property", m_sProperty)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDynamicPinAttribute::xiiDynamicPinAttribute(xiiStringView sProperty) :
  m_sProperty(sProperty)
{
}

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
    XII_CONSTRUCTOR_PROPERTY(xiiStringView),
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
