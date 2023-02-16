#pragma once

#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
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

  /// \brief Opens the asset file and reads the "Header" into the given xiiAssetDocumentInfo.
  virtual xiiStatus ReadAssetDocumentInfo(xiiUniquePtr<xiiAssetDocumentInfo>& out_pInfo, xiiStreamReader& stream) const;
  virtual void      FillOutSubAssetList(const xiiAssetDocumentInfo& assetInfo, xiiHybridArray<xiiSubAssetData, 4>& out_SubAssets) const {}

  /// If this asset type has additional output files that need to be generated (like a texture atlas that combines outputs from multiple assets)
  /// this function should make sure those files are all generated and return the list of relative file paths (from the data directory root).
  virtual xiiStatus GetAdditionalOutputs(xiiDynamicArray<xiiString>& files) { return xiiStatus(XII_SUCCESS); }

  // xiiDocumentManager overrides:
public:
  virtual xiiStatus CloneDocument(const char* szPath, const char* szClonePath, xiiUuid& inout_cloneGuid) override;

  /// \name Asset Profile Functions
  ///@{
public:
  /// \brief Called by the xiiAssetCurator when the active asset profile changes to re-compute m_uiAssetProfileHash.
  void ComputeAssetProfileHash(const xiiPlatformProfile* pAssetProfile);

  /// \brief Returns the hash that was previously computed through ComputeAssetProfileHash().
  XII_ALWAYS_INLINE xiiUInt64 GetAssetProfileHash() const { return m_uiAssetProfileHash; }

  /// \brief Returns pAssetProfile, or if that is null, xiiAssetCurator::GetSingleton()->GetActiveAssetProfile().
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
  /// \brief Returns the absolute path to the thumbnail that belongs to the given document.
  static xiiString GenerateResourceThumbnailPath(const char* szDocumentPath);
  static bool      IsThumbnailUpToDate(const char* szDocumentPath, xiiUInt64 uiThumbnailHash, xiiUInt32 uiTypeVersion);

  ///@}
  /// \name Output Functions
  ///@{

  virtual void AddEntriesToAssetTable(
    const char*                                                                      szDataDirectory,
    const xiiPlatformProfile*                                                        pAssetProfile,
    xiiDelegate<void(xiiStringView sGuid, xiiStringView sPath, xiiStringView sType)> addEntry) const;
  virtual xiiString GetAssetTableEntry(const xiiSubAsset* pSubAsset, const char* szDataDirectory, const xiiPlatformProfile* pAssetProfile) const;

  /// \brief Calls GetRelativeOutputFileName and prepends [DataDir]/AssetCache/ .
  xiiString GetAbsoluteOutputFileName(const xiiAssetDocumentTypeDescriptor* pTypeDesc, const char* szDocumentPath, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile = nullptr) const;

  /// \brief Relative to 'AssetCache' folder.
  virtual xiiString GetRelativeOutputFileName(const xiiAssetDocumentTypeDescriptor* pTypeDesc, const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile = nullptr) const;
  virtual bool      GeneratesProfileSpecificAssets() const = 0;

  bool IsOutputUpToDate(
    const char*                           szDocumentPath,
    const xiiDynamicArray<xiiString>&     outputs,
    xiiUInt64                             uiHash,
    const xiiAssetDocumentTypeDescriptor* pTypeDescriptor);
  virtual bool IsOutputUpToDate(
    const char*                           szDocumentPath,
    const char*                           szOutputTag,
    xiiUInt64                             uiHash,
    const xiiAssetDocumentTypeDescriptor* pTypeDescriptor);

  ///@}


  /// \brief Called by the editor to try to open a document for the matching picking result
  virtual xiiResult OpenPickedDocument(const xiiDocumentObject* pPickedComponent, xiiUInt32 uiPartIndex) { return XII_FAILURE; }

  xiiResult TryOpenAssetDocument(const char* szPathOrGuid);

protected:
  static bool IsResourceUpToDate(const char* szResourceFile, xiiUInt64 uiHash, xiiUInt16 uiTypeVersion);
  static void GenerateOutputFilename(
    xiiStringBuilder&         inout_sRelativeDocumentPath,
    const xiiPlatformProfile* pAssetProfile,
    const char*               szExtension,
    bool                      bPlatformSpecific);
};
