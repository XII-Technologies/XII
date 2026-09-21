/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>

class xiiMaterialAssetDocument;
struct xiiPropertyMetaStateEvent;
struct xiiEditorAppEvent;

struct xiiMaterialShaderMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    BaseMaterial,
    File,
    Custom,

    Default = BaseMaterial
  };
};

struct xiiMaterialVisualShaderEvent
{
  enum Type
  {
    TransformFailed,
    TransformSucceeded,
    VisualShaderNotUsed,
  };

  Type      m_Type;
  xiiString m_sTransformError;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiMaterialShaderMode);

struct xiiMaterialAssetPreview
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Sphere = 0,
    Box,
    Plane,

    Default = Sphere
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiMaterialAssetPreview);

class xiiMaterialAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialAssetProperties, xiiReflectedClass);

public:
  xiiMaterialAssetProperties()

    = default;

  void        SetBaseMaterial(const char* szBaseMaterial);
  const char* GetBaseMaterial() const;

  void        SetSurface(const char* szSurface) { m_sSurface = szSurface; }
  const char* GetSurface() const { return m_sSurface; }

  void                           SetShader(const char* szShader);
  const char*                    GetShader() const;
  void                           SetShaderProperties(xiiReflectedClass* pProperties);
  xiiReflectedClass*             GetShaderProperties() const;
  void                           SetShaderMode(xiiEnum<xiiMaterialShaderMode> mode);
  xiiEnum<xiiMaterialShaderMode> GetShaderMode() const { return m_ShaderMode; }

  void SetDocument(xiiMaterialAssetDocument* pDocument);
  void UpdateShader(bool bForce = false);

  void DeleteProperties();
  void CreateProperties(const char* szShaderPath);

  void SaveOldValues();
  void LoadOldValues();

  xiiString ResolveRelativeShaderPath() const;
  xiiString GetAutoGenShaderPathAbs() const;

  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

public:
  xiiString m_sBaseMaterial;
  xiiString m_sSurface;
  xiiString m_sShader;
  xiiString m_sAssetFilterTags;

  xiiMap<xiiString, xiiVariant>  m_CachedProperties;
  xiiMaterialAssetDocument*      m_pDocument = nullptr;
  xiiEnum<xiiMaterialShaderMode> m_ShaderMode;
};

class xiiMaterialAssetDocument : public xiiSimpleAssetDocument<xiiMaterialAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialAssetDocument, xiiSimpleAssetDocument<xiiMaterialAssetProperties>);

public:
  xiiMaterialAssetDocument(xiiStringView sDocumentPath);
  ~xiiMaterialAssetDocument();

  xiiDocumentObject*       GetShaderPropertyObject();
  const xiiDocumentObject* GetShaderPropertyObject() const;

  void SetBaseMaterial(const char* szBaseMaterial);

  xiiStatus WriteMaterialAsset(xiiStreamWriter& inout_stream, const xiiPlatformProfile* pAssetProfile, bool bEmbedLowResData) const;

  /// Will make sure that the visual shader is rebuilt.
  /// Typically called during asset transformation, but can be triggered manually to enforce getting visual shader node changes in.
  xiiStatus RecreateVisualShaderFile(const xiiAssetFileHeader& assetHeader);

  /// If shader compilation failed this will modify the output shader file such that transforming it again, will trigger a full
  /// regeneration Otherwise the AssetCurator would early out
  void TagVisualShaderFileInvalid(const xiiPlatformProfile* pAssetProfile, const char* szError);

  /// Deletes all Visual Shader nodes that are not connected to the output
  void RemoveDisconnectedNodes();

  static xiiUuid GetLitBaseMaterial();
  static xiiUuid GetLitAlphaTestBaseMaterial();
  static xiiUuid GetNeutralNormalMap();

  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_sMimeType) const override;
  virtual bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType) override;

  xiiEvent<const xiiMaterialVisualShaderEvent&> m_VisualShaderEvents;
  xiiEnum<xiiMaterialAssetPreview>              m_PreviewModel;

protected:
  xiiUuid        GetSeedFromBaseMaterial(const xiiAbstractObjectGraph* pBaseGraph);
  static xiiUuid GetMaterialNodeGuid(const xiiAbstractObjectGraph& graph);
  virtual void   UpdatePrefabObject(xiiDocumentObject* pObject, const xiiUuid& PrefabAsset, const xiiUuid& PrefabSeed, xiiStringView sBasePrefab) override;
  virtual void   InitializeAfterLoading(bool bFirstTimeCreation) override;

  virtual xiiTransformStatus InternalTransformAsset(xiiStringView sTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  virtual void InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable) override;

  virtual void UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;

  void InvalidateCachedShader();
  void EditorEventHandler(const xiiEditorAppEvent& e);

private:
  xiiStringBuilder m_sCheckPermutations;
  static xiiUuid   s_LitBaseMaterial;
  static xiiUuid   s_LitAlphaTextBaseMaterial;
  static xiiUuid   s_NeutralNormalMap;
};

class xiiMaterialObjectManager : public xiiVisualShaderNodeManager
{
};
