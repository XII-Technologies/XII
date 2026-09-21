/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class xiiDecalAssetDocumentManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiDecalAssetDocumentManager();
  ~xiiDecalAssetDocumentManager();

  virtual void      AddEntriesToAssetTable(xiiStringView sDataDirectory, const xiiPlatformProfile* pAssetProfile, xiiDelegate<void(xiiStringView sGuid, xiiStringView sPath, xiiStringView sType)> addEntry) const override;
  virtual xiiString GetAssetTableEntry(const xiiSubAsset* pSubAsset, xiiStringView sDataDirectory, const xiiPlatformProfile* pAssetProfile) const override;

  /// There is only a single decal texture per project. This function creates it, in case any decal asset was modified.
  xiiStatus GenerateDecalTexture(const xiiPlatformProfile* pAssetProfile);
  xiiString GetDecalTexturePath(const xiiPlatformProfile* pAssetProfile) const;

private:
  void      OnDocumentManagerEvent(const xiiDocumentManager::Event& e);
  bool      IsDecalTextureUpToDate(const char* szDecalFile, xiiUInt64 uiAssetHash) const;
  xiiStatus RunTextureConverter(const char* szTargetFile, const char* szInputFile, const xiiAssetFileHeader& AssetHeader);

  virtual void InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual xiiUInt64 ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const override;

  xiiAssetDocumentTypeDescriptor m_DocTypeDesc;
};
