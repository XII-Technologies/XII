#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAsset.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiImageDataAssetDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiImageDataAssetDocument::xiiImageDataAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiImageDataAssetProperties>(sDocumentPath, xiiAssetDocEngineConnection::None)
{
}

xiiTransformStatus xiiImageDataAssetDocument::InternalTransformAsset(xiiStringView sTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const bool bUpdateThumbnail = pAssetProfile == xiiAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

  xiiStatus result = RunTexConv(sTargetFile, AssetHeader, bUpdateThumbnail);

  xiiFileStats stat;
  if (xiiOSFile::GetFileStats(sTargetFile, stat).Succeeded() && stat.m_uiFileSize == 0)
  {
    // if the file was touched, but nothing written to it, delete the file
    // might happen if TexConv crashed or had an error
    xiiOSFile::DeleteFile(sTargetFile).IgnoreResult();
    result.m_Result = XII_FAILURE;
  }

  if (result.Succeeded())
  {
    xiiImageDataAssetEvent e;
    e.m_Type = xiiImageDataAssetEvent::Type::Transformed;
    m_Events.Broadcast(e);
  }

  return result;
}

xiiStatus xiiImageDataAssetDocument::RunTexConv(xiiStringView sTargetFile, const xiiAssetFileHeader& AssetHeader, bool bUpdateThumbnail)
{
  const xiiImageDataAssetProperties* pProp = GetProperties();

  QStringList      arguments;
  xiiStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << xiiConversionUtils::ToString(AssetHeader.GetFileVersion(), temp).GetData();
  }

  // Asset Hash
  {
    const xiiUInt64 uiHash64     = AssetHeader.GetFileHash();
    const xiiUInt32 uiHashLow32  = uiHash64 & 0xFFFFFFFF;
    const xiiUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.SetFormat("{0}", xiiArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.SetFormat("{0}", xiiArgU(uiHashHigh32, 8, true, 16, true));
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }

  arguments << "-out";
  arguments << sTargetFile.GetData(temp);

  const xiiStringBuilder sThumbnail = GetThumbnailFilePath();

  if (bUpdateThumbnail)
  {
    // Thumbnail
    const xiiStringBuilder sDir = sThumbnail.GetFileDirectory();
    xiiOSFile::CreateDirectoryStructure(sDir).IgnoreResult();

    arguments << "-thumbnailRes";
    arguments << "256";
    arguments << "-thumbnailOut";

    arguments << QString::fromUtf8(sThumbnail.GetData());
  }

  arguments << "-mipmaps";
  arguments << "None";

  arguments << "-type";
  arguments << "2D";

  arguments << "-compression";
  arguments << "None";

  arguments << "-usage";
  arguments << "Linear";

  // arguments << "-maxRes" << QString::number(pAssetConfig->m_uiMaxResolution);

  {
    arguments << "-in0";

    xiiStringBuilder sPath = pProp->m_sInputFile;
    sPath.MakeCleanPath();

    if (!sPath.IsAbsolutePath())
    {
      xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
    }

    arguments << QString(sPath.GetData());
  }

  arguments << "-rgba";
  arguments << "in0.rgba";

  XII_SUCCEED_OR_RETURN(xiiQtEditorApp::GetSingleton()->ExecuteTool("xiiTexConv", arguments, 180, xiiLog::GetThreadLocalLogSystem()));

  if (bUpdateThumbnail)
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
