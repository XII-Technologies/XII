#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class xiiTextureAssetProfileConfig : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureAssetProfileConfig, xiiProfileConfigData);

public:
  xiiUInt16 m_uiMaxResolution = 1024 * 16;
};

class xiiTextureAssetDocumentManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiTextureAssetDocumentManager();
  ~xiiTextureAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override { return xiiAssetDocumentManager::OutputReliability::Perfect; }

private:
  void OnDocumentManagerEvent(const xiiDocumentManager::Event& e);

  virtual xiiUInt64 ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const override;

  virtual void InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual xiiString GetRelativeOutputFileName(const xiiAssetDocumentTypeDescriptor* pTypeDescriptor, const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile) const override;

private:
  xiiAssetDocumentTypeDescriptor m_DocTypeDesc;
  xiiAssetDocumentTypeDescriptor m_DocTypeDesc2;
};
