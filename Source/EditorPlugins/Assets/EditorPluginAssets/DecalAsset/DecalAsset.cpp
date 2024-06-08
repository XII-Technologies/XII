#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiDecalMode, 1)
  XII_ENUM_CONSTANT(xiiDecalMode::BaseColor),
  XII_ENUM_CONSTANT(xiiDecalMode::BaseColorNormal),
  XII_ENUM_CONSTANT(xiiDecalMode::BaseColorORM),
  XII_ENUM_CONSTANT(xiiDecalMode::BaseColorNormalORM),
  XII_ENUM_CONSTANT(xiiDecalMode::BaseColorEmissive)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalAssetProperties, 3, xiiRTTIDefaultAllocator<xiiDecalAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Mode", xiiDecalMode, m_Mode),
    XII_MEMBER_PROPERTY("BlendModeColorize", m_bBlendModeColorize),
    XII_MEMBER_PROPERTY("AlphaMask", m_sAlphaMask)->AddAttributes(new xiiFileBrowserAttribute("Select Alpha Mask", xiiFileBrowserAttribute::ImagesLdrOnly)),
    XII_MEMBER_PROPERTY("BaseColor", m_sBaseColor)->AddAttributes(new xiiFileBrowserAttribute("Select Base Color Map", xiiFileBrowserAttribute::ImagesLdrOnly)),
    XII_MEMBER_PROPERTY("Normal", m_sNormal)->AddAttributes(new xiiFileBrowserAttribute("Select Normal Map", xiiFileBrowserAttribute::ImagesLdrOnly), new xiiDefaultValueAttribute(xiiStringView("Textures/NeutralNormal.tga"))), // wrap in xiiStringView to prevent a memory leak report
    XII_MEMBER_PROPERTY("ORM", m_sORM)->AddAttributes(new xiiFileBrowserAttribute("Select ORM Map", xiiFileBrowserAttribute::ImagesLdrOnly)),
    XII_MEMBER_PROPERTY("Emissive", m_sEmissive)->AddAttributes(new xiiFileBrowserAttribute("Select Emissive Map", xiiFileBrowserAttribute::ImagesLdrOnly)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDecalAssetProperties::xiiDecalAssetProperties() = default;

void xiiDecalAssetProperties::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiDecalAssetProperties>())
  {
    xiiInt64 mode = e.m_pObject->GetTypeAccessor().GetValue("Mode").ConvertTo<xiiInt64>();

    auto& props = *e.m_pPropertyStates;

    props["Normal"].m_Visibility   = xiiPropertyUiState::Invisible;
    props["ORM"].m_Visibility      = xiiPropertyUiState::Invisible;
    props["Emissive"].m_Visibility = xiiPropertyUiState::Invisible;

    if (mode == xiiDecalMode::BaseColorNormal)
    {
      props["Normal"].m_Visibility = xiiPropertyUiState::Default;
    }
    else if (mode == xiiDecalMode::BaseColorORM)
    {
      props["ORM"].m_Visibility = xiiPropertyUiState::Default;
    }
    else if (mode == xiiDecalMode::BaseColorNormalORM)
    {
      props["Normal"].m_Visibility = xiiPropertyUiState::Default;
      props["ORM"].m_Visibility    = xiiPropertyUiState::Default;
    }
    else if (mode == xiiDecalMode::BaseColorEmissive)
    {
      props["Emissive"].m_Visibility = xiiPropertyUiState::Default;
    }
  }
}

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalAssetDocument, 5, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDecalAssetDocument::xiiDecalAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiDecalAssetProperties>(sDocumentPath, xiiAssetDocEngineConnection::Simple, true)
{
}

xiiTransformStatus xiiDecalAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  return static_cast<xiiDecalAssetDocumentManager*>(GetAssetDocumentManager())->GenerateDecalTexture(pAssetProfile);
}

xiiTransformStatus xiiDecalAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& Unused)
{
  const xiiDecalAssetProperties* pProp = GetProperties();

  QStringList      arguments;
  xiiStringBuilder temp;

  const xiiStringBuilder sThumbnail = GetThumbnailFilePath();

  arguments << "-usage";
  arguments << "Color";

  {
    // Thumbnail
    const xiiStringBuilder sDir = sThumbnail.GetFileDirectory();
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(sDir));

    arguments << "-thumbnailOut";
    arguments << QString::fromUtf8(sThumbnail.GetData());

    arguments << "-thumbnailRes";
    arguments << "256";
  }

  {
    xiiQtEditorApp* pEditorApp = xiiQtEditorApp::GetSingleton();

    temp.SetFormat("-in0");

    xiiStringBuilder sAbsPath = pProp->m_sBaseColor;
    if (!pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
    {
      return xiiStatus(xiiFmt("Failed to make path absolute: '{}'", sAbsPath));
    }

    arguments << temp.GetData();
    arguments << QString(sAbsPath.GetData());

    if (!pProp->m_sAlphaMask.IsEmpty())
    {
      xiiStringBuilder sAbsPath2 = pProp->m_sAlphaMask;
      if (!pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath2))
      {
        return xiiStatus(xiiFmt("Failed to make path absolute: '{}'", sAbsPath2));
      }

      arguments << "-in1";
      arguments << QString(sAbsPath2.GetData());

      arguments << "-rgb";
      arguments << "in0.rgb";

      arguments << "-a";
      arguments << "in1.r";
    }
    else
    {
      arguments << "-rgba";
      arguments << "in0.rgba";
    }
  }

  XII_SUCCEED_OR_RETURN(xiiQtEditorApp::GetSingleton()->ExecuteTool("xiiTexConv", arguments, 180, xiiLog::GetThreadLocalLogSystem()));

  {
    xiiUInt64 uiThumbnailHash = xiiAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    XII_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, thumbnailInfo);
    InvalidateAssetThumbnail();
  }

  return xiiStatus(XII_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiDecalAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDecalAssetDocumentGenerator::xiiDecalAssetDocumentGenerator()
{
  AddSupportedFileType("tga");
  AddSupportedFileType("dds");
  AddSupportedFileType("jpg");
  AddSupportedFileType("jpeg");
  AddSupportedFileType("png");
}

xiiDecalAssetDocumentGenerator::~xiiDecalAssetDocumentGenerator() = default;

void xiiDecalAssetDocumentGenerator::GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const
{
  const xiiStringBuilder baseFilename = sAbsInputFile.GetFileName();

  const bool isDecal = (baseFilename.FindSubString_NoCase("decal") != nullptr);

  {
    xiiAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority                             = isDecal ? xiiAssetDocGeneratorPriority::HighPriority : xiiAssetDocGeneratorPriority::LowPriority;
    info.m_sName                                = "DecalImport.All";
    info.m_sIcon                                = ":/AssetIcons/Decal.svg";
  }
}

xiiStatus xiiDecalAssetDocumentGenerator::Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDocument*& out_pGeneratedDocument)
{
  xiiStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  xiiOSFile::FindFreeFilename(sOutFile);

  auto pApp = xiiQtEditorApp::GetSingleton();

  xiiStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  out_pGeneratedDocument = pApp->CreateDocument(sOutFile, xiiDocumentFlags::None);

  if (out_pGeneratedDocument == nullptr)
    return xiiStatus("Could not create target document");

  xiiDecalAssetDocument* pAssetDoc = xiiDynamicCast<xiiDecalAssetDocument*>(out_pGeneratedDocument);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("BaseColor", sInputFileRel.GetView());

  return xiiStatus(XII_SUCCESS);
}
