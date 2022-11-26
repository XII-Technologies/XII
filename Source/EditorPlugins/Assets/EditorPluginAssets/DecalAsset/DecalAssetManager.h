#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class xiiDecalAssetDocumentManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiDecalAssetDocumentManager();
  ~xiiDecalAssetDocumentManager();

  virtual void AddEntriesToAssetTable(
    const char*                   szDataDirectory,
    const xiiPlatformProfile*     pAssetProfile,
    xiiMap<xiiString, xiiString>& inout_GuidToPath) const override;
  virtual xiiString GetAssetTableEntry(
    const xiiSubAsset*        pSubAsset,
    const char*               szDataDirectory,
    const xiiPlatformProfile* pAssetProfile) const override;

  /// \brief There is only a single decal texture per project. This function creates it, in case any decal asset was modified.
  xiiStatus GenerateDecalTexture(const xiiPlatformProfile* pAssetProfile);
  xiiString GetDecalTexturePath(const xiiPlatformProfile* pAssetProfile) const;

private:
  void      OnDocumentManagerEvent(const xiiDocumentManager::Event& e);
  bool      IsDecalTextureUpToDate(const char* szDecalFile, xiiUInt64 uiAssetHash) const;
  xiiStatus RunTexConv(const char* szTargetFile, const char* szInputFile, const xiiAssetFileHeader& AssetHeader);

  virtual void InternalCreateDocument(
    const char*              szDocumentTypeName,
    const char*              szPath,
    bool                     bCreateNewDocument,
    xiiDocument*&            out_pDocument,
    const xiiDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual xiiUInt64 ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const override;

  xiiAssetDocumentTypeDescriptor m_DocTypeDesc;
};
