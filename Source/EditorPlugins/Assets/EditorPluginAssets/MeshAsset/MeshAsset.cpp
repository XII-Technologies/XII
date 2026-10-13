/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/Progress.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <ModelImporter/MeshImport.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshAssetDocument, 13, xiiRTTINoAllocator)
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
    XII_SUCCEED_OR_RETURN(CreateMeshFromGeom(pProp, desc, !transformFlags.IsSet(xiiTransformFlags::BackgroundProcessing)));
  }

  // if there is no material set for a slot, use the "Pattern" material as a fallback
  for (xiiUInt32 matIdx = 0; matIdx < desc.GetMaterials().GetCount(); ++matIdx)
  {
    if (desc.GetMaterials()[matIdx].IsEmpty())
    {
      // Data/Base/Materials/Common/Pattern.xiiMaterialAsset
      desc.SetMaterial(matIdx, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");
    }
  }

  range.BeginNextStep("Writing Result");
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  desc.Save(writer);
  return storage.CopyToStream(stream);
}


xiiTransformStatus xiiMeshAssetDocument::CreateMeshFromGeom(xiiMeshAssetProperties* pProp, xiiMeshResourceDescriptor& desc, bool bAllowMaterialImport)
{
  const xiiMat3 mTransformation = CalculateTransformationMatrix(pProp);

  xiiGeometry geom;

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

  xiiHybridArray<xiiMaterialResourceSlot, 8> materialSlots = pProp->m_Slots;
  const xiiStringView                        labels[]      = {"Default"};
  xiiMeshMaterialSlots::Reconcile(labels, materialSlots);
  if (!bAllowMaterialImport && !xiiMeshMaterialSlots::Equal(materialSlots, pProp->m_Slots))
    return xiiTransformStatus(xiiTransformResult::NeedsImport);
  auto& buffer = desc.GetMeshBufferDescriptor();
  buffer.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList, false);
  if (buffer.GetPrimitiveCount() == 0)
    return xiiStatus("Procedural mesh contains no triangles.");
  desc.AddMaterialSlot(materialSlots[0].m_sResource);
  desc.AddSection(buffer.GetPrimitiveCount(), 0, 0);
  desc.ComputeBounds();
  XII_SUCCEED_OR_RETURN(desc.BuildMeshlets());
  if (!xiiMeshMaterialSlots::Equal(materialSlots, pProp->m_Slots))
  {
    GetObjectAccessor()->StartTransaction("Update Mesh Material Info");
    pProp->m_Slots = materialSlots;
    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();
  }
  return XII_SUCCESS;
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

  xiiModelMeshImportOptions options;
  options.m_bRecalculateNormals   = pProp->m_bRecalculateNormals;
  options.m_bRecalculateTangents  = pProp->m_bRecalculateTangents;
  options.m_VertexColorConversion = pProp->m_VertexColorConversion;
  options.m_RootTransform         = xiiMat4(CalculateTransformationMatrix(pProp), xiiVec3::MakeZero());
  if (pProp->m_bSimplifyMesh)
  {
    // Authored 100% previously meant maximum simplification, not an empty mesh.
    options.m_fSimplification           = xiiMath::Min<xiiUInt32>(pProp->m_uiMeshSimplification, 99) / 100.0f;
    options.m_fMaxSimplificationError   = pProp->m_uiMaxSimplificationError / 100.0f;
    options.m_bAggressiveSimplification = pProp->m_bAggressiveSimplification;
  }
  xiiModelMeshImport imported;
  xiiStringBuilder   error;
  if (xiiModelMeshImport::ReadFile(sAbsFilename, options, imported, &error).Failed())
    return xiiStatus(error.GetView());
  range.BeginNextStep("Importing Materials");
  auto materialSlots = pProp->m_Slots;
  XII_SUCCEED_OR_RETURN(xiiMeshImportUtils::PrepareMaterialSlots(materialSlots, pProp->m_bImportMaterials, bAllowMaterialImport, GetDocumentPath(), sAbsFilename, imported));
  XII_SUCCEED_OR_RETURN(xiiMeshImportUtils::CopyMeshAssetMaterialSlotToResource(imported.m_Mesh, materialSlots));
  if (!xiiMeshMaterialSlots::Equal(materialSlots, pProp->m_Slots))
  {
    GetObjectAccessor()->StartTransaction("Update Mesh Materials");
    pProp->m_Slots = std::move(materialSlots);
    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();
  }
  desc = std::move(imported.m_Mesh);
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
