#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>

xiiStringView FindRCSSReference(xiiStringView& sRml)
{
  const char* szCurrent = sRml.FindSubString("href");
  if (szCurrent == nullptr)
    return xiiStringView();

  const char* szStart = nullptr;
  const char* szEnd   = nullptr;
  while (*szCurrent != '\0')
  {
    if (*szCurrent == '\"')
    {
      if (szStart == nullptr)
      {
        szStart = szCurrent + 1;
      }
      else
      {
        szEnd = szCurrent;
        break;
      }
    }

    ++szCurrent;
  }

  if (szStart != nullptr && szEnd != nullptr)
  {
    sRml.SetStartPosition(szEnd);

    xiiStringView rcss = xiiStringView(szStart, szEnd);
    if (rcss.EndsWith_NoCase(".rcss"))
    {
      return rcss;
    }
  }

  return xiiStringView();
}

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRmlUiAssetDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRmlUiAssetDocument::xiiRmlUiAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiRmlUiAssetProperties>(szDocumentPath, xiiAssetDocEngineConnection::Simple)
{
}

xiiTransformStatus xiiRmlUiAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  xiiRmlUiAssetProperties* pProp = GetProperties();

  xiiRmlUiResourceDescriptor desc;
  desc.m_sRmlFile            = pProp->m_sRmlFile;
  desc.m_ScaleMode           = pProp->m_ScaleMode;
  desc.m_ReferenceResolution = pProp->m_ReferenceResolution;

  desc.m_DependencyFile.AddFileDependency(pProp->m_sRmlFile);

  // Find rcss dependencies
  {
    xiiStringBuilder sContent;
    {
      xiiFileReader reader;
      if (reader.Open(pProp->m_sRmlFile).Failed())
        return xiiStatus("Failed to read rml file");

      sContent.ReadAll(reader);
    }

    xiiStringBuilder sRmlFilePath = pProp->m_sRmlFile;
    sRmlFilePath                  = sRmlFilePath.GetFileDirectory();

    xiiStringView sContentView = sContent;

    while (true)
    {
      xiiStringView rcssReference = FindRCSSReference(sContentView);
      if (rcssReference.IsEmpty())
        break;

      xiiStringBuilder sRcssRef = rcssReference;
      if (!xiiFileSystem::ExistsFile(sRcssRef))
      {
        xiiStringBuilder sTemp;
        sTemp.AppendPath(sRmlFilePath, sRcssRef);
        sRcssRef = sTemp;
      }

      if (xiiFileSystem::ExistsFile(sRcssRef))
      {
        desc.m_DependencyFile.AddFileDependency(sRcssRef);
      }
      else
      {
        xiiLog::Warning("RCSS file '{}' was not added as dependency since it doesn't exist", sRcssRef);
      }
    }
  }

  XII_SUCCEED_OR_RETURN(desc.Save(stream));

  return xiiStatus(XII_SUCCESS);
}

xiiTransformStatus xiiRmlUiAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  xiiStatus status = xiiAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}
