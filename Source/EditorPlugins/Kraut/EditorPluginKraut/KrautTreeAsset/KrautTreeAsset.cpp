#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/Progress.h>
#include <KrautGenerator/Serialization/SerializeTree.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Material/MaterialResource.h>

using namespace AE_NS_FOUNDATION;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiKrautTreeAssetDocument, 4, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiKrautTreeAssetDocument::xiiKrautTreeAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiKrautTreeAssetProperties>(szDocumentPath, xiiAssetDocEngineConnection::Simple, true)
{
}

//////////////////////////////////////////////////////////////////////////

class KrautStreamIn : public aeStreamIn
{
public:
  xiiStreamReader* m_pStream = nullptr;

private:
  virtual aeUInt32 ReadFromStream(void* pData, aeUInt32 uiSize) override { return (aeUInt32)m_pStream->ReadBytes(pData, uiSize); }
};

static void GetMaterialLabel(xiiStringBuilder& out, xiiKrautBranchType branchType, xiiKrautMaterialType materialType)
{
  out.Clear();

  switch (branchType)
  {
    case xiiKrautBranchType::Trunk1:
    case xiiKrautBranchType::Trunk2:
    case xiiKrautBranchType::Trunk3:
      out.Format("Trunk {}", (int)branchType - (int)xiiKrautBranchType::Trunk1 + 1);
      break;

    case xiiKrautBranchType::MainBranches1:
    case xiiKrautBranchType::MainBranches2:
    case xiiKrautBranchType::MainBranches3:
      out.Format("Branch {}", (int)branchType - (int)xiiKrautBranchType::MainBranches1 + 1);
      break;

    case xiiKrautBranchType::SubBranches1:
    case xiiKrautBranchType::SubBranches2:
    case xiiKrautBranchType::SubBranches3:
      out.Format("Twig {}", (int)branchType - (int)xiiKrautBranchType::SubBranches1 + 1);
      break;

    case xiiKrautBranchType::Twigs1:
    case xiiKrautBranchType::Twigs2:
    case xiiKrautBranchType::Twigs3:
      out.Format("Twigy {}", (int)branchType - (int)xiiKrautBranchType::Twigs1 + 1);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  switch (materialType)
  {
    case xiiKrautMaterialType::Branch:
      out.Append(" - Stem");
      break;
    case xiiKrautMaterialType::Frond:
      out.Append(" - Frond");
      break;
    case xiiKrautMaterialType::Leaf:
      out.Append(" - Leaf");
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

xiiTransformStatus xiiKrautTreeAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  xiiProgressRange range("Transforming Asset", 2, false);

  xiiKrautTreeAssetProperties* pProp = GetProperties();

  if (!xiiPathUtils::HasExtension(pProp->m_sKrautFile, ".tree"))
    return xiiStatus("Unsupported file format");

  xiiKrautGeneratorResourceDescriptor desc;

  // read the input data
  {
    xiiFileReader krautFile;
    if (krautFile.Open(pProp->m_sKrautFile).Failed())
      return xiiStatus(xiiFmt("Could not open Kraut file '{0}'", pProp->m_sKrautFile));

    KrautStreamIn kstream;
    kstream.m_pStream = &krautFile;

    xiiUInt32 uiKrautEditorVersion = 0;
    krautFile >> uiKrautEditorVersion;

    Kraut::Deserializer ts;
    ts.m_pTreeStructure = &desc.m_TreeStructureDesc;
    ts.m_LODs[0]        = &desc.m_LodDesc[0];
    ts.m_LODs[1]        = &desc.m_LodDesc[1];
    ts.m_LODs[2]        = &desc.m_LodDesc[2];
    ts.m_LODs[3]        = &desc.m_LodDesc[3];
    ts.m_LODs[4]        = &desc.m_LodDesc[4];

    if (!ts.Deserialize(kstream))
    {
      return xiiStatus(xiiFmt("Reading the Kraut file failed: '{}'", pProp->m_sKrautFile));
    }
  }

  // find materials
  {
    desc.m_Materials.Clear();

    xiiStringBuilder materialLabel;

    for (xiiUInt32 bt = 0; bt < Kraut::BranchType::ENUM_COUNT; ++bt)
    {
      const auto& type = desc.m_TreeStructureDesc.m_BranchTypes[bt];

      if (!type.m_bUsed)
        continue;

      for (xiiUInt32 gt = 0; gt < Kraut::BranchGeometryType::ENUM_COUNT; ++gt)
      {
        if (!type.m_bEnable[gt])
          continue;

        auto& m = desc.m_Materials.ExpandAndGetRef();

        m.m_MaterialType = static_cast<xiiKrautMaterialType>((int)xiiKrautMaterialType::Branch + gt);
        m.m_BranchType   = static_cast<xiiKrautBranchType>((int)xiiKrautBranchType::Trunk1 + bt);

        GetMaterialLabel(materialLabel, m.m_BranchType, m.m_MaterialType);

        // find the matching material from the user input (don't want to guess an index, in case the list size changed)
        for (const auto& mat : pProp->m_Materials)
        {
          if (mat.m_sLabel == materialLabel)
          {
            m.m_hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(mat.m_sMaterial);
            break;
          }
        }
      }
    }
  }

  // write the output data
  {
    desc.m_sSurfaceResource      = pProp->m_sSurface;
    desc.m_fStaticColliderRadius = pProp->m_fStaticColliderRadius;
    desc.m_fUniformScaling       = pProp->m_fUniformScaling;
    desc.m_fLodDistanceScale     = pProp->m_fLodDistanceScale;
    desc.m_GoodRandomSeeds       = pProp->m_GoodRandomSeeds;
    desc.m_uiDefaultDisplaySeed  = pProp->m_uiRandomSeedForDisplay;
    desc.m_fTreeStiffness        = pProp->m_fTreeStiffness;

    if (desc.Serialize(stream).Failed())
    {
      return xiiStatus("Writing KrautGenerator resource descriptor failed.");
    }
  }

  SyncBackAssetProperties(pProp, desc);

  return xiiStatus(XII_SUCCESS);
}

void xiiKrautTreeAssetDocument::SyncBackAssetProperties(xiiKrautTreeAssetProperties*& pProp, const xiiKrautGeneratorResourceDescriptor& desc)
{
  bool bModified = pProp->m_Materials.GetCount() != desc.m_Materials.GetCount();

  pProp->m_Materials.SetCount(desc.m_Materials.GetCount());

  xiiStringBuilder newLabel;

  // TODO: match up old and new materials by label name

  for (xiiUInt32 m = 0; m < pProp->m_Materials.GetCount(); ++m)
  {
    auto& mat = pProp->m_Materials[m];

    GetMaterialLabel(newLabel, desc.m_Materials[m].m_BranchType, desc.m_Materials[m].m_MaterialType);

    if (newLabel != mat.m_sLabel)
    {
      mat.m_sLabel = newLabel;
      bModified    = true;
    }
  }

  if (bModified)
  {
    GetObjectAccessor()->StartTransaction("Update Kraut Material Info");
    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }
}

xiiTransformStatus xiiKrautTreeAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  xiiStatus status = xiiAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiKrautTreeAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiKrautTreeAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiKrautTreeAssetDocumentGenerator::xiiKrautTreeAssetDocumentGenerator()
{
  AddSupportedFileType("tree");
}

xiiKrautTreeAssetDocumentGenerator::~xiiKrautTreeAssetDocumentGenerator() = default;

void xiiKrautTreeAssetDocumentGenerator::GetImportModes(xiiStringView sParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  xiiStringBuilder baseOutputFile = sParentDirRelativePath;
  baseOutputFile.ChangeFileExtension("xiiKrautTreeAsset");

  {
    xiiAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority                       = xiiAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName                          = "KrautTreeImport.Tree";
    info.m_sOutputFileParentRelative      = baseOutputFile;
    info.m_sIcon                          = ":/AssetIcons/Kraut_Tree.png";
  }
}

xiiStatus xiiKrautTreeAssetDocumentGenerator::Generate(xiiStringView sDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument)
{
  auto pApp = xiiQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, xiiDocumentFlags::None);

  if (out_pGeneratedDocument == nullptr)
    return xiiStatus("Could not create target document");

  xiiKrautTreeAssetDocument* pAssetDoc = xiiDynamicCast<xiiKrautTreeAssetDocument*>(out_pGeneratedDocument);

  if (pAssetDoc == nullptr)
    return xiiStatus("Target document is not a valid xiiKrautTreeAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("KrautFile", sDataDirRelativePath);

  return xiiStatus(XII_SUCCESS);
}
