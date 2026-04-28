/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <GraphicsCore/Meshes/MeshResourceDescriptor.h>
#include <ModelImporter2/ModelImporter.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshAssetDocument, 12, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static xiiMat3 CalculateTransformationMatrix(const xiiMeshAssetProperties* pProp)
{
  const float us = xiiMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  auto rightDir = xiiMeshImportTransform::GetRightDir(pProp->m_ImportTransform, pProp->m_RightDir);
  auto upDir    = xiiMeshImportTransform::GetUpDir(pProp->m_ImportTransform, pProp->m_UpDir);
  auto flipFwd  = xiiMeshImportTransform::GetFlipForward(pProp->m_ImportTransform, pProp->m_bFlipForwardDir);

  const xiiBasisAxis::Enum forwardDir = xiiBasisAxis::GetOrthogonalAxis(rightDir, upDir, !flipFwd);

  return xiiBasisAxis::CalculateTransformationMatrix(forwardDir, rightDir, upDir, us);
}

xiiMeshAssetDocument::xiiMeshAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiMeshAssetProperties>(sDocumentPath, xiiAssetDocEngineConnection::Simple, true)
{
}

xiiTransformStatus xiiMeshAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
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

  // if there is no material set for a slot, use the "Pattern" material as a fallback
  for (xiiUInt32 matIdx = 0; matIdx < desc.GetMaterials().GetCount(); ++matIdx)
  {
    if (desc.GetMaterials()[matIdx].m_sPath.IsEmpty())
    {
      // Data/Base/Materials/Common/Pattern.xiiMaterialAsset
      desc.SetMaterial(matIdx, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");
    }
  }

  range.BeginNextStep("Writing Result");
  desc.Save(stream);

  return XII_SUCCESS;
}


void xiiMeshAssetDocument::CreateMeshFromGeom(xiiMeshAssetProperties* pProp, xiiMeshResourceDescriptor& desc)
{
  const xiiMat3 mTransformation = CalculateTransformationMatrix(pProp);

  xiiGeometry geom;
  // const xiiMat4 mTrans(mTransformation, xiiVec3::MakeZero());

  xiiGeometry::GeoOptions opt;
  opt.m_Transform = xiiMat4(mTransformation, xiiVec3::MakeZero());

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

    geom.AddCylinder(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f, pProp->m_bCap, pProp->m_bCap2, xiiMath::Max<xiiUInt16>(3, detail1), opt, xiiMath::Clamp(pProp->m_Angle, xiiAngle::MakeFromDegree(0.0f), xiiAngle::MakeFromDegree(360.0f)));
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
    geom.AddPyramid(1.0f, 1.0f, pProp->m_bCap, opt);
  }
  else if (pProp->m_PrimitiveType == xiiMeshPrimitive::Rect)
  {
    opt.m_Transform.Element(2, 0) = -opt.m_Transform.Element(2, 0);
    opt.m_Transform.Element(2, 1) = -opt.m_Transform.Element(2, 1);
    opt.m_Transform.Element(2, 2) = -opt.m_Transform.Element(2, 2);

    geom.AddRect(xiiVec2(1.0f), xiiMath::Max<xiiUInt16>(1, detail1), xiiMath::Max<xiiUInt16>(1, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == xiiMeshPrimitive::Sphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 32;

    geom.AddStackedSphere(pProp->m_fRadius, xiiMath::Max<xiiUInt16>(3, detail1), xiiMath::Max<xiiUInt16>(2, detail2), opt);
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

  // the the procedurally generated geometry we can always use fixed, low precision data, because we know that the geometry isn't detailed enough to run into problems
  // and then we can unclutter the UI a little by not showing those options at all
  auto& mbd = desc.MeshBufferDesc();
  mbd.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALResourceFormat::RGB32Float);
  mbd.AddStream(xiiGALInputLayoutSemantic::TexCoord0, xiiMeshTexCoordPrecision::ToResourceFormat(xiiMeshTexCoordPrecision::_16Bit /*pProp->m_TexCoordPrecision*/));
  mbd.AddStream(xiiGALInputLayoutSemantic::Normal, xiiMeshNormalPrecision::ToResourceFormatNormal(xiiMeshNormalPrecision::_10Bit /*pProp->m_NormalPrecision*/));
  mbd.AddStream(xiiGALInputLayoutSemantic::Tangent, xiiMeshNormalPrecision::ToResourceFormatTangent(xiiMeshNormalPrecision::_10Bit /*pProp->m_NormalPrecision*/));

  mbd.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList);
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
  opt.m_sSourceFile               = sAbsFilename;
  opt.m_bRecomputeNormals         = pProp->m_bRecalculateNormals;
  opt.m_bRecomputeTangents        = pProp->m_bRecalculateTrangents;
  opt.m_pMeshOutput               = &desc;
  opt.m_MeshNormalsPrecision      = pProp->m_NormalPrecision;
  opt.m_MeshTexCoordsPrecision    = pProp->m_TexCoordPrecision;
  opt.m_MeshVertexColorConversion = pProp->m_VertexColorConversion;
  opt.m_RootTransform             = CalculateTransformationMatrix(pProp);

  if (pProp->m_bSimplifyMesh)
  {
    opt.m_uiMeshSimplification      = pProp->m_uiMeshSimplification;
    opt.m_uiMaxSimplificationError  = pProp->m_uiMaxSimplificationError;
    opt.m_bAggressiveSimplification = pProp->m_bAggressiveSimplification;
  }

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

  return XII_SUCCESS;
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
    pInfo->m_TransformDependencies.Remove(sMeshFile);
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

xiiMeshAssetDocumentGenerator::~xiiMeshAssetDocumentGenerator() = default;

void xiiMeshAssetDocumentGenerator::GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    xiiAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority                             = xiiAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName                                = "MeshImport.WithMaterials";
    info.m_sIcon                                = ":/AssetIcons/Mesh.svg";
  }

  {
    xiiAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority                             = xiiAssetDocGeneratorPriority::LowPriority;
    info.m_sName                                = "MeshImport.NoMaterials";
    info.m_sIcon                                = ":/AssetIcons/Mesh.svg";
  }
}

xiiStatus xiiMeshAssetDocumentGenerator::Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDynamicArray<xiiDocument*>& out_generatedDocuments)
{
  xiiStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  xiiOSFile::FindFreeFilename(sOutFile);

  auto pApp = xiiQtEditorApp::GetSingleton();

  xiiStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  xiiDocument* pDoc = pApp->CreateDocument(sOutFile, xiiDocumentFlags::None);
  if (pDoc == nullptr)
    return xiiStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  xiiMeshAssetDocument* pAssetDoc = xiiDynamicCast<xiiMeshAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sInputFileRel.GetView());

  if (sMode == "MeshImport.WithMaterials")
  {
    accessor.SetValue("ImportMaterials", true);
  }

  if (sMode == "MeshImport.NoMaterials")
  {
    accessor.SetValue("ImportMaterials", false);
  }

  return XII_SUCCESS;
}
