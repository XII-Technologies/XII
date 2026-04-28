/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiMaterialAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

const char* const xiiMaterialAssetDocumentManager::s_szShaderOutputTag = "VISUAL_SHADER";

xiiMaterialAssetDocumentManager::xiiMaterialAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiMaterialAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  xiiAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Material", "xiiMaterial");

  m_DocTypeDesc.m_sDocumentTypeName = "Material";
  m_DocTypeDesc.m_sFileExtension    = "xiiMaterialAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Material.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Rendering";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiMaterialAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Material");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinMaterial";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::SupportsThumbnail;
}

xiiMaterialAssetDocumentManager::~xiiMaterialAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiMaterialAssetDocumentManager::OnDocumentManagerEvent, this));
}

xiiString xiiMaterialAssetDocumentManager::GetRelativeOutputFileName(const xiiAssetDocumentTypeDescriptor* pTypeDescriptor, xiiStringView sDataDirectory, xiiStringView sDocumentPath, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile) const
{
  if (sOutputTag.IsEqual(s_szShaderOutputTag))
  {
    xiiStringBuilder sRelativePath(sDocumentPath);
    sRelativePath.MakeRelativeTo(sDataDirectory).IgnoreResult();
    xiiAssetDocumentManager::GenerateOutputFilename(sRelativePath, pAssetProfile, "autogen.xiiShader", false);
    return sRelativePath;
  }

  return SUPER::GetRelativeOutputFileName(pTypeDescriptor, sDataDirectory, sDocumentPath, sOutputTag, pAssetProfile);
}


bool xiiMaterialAssetDocumentManager::IsOutputUpToDate(xiiStringView sDocumentPath, xiiStringView sOutputTag, xiiUInt64 uiHash, const xiiAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  if (sOutputTag.IsEqual(s_szShaderOutputTag))
  {
    const xiiString sTargetFile = GetAbsoluteOutputFileName(pTypeDescriptor, sDocumentPath, sOutputTag);

    xiiStringBuilder sExpectedHeader;
    sExpectedHeader.SetFormat("//{0}|{1}\n", uiHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion());

    xiiFileReader file;
    if (file.Open(sTargetFile, 256).Failed())
      return false;

    // this might happen if writing to the file failed
    if (file.GetFileSize() < sExpectedHeader.GetElementCount())
      return false;

    xiiUInt8         Temp[256]   = {0};
    const xiiUInt32  uiRead      = (xiiUInt32)file.ReadBytes(Temp, sExpectedHeader.GetElementCount());
    xiiStringBuilder sFileHeader = xiiStringView((const char*)&Temp[0], (const char*)&Temp[uiRead]);

    return sFileHeader.IsEqual(sExpectedHeader);
  }

  return xiiAssetDocumentManager::IsOutputUpToDate(sDocumentPath, sOutputTag, uiHash, pTypeDescriptor);
}


void xiiMaterialAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiMaterialAssetDocument>())
      {
        new xiiQtMaterialAssetDocumentWindow(static_cast<xiiMaterialAssetDocument*>(e.m_pDocument)); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiMaterialAssetDocumentManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiMaterialAssetDocument(sPath);
}

void xiiMaterialAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
