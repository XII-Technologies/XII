#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshAssetProperties, 3, xiiRTTIDefaultAllocator<xiiMeshAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("PrimitiveType", xiiMeshPrimitive, m_PrimitiveType),
    XII_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new xiiFileBrowserAttribute("Select Mesh", xiiFileBrowserAttribute::Meshes)),
    XII_ENUM_MEMBER_PROPERTY("RightDir", xiiBasisAxis, m_RightDir)->AddAttributes(new xiiDefaultValueAttribute((int)xiiBasisAxis::PositiveX)),
    XII_ENUM_MEMBER_PROPERTY("UpDir", xiiBasisAxis, m_UpDir)->AddAttributes(new xiiDefaultValueAttribute((int)xiiBasisAxis::PositiveY)),
    XII_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    XII_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0001f, 10000.0f)),
    XII_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    XII_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTrangents)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ENUM_MEMBER_PROPERTY("NormalPrecision", xiiMeshNormalPrecision, m_NormalPrecision),
    XII_ENUM_MEMBER_PROPERTY("TexCoordPrecision", xiiMeshTexCoordPrecision, m_TexCoordPrecision),
    XII_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Radius2", m_fRadius2)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Detail", m_uiDetail)->AddAttributes(new xiiDefaultValueAttribute(0), new xiiClampValueAttribute(0, 128)),
    XII_MEMBER_PROPERTY("Detail2", m_uiDetail2)->AddAttributes(new xiiDefaultValueAttribute(0), new xiiClampValueAttribute(0, 128)),
    XII_MEMBER_PROPERTY("Cap", m_bCap)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Cap2", m_bCap2)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(360.0f)), new xiiClampValueAttribute(xiiAngle::MakeFromDegree(0.0f), xiiAngle::MakeFromDegree(360.0f))),
    XII_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new xiiContainerAttribute(false, true, true)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class xiiMeshAssetPropertiesPatch_1_2 : public xiiGraphPatch
{
public:
  xiiMeshAssetPropertiesPatch_1_2() :
    xiiGraphPatch("xiiMeshAssetProperties", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Primitive Type", "PrimitiveType");
    pNode->RenameProperty("Forward Dir", "ForwardDir");
    pNode->RenameProperty("Right Dir", "RightDir");
    pNode->RenameProperty("Up Dir", "UpDir");
    pNode->RenameProperty("Uniform Scaling", "UniformScaling");
    pNode->RenameProperty("Non-Uniform Scaling", "NonUniformScaling");
    pNode->RenameProperty("Mesh File", "MeshFile");
    pNode->RenameProperty("Radius 2", "Radius2");
    pNode->RenameProperty("Detail 2", "Detail2");
    pNode->RenameProperty("Cap 2", "Cap2");
    pNode->RenameProperty("Import Materials", "ImportMaterials");
  }
};

xiiMeshAssetPropertiesPatch_1_2 g_MeshAssetPropertiesPatch_1_2;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMeshPrimitive, 1)
XII_ENUM_CONSTANT(xiiMeshPrimitive::File), XII_ENUM_CONSTANT(xiiMeshPrimitive::Box), XII_ENUM_CONSTANT(xiiMeshPrimitive::Rect), XII_ENUM_CONSTANT(xiiMeshPrimitive::Cylinder), XII_ENUM_CONSTANT(xiiMeshPrimitive::Cone), XII_ENUM_CONSTANT(xiiMeshPrimitive::Pyramid), XII_ENUM_CONSTANT(xiiMeshPrimitive::Sphere), XII_ENUM_CONSTANT(xiiMeshPrimitive::HalfSphere), XII_ENUM_CONSTANT(xiiMeshPrimitive::GeodesicSphere), XII_ENUM_CONSTANT(xiiMeshPrimitive::Capsule), XII_ENUM_CONSTANT(xiiMeshPrimitive::Torus),
  XII_END_STATIC_REFLECTED_ENUM;

xiiMeshAssetProperties::xiiMeshAssetProperties()  = default;
xiiMeshAssetProperties::~xiiMeshAssetProperties() = default;


void xiiMeshAssetProperties::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiMeshAssetProperties>())
  {
    xiiInt64 primType = e.m_pObject->GetTypeAccessor().GetValue("PrimitiveType").ConvertTo<xiiInt64>();

    auto& props = *e.m_pPropertyStates;

    props["MeshFile"].m_Visibility            = xiiPropertyUiState::Invisible;
    props["Radius"].m_Visibility              = xiiPropertyUiState::Invisible;
    props["Radius2"].m_Visibility             = xiiPropertyUiState::Invisible;
    props["Height"].m_Visibility              = xiiPropertyUiState::Invisible;
    props["Detail"].m_Visibility              = xiiPropertyUiState::Invisible;
    props["Detail2"].m_Visibility             = xiiPropertyUiState::Invisible;
    props["Cap"].m_Visibility                 = xiiPropertyUiState::Invisible;
    props["Cap2"].m_Visibility                = xiiPropertyUiState::Invisible;
    props["Angle"].m_Visibility               = xiiPropertyUiState::Invisible;
    props["ImportMaterials"].m_Visibility     = xiiPropertyUiState::Invisible;
    props["RecalculateNormals"].m_Visibility  = xiiPropertyUiState::Invisible;
    props["RecalculateTangents"].m_Visibility = xiiPropertyUiState::Invisible;
    props["NormalPrecision"].m_Visibility     = xiiPropertyUiState::Invisible;
    props["TexCoordPrecision"].m_Visibility   = xiiPropertyUiState::Invisible;

    switch (primType)
    {
      case xiiMeshPrimitive::File:
        props["MeshFile"].m_Visibility            = xiiPropertyUiState::Default;
        props["ImportMaterials"].m_Visibility     = xiiPropertyUiState::Default;
        props["RecalculateNormals"].m_Visibility  = xiiPropertyUiState::Default;
        props["RecalculateTangents"].m_Visibility = xiiPropertyUiState::Default;
        props["NormalPrecision"].m_Visibility     = xiiPropertyUiState::Default;
        props["TexCoordPrecision"].m_Visibility   = xiiPropertyUiState::Default;
        break;

      case xiiMeshPrimitive::Box:
        break;

      case xiiMeshPrimitive::Rect:
        props["Detail"].m_Visibility  = xiiPropertyUiState::Default;
        props["Detail2"].m_Visibility = xiiPropertyUiState::Default;

        props["Detail"].m_sNewLabelText  = "Prim.Rect.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Rect.Detail2";
        break;

      case xiiMeshPrimitive::Capsule:
        props["Radius"].m_Visibility  = xiiPropertyUiState::Default;
        props["Height"].m_Visibility  = xiiPropertyUiState::Default;
        props["Detail"].m_Visibility  = xiiPropertyUiState::Default;
        props["Detail2"].m_Visibility = xiiPropertyUiState::Default;

        props["Detail"].m_sNewLabelText  = "Prim.Sphere.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
        break;

      case xiiMeshPrimitive::Cone:
        props["Radius"].m_Visibility = xiiPropertyUiState::Default;
        props["Height"].m_Visibility = xiiPropertyUiState::Default;
        props["Detail"].m_Visibility = xiiPropertyUiState::Default;
        props["Cap"].m_Visibility    = xiiPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Cylinder.Detail";
        break;

      case xiiMeshPrimitive::Cylinder:
        props["Radius"].m_Visibility  = xiiPropertyUiState::Default;
        props["Radius2"].m_Visibility = xiiPropertyUiState::Default;
        props["Height"].m_Visibility  = xiiPropertyUiState::Default;
        props["Detail"].m_Visibility  = xiiPropertyUiState::Default;
        props["Cap"].m_Visibility     = xiiPropertyUiState::Default;
        props["Cap2"].m_Visibility    = xiiPropertyUiState::Default;
        props["Angle"].m_Visibility   = xiiPropertyUiState::Default;

        props["Detail"].m_sNewLabelText  = "Prim.Cylinder.Detail";
        props["Radius"].m_sNewLabelText  = "Prim.Cylinder.Radius1";
        props["Radius2"].m_sNewLabelText = "Prim.Cylinder.Radius2";
        props["Angle"].m_sNewLabelText   = "Prim.Cylinder.Angle";
        props["Cap"].m_sNewLabelText     = "Prim.Cylinder.Cap1";
        props["Cap2"].m_sNewLabelText    = "Prim.Cylinder.Cap2";
        break;

      case xiiMeshPrimitive::GeodesicSphere:
        props["Radius"].m_Visibility = xiiPropertyUiState::Default;
        props["Detail"].m_Visibility = xiiPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.GeoSphere.Detail";
        break;

      case xiiMeshPrimitive::HalfSphere:
        props["Radius"].m_Visibility  = xiiPropertyUiState::Default;
        props["Detail"].m_Visibility  = xiiPropertyUiState::Default;
        props["Detail2"].m_Visibility = xiiPropertyUiState::Default;
        props["Cap"].m_Visibility     = xiiPropertyUiState::Default;

        props["Detail"].m_sNewLabelText  = "Prim.Sphere.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
        break;

      case xiiMeshPrimitive::Pyramid:
        props["Cap"].m_Visibility = xiiPropertyUiState::Default;
        break;

      case xiiMeshPrimitive::Sphere:
        props["Radius"].m_Visibility  = xiiPropertyUiState::Default;
        props["Detail"].m_Visibility  = xiiPropertyUiState::Default;
        props["Detail2"].m_Visibility = xiiPropertyUiState::Default;

        props["Detail"].m_sNewLabelText  = "Prim.Sphere.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
        break;

      case xiiMeshPrimitive::Torus:
        props["Radius"].m_Visibility  = xiiPropertyUiState::Default;
        props["Radius2"].m_Visibility = xiiPropertyUiState::Default;
        props["Detail"].m_Visibility  = xiiPropertyUiState::Default;
        props["Detail2"].m_Visibility = xiiPropertyUiState::Default;

        props["Detail"].m_sNewLabelText  = "Prim.Torus.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Torus.Detail2";
        props["Radius"].m_sNewLabelText  = "Prim.Torus.Radius1";
        props["Radius2"].m_sNewLabelText = "Prim.Torus.Radius2";
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class xiiMeshAssetPropertiesPatch_2_3 : public xiiGraphPatch
{
public:
  xiiMeshAssetPropertiesPatch_2_3() :
    xiiGraphPatch("xiiMeshAssetProperties", 3)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    // convert the "Angle" property from float to xiiAngle
    if (auto pProp = pNode->FindProperty("Angle"))
    {
      if (pProp->m_Value.IsA<float>())
      {
        const float valFloat = pProp->m_Value.Get<float>();
        pProp->m_Value       = xiiAngle::MakeFromDegree(valFloat);
      }
    }
  }
};

xiiMeshAssetPropertiesPatch_2_3 g_xiiMeshAssetPropertiesPatch_2_3;
