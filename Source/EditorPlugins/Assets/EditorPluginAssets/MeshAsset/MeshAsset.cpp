#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshAssetDocument, 12, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static xiiMat3 CalculateTransformationMatrix(const xiiMeshAssetProperties* pProp)
{
  const float us = xiiMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  const xiiBasisAxis::Enum forwardDir = xiiBasisAxis::GetOrthogonalAxis(pProp->m_RightDir, pProp->m_UpDir, !pProp->m_bFlipForwardDir);

  return xiiBasisAxis::CalculateTransformationMatrix(forwardDir, pProp->m_RightDir, pProp->m_UpDir, us);
}

xiiMeshAssetDocument::xiiMeshAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiMeshAssetProperties>(szDocumentPath, xiiAssetDocEngineConnection::Simple, true)
{
}

xiiTransformStatus xiiMeshAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  xiiProgressRange range("Transforming Asset", 2, false);

  xiiMeshAssetProperties* pProp = GetProperties();

  xiiMeshResourceDescriptor desc;

  range.SetStepWeighting(0, 0.9f);
  range.BeginNextStep("Importing Mesh");

  if (pProp->m_PrimitiveType == xiiMeshPrimitive::File)
  {
    XII_SUCCEED_OR_RETURN(CreateMeshFromFile(pProp, desc, !transformFlags.IsSet(xiiTransformFlags::BackgroundProcessing)));
  }
  else
  {
    CreateMeshFromGeom(pProp, desc);
  }

  range.BeginNextStep("Writing Result");
  desc.Save(stream);

  return xiiStatus(XII_SUCCESS);
}


void xiiMeshAssetDocument::CreateMeshFromGeom(xiiMeshAssetProperties* pProp, xiiMeshResourceDescriptor& desc)
{
  const xiiMat3 mTransformation = CalculateTransformationMatrix(pProp);

  xiiGeometry geom;
  //const xiiMat4 mTrans(mTransformation, xiiVec3::ZeroVector());

  xiiGeometry::GeoOptions opt;
  opt.m_Transform = xiiMat4(mTransformation, xiiVec3::ZeroVector());

  auto detail1 = pProp->m_uiDetail;
  auto detail2 = pProp->m_uiDetail2;

  if (pProp->m_PrimitiveType == xiiMeshPrimitive::Box)
  {
    geom.AddBox(xiiVec3(1.0f), true, opt);
  }
  else if (pProp->m_PrimitiveType == xiiMeshPrimitive::Capsule)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 16;

    geom.AddCapsule(pProp->m_fRadius, xiiMath::Max(0.0f, pProp->m_fHeight), xiiMath::Max<xiiUInt16>(3, detail1), xiiMath::Max<xiiUInt16>(1, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == xiiMeshPrimitive::Cone)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;

    geom.AddCone(pProp->m_fRadius, pProp->m_fHeight, pProp->m_bCap, xiiMath::Max<xiiUInt16>(3, detail1), opt);
  }
  else if (pProp->m_PrimitiveType == xiiMeshPrimitive::Cylinder)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;

    geom.AddCylinder(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f, pProp->m_bCap, pProp->m_bCap2, xiiMath::Max<xiiUInt16>(3, detail1), opt, xiiMath::Clamp(pProp->m_Angle, xiiAngle::Degree(0.0f), xiiAngle::Degree(360.0f)));
  }
  else if (pProp->m_PrimitiveType == xiiMeshPrimitive::GeodesicSphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 2;

    geom.AddGeodesicSphere(pProp->m_fRadius, xiiMath::Clamp<xiiUInt16>(detail1, 0, 6), opt);
  }
  else if (pProp->m_PrimitiveType == xiiMeshPrimitive::HalfSphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 16;

    geom.AddHalfSphere(pProp->m_fRadius, xiiMath::Max<xiiUInt16>(3, detail1), xiiMath::Max<xiiUInt16>(1, detail2), pProp->m_bCap, opt);
  }
  else if (pProp->m_PrimitiveType == xiiMeshPrimitive::Pyramid)
  {
    geom.AddPyramid(xiiVec3(1.0f), pProp->m_bCap, opt);
  }
  else if (pProp->m_PrimitiveType == xiiMeshPrimitive::Rect)
  {
    opt.m_Transform.Element(2, 0) = -opt.m_Transform.Element(2, 0);
    opt.m_Transform.Element(2, 1) = -opt.m_Transform.Element(2, 1);
    opt.m_Transform.Element(2, 2) = -opt.m_Transform.Element(2, 2);

    geom.AddRectXY(xiiVec2(1.0f), xiiMath::Max<xiiUInt16>(1, detail1), xiiMath::Max<xiiUInt16>(1, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == xiiMeshPrimitive::Sphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 32;

    geom.AddSphere(pProp->m_fRadius, xiiMath::Max<xiiUInt16>(3, detail1), xiiMath::Max<xiiUInt16>(2, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == xiiMeshPrimitive::Torus)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 32;

    float r1 = pProp->m_fRadius;
    float r2 = pProp->m_fRadius2;

    if (r1 == r2)
      r1 = r2 * 0.5f;

    geom.AddTorus(r1, xiiMath::Max(r1 + 0.01f, r2), xiiMath::Max<xiiUInt16>(3, detail1), xiiMath::Max<xiiUInt16>(3, detail2), true, opt);
  }

  geom.TriangulatePolygons(4);
  geom.ComputeTangents();

  // Material setup.
  {
    // Ensure there is just one slot.
    if (pProp->m_Slots.GetCount() != 1)
    {
      GetObjectAccessor()->StartTransaction("Update Mesh Material Info");

      pProp->m_Slots.SetCount(1);
      pProp->m_Slots[0].m_sLabel = "Default";

      ApplyNativePropertyChangesToObjectManager();
      GetObjectAccessor()->FinishTransaction();

      // Need to reacquire pProp pointer since it might be reallocated.
      pProp = GetProperties();
    }

    // Set material for mesh.
    if (!pProp->m_Slots.IsEmpty())
      desc.SetMaterial(0, pProp->m_Slots[0].m_sResource);
    else
      desc.SetMaterial(0, "");
  }

  auto& mbd = desc.MeshBufferDesc();
  mbd.AddStream(xiiGALVertexAttributeSemantic::Position, xiiGALResourceFormat::XYZFloat);
  mbd.AddStream(xiiGALVertexAttributeSemantic::TexCoord0, xiiMeshTexCoordPrecision::ToResourceFormat(pProp->m_TexCoordPrecision));
  mbd.AddStream(xiiGALVertexAttributeSemantic::Normal, xiiMeshNormalPrecision::ToResourceFormatNormal(pProp->m_NormalPrecision));
  mbd.AddStream(xiiGALVertexAttributeSemantic::Tangent, xiiMeshNormalPrecision::ToResourceFormatTangent(pProp->m_NormalPrecision));

  mbd.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::Triangles);
  desc.AddSubMesh(mbd.GetPrimitiveCount(), 0, 0);
}

xiiTransformStatus xiiMeshAssetDocument::CreateMeshFromFile(xiiMeshAssetProperties* pProp, xiiMeshResourceDescriptor& desc, bool bAllowMaterialImport)
{
  xiiProgressRange range("Mesh Import", 5, false);

  range.SetStepWeighting(0, 0.7f);
  range.BeginNextStep("Importing Mesh Data");

  xiiStringBuilder sAbsFilename = pProp->m_sMeshFile;
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return xiiStatus(xiiFmt("Couldn't make path absolute: '{0};", sAbsFilename));
  }

  xiiUniquePtr<xiiModelImporter2::Importer> pImporter = xiiModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return xiiStatus("No known importer for this file type.");

  xiiModelImporter2::ImportOptions opt;
  opt.m_sSourceFile            = sAbsFilename;
  opt.m_bRecomputeNormals      = pProp->m_bRecalculateNormals;
  opt.m_bRecomputeTangents     = pProp->m_bRecalculateTrangents;
  opt.m_pMeshOutput            = &desc;
  opt.m_MeshNormalsPrecision   = pProp->m_NormalPrecision;
  opt.m_MeshTexCoordsPrecision = pProp->m_TexCoordPrecision;
  opt.m_RootTransform          = CalculateTransformationMatrix(pProp);

  if (pImporter->Import(opt).Failed())
    return xiiStatus("Model importer was unable to read this asset.");

  range.BeginNextStep("Importing Materials");

  // correct the number of material slots
  bool bSlotCountMissmatch = pProp->m_Slots.GetCount() != desc.GetSubMeshes().GetCount();
  if (pProp->m_bImportMaterials || bSlotCountMissmatch)
  {
    if (!bAllowMaterialImport && bSlotCountMissmatch)
    {
      return xiiTransformStatus(xiiTransformResult::NeedsImport);
    }

    GetObjectAccessor()->StartTransaction("Update Mesh Materials");

    xiiMeshImportUtils::SetMeshAssetMaterialSlots(pProp->m_Slots, pImporter.Borrow());

    if (pProp->m_bImportMaterials)
    {
      xiiMeshImportUtils::ImportMeshAssetMaterials(pProp->m_Slots, GetDocumentPath(), pImporter.Borrow());
    }

    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }

  xiiMeshImportUtils::CopyMeshAssetMaterialSlotToResource(desc, pProp->m_Slots);

  return xiiStatus(XII_SUCCESS);
}

xiiTransformStatus xiiMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  xiiStatus status = xiiAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

void xiiMeshAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (GetProperties()->m_PrimitiveType != xiiMeshPrimitive::File)
  {
    // remove the mesh file dependency, if it is not actually used
    const auto& sMeshFile = GetProperties()->m_sMeshFile;
    pInfo->m_AssetTransformDependencies.Remove(sMeshFile);
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiMeshAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiMeshAssetDocumentGenerator::xiiMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
  AddSupportedFileType("vox");
}

xiiMeshAssetDocumentGenerator::~xiiMeshAssetDocumentGenerator() {}

void xiiMeshAssetDocumentGenerator::GetImportModes(xiiStringView sParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  xiiStringBuilder baseOutputFile = sParentDirRelativePath;
  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  {
    xiiAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority                       = xiiAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName                          = "MeshImport.WithMaterials";
    info.m_sOutputFileParentRelative      = baseOutputFile;
    info.m_sIcon                          = ":/AssetIcons/Mesh.png";
  }

  {
    xiiAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info.m_sName                          = "MeshImport.NoMaterials";
    info.m_sOutputFileParentRelative      = baseOutputFile;
    info.m_sIcon                          = ":/AssetIcons/Mesh.png";
  }
}

xiiStatus xiiMeshAssetDocumentGenerator::Generate(xiiStringView sDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument)
{
  auto pApp = xiiQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, xiiDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return xiiStatus("Could not create target document");

  xiiMeshAssetDocument* pAssetDoc = xiiDynamicCast<xiiMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return xiiStatus("Target document is not a valid xiiMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sDataDirRelativePath);

  if (info.m_sName == "MeshImport.WithMaterials")
  {
    accessor.SetValue("ImportMaterials", true);
  }

  if (info.m_sName == "MeshImport.NoMaterials")
  {
    accessor.SetValue("ImportMaterials", false);
  }

  return xiiStatus(XII_SUCCESS);
}
