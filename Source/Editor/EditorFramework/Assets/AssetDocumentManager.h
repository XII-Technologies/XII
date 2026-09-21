/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/Declarations.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct xiiSubAsset;
class xiiPlatformProfile;

class XII_EDITORFRAMEWORK_DLL xiiAssetDocumentManager : public xiiDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAssetDocumentManager, xiiDocumentManager);

public:
  xiiAssetDocumentManager();
  ~xiiAssetDocumentManager();

  /// Opens the asset file and reads the "Header" into the given xiiAssetDocumentInfo.
  virtual xiiStatus ReadAssetDocumentInfo(xiiUniquePtr<xiiAssetDocumentInfo>& out_pInfo, xiiStreamReader& inout_stream) const;
  virtual void      FillOutSubAssetList(const xiiAssetDocumentInfo& assetInfo, xiiDynamicArray<xiiSubAssetData>& out_subAssets) const {}

  /// If this asset type has additional output files that need to be generated (like a texture atlas that combines outputs from multiple assets)
  /// this function should make sure those files are all generated and return the list of relative file paths (from the data directory root).
  virtual xiiStatus GetAdditionalOutputs(xiiDynamicArray<xiiString>& ref_files) { return XII_SUCCESS; }

  // xiiDocumentManager overrides:
public:
  virtual xiiStatus CloneDocument(xiiStringView sPath, xiiStringView sClonePath, xiiUuid& inout_cloneGuid) override;

  /// \name Asset Profile Functions
  ///@{
public:
  /// Called by the xiiAssetCurator when the active asset profile changes to re-compute m_uiAssetProfileHash.
  void ComputeAssetProfileHash(const xiiPlatformProfile* pAssetProfile);

  /// Returns the hash that was previously computed through ComputeAssetProfileHash().
  XII_ALWAYS_INLINE xiiUInt64 GetAssetProfileHash() const { return m_uiAssetProfileHash; }

  /// Returns pAssetProfile, or if that is null, xiiAssetCurator::GetSingleton()->GetActiveAssetProfile().
  static const xiiPlatformProfile* DetermineFinalTargetProfile(const xiiPlatformProfile* pAssetProfile);

private:
  virtual xiiUInt64 ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const;

  // The hash that is combined with the asset document hash to determine whether the document output is up to date.
  // This hash needs to be computed in ComputeAssetProfileHash() and should reflect all important settings from the givne asset profile that
  // affect the asset output for this manager.
  // However, if GeneratesProfileSpecificAssets() return false, the hash must be zero, as then all outputs must be identical in all
  // profiles.
  xiiUInt64 m_uiAssetProfileHash = 0;

  ///@}
  /// \name Thumbnail Functions
  ///@{
public:
  /// Returns the absolute path to the thumbnail that belongs to the given document.
  virtual xiiString GenerateResourceThumbnailPath(xiiStringView sDocumentPath, xiiStringView sSubAssetName = xiiStringView());
  virtual bool      IsThumbnailUpToDate(xiiStringView sDocumentPath, xiiStringView sSubAssetName, xiiUInt64 uiThumbnailHash, xiiUInt32 uiTypeVersion);

  ///@}
  /// \name Output Functions
  ///@{

  virtual void      AddEntriesToAssetTable(xiiStringView sDataDirectory, const xiiPlatformProfile* pAssetProfile, xiiDelegate<void(xiiStringView sGuid, xiiStringView sPath, xiiStringView sType)> addEntry) const;
  virtual xiiString GetAssetTableEntry(const xiiSubAsset* pSubAsset, xiiStringView sDataDirectory, const xiiPlatformProfile* pAssetProfile) const;

  /// Calls GetRelativeOutputFileName and prepends [DataDir]/AssetCache/ .
  xiiString GetAbsoluteOutputFileName(const xiiAssetDocumentTypeDescriptor* pTypeDesc, xiiStringView sDocumentPath, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile = nullptr) const;

  /// Relative to 'AssetCache' folder.
  virtual xiiString GetRelativeOutputFileName(const xiiAssetDocumentTypeDescriptor* pTypeDesc, xiiStringView sDataDirectory, xiiStringView sDocumentPath, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile = nullptr) const;
  virtual bool      GeneratesProfileSpecificAssets() const = 0;

  bool         IsOutputUpToDate(xiiStringView sDocumentPath, const xiiDynamicArray<xiiString>& outputs, xiiUInt64 uiHash, const xiiAssetDocumentTypeDescriptor* pTypeDescriptor);
  virtual bool IsOutputUpToDate(xiiStringView sDocumentPath, xiiStringView sOutputTag, xiiUInt64 uiHash, const xiiAssetDocumentTypeDescriptor* pTypeDescriptor);

  /// Describes how likely it is that a generated file is 'corrupted', due to dependency issues and such.
  /// For example a prefab may not work correctly, if it was written with a very different C++ plugin state, but this can't be detected later.
  /// Whereas a texture always produces exactly the same output and is thus perfectly reliable.
  /// This is used to clear asset caches selectively, and keep things that are unlikely to be in a broken state.
  enum OutputReliability : xiiUInt8
  {
    Unknown = 0,
    Good    = 1,
    Perfect = 2,
  };

  /// \see OutputReliability
  virtual OutputReliability GetAssetTypeOutputReliability() const { return OutputReliability::Unknown; }

  ///@}


  /// Called by the editor to try to open a document for the matching picking result
  virtual xiiResult OpenPickedDocument(const xiiDocumentObject* pPickedComponent, xiiUInt32 uiPartIndex) { return XII_FAILURE; }

  xiiResult TryOpenAssetDocument(const char* szPathOrGuid);

  /// In case this manager deals with types that need to be force transformed on scene export, it can add the asset type names to this list.
  /// This is only needed for assets that have such special dependencies for their transform step, that the regular dependency tracking doesn't work for them.
  /// Currently the only known case are Collection assets, because they have to manually go through the Package dependencies transitively, which means
  /// that the asset curator can't know when they need to be updated.
  virtual void GetAssetTypesRequiringTransformForSceneExport(xiiSet<xiiTempHashedString>& inout_assetTypes) {};

protected:
  static bool IsResourceUpToDate(const char* szResourceFile, xiiUInt64 uiHash, xiiUInt16 uiTypeVersion);
  static void GenerateOutputFilename(xiiStringBuilder& inout_sRelativeDocumentPath, const xiiPlatformProfile* pAssetProfile, const char* szExtension, bool bPlatformSpecific);
};
