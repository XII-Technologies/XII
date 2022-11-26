#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <ToolsFoundation/Document/Document.h>

class XII_EDITORFRAMEWORK_DLL xiiAssetDocumentInfo final : public xiiDocumentInfo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAssetDocumentInfo, xiiDocumentInfo);

public:
  xiiAssetDocumentInfo();
  virtual ~xiiAssetDocumentInfo();
  xiiAssetDocumentInfo(xiiAssetDocumentInfo&& rhs);
  void operator=(xiiAssetDocumentInfo&& rhs);
  /// \brief Creates a clone without meta data.
  void CreateShallowClone(xiiAssetDocumentInfo& out_docInfo) const;
  void ClearMetaData();

  xiiUInt64 m_uiSettingsHash; ///< Current hash over all settings in the document, used to check resulting resource for being up-to-date in combination
                              ///< with dependency hashes.
  xiiSet<xiiString>
    m_AssetTransformDependencies;          ///< Files that are required to generate the asset, ie. if one changes, the asset needs to be recreated
  xiiSet<xiiString> m_RuntimeDependencies; ///< Other files that are used at runtime together with this asset, e.g. materials for a mesh, needed for
                                           ///< thumbnails and packaging.
  xiiSet<xiiString> m_Outputs;             ///< Additional output this asset produces besides the default one. These are tags like VISUAL_SHADER that are resolved
                                           ///< by the xiiAssetDocumentManager into paths.
  xiiHashedString m_sAssetsDocumentTypeName;
  xiiDynamicArray<xiiReflectedClass*>
    m_MetaInfo; ///< Holds arbitrary objects that store meta-data for the asset document. Mainly used for exposed parameters, but can be any reflected
                ///< type. This array takes ownership of all objects and deallocates them on shutdown.

  const char* GetAssetsDocumentTypeName() const;
  void        SetAssetsDocumentTypeName(const char* sz);

  /// \brief Returns an object from m_MetaInfo of the given base type, or nullptr if none exists
  const xiiReflectedClass* GetMetaInfo(const xiiRTTI* pType) const;

  /// \brief Returns an object from m_MetaInfo of the given base type, or nullptr if none exists
  template <typename T>
  const T* GetMetaInfo()
  {
    return static_cast<const T*>(GetMetaInfo(xiiGetStaticRTTI<T>()));
  }

private:
  xiiAssetDocumentInfo(const xiiAssetDocumentInfo&);
  void operator=(const xiiAssetDocumentInfo&) = delete;
};
