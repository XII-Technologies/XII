#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>

class xiiMaterialAssetDocument;
struct xiiPropertyMetaStateEvent;
struct xiiEditorAppEvent;

struct xiiMaterialShaderMode
{
  typedef xiiUInt8 StorageType;

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
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Ball,
    Sphere,
    Box,
    Plane,

    Default = Ball
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiMaterialAssetPreview);

class xiiMaterialAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialAssetProperties, xiiReflectedClass);

public:
  xiiMaterialAssetProperties() :
    m_pDocument(nullptr)
  {
  }

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

  xiiMap<xiiString, xiiVariant>  m_CachedProperties;
  xiiMaterialAssetDocument*      m_pDocument;
  xiiEnum<xiiMaterialShaderMode> m_ShaderMode;
};

class xiiMaterialAssetDocument : public xiiSimpleAssetDocument<xiiMaterialAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialAssetDocument, xiiSimpleAssetDocument<xiiMaterialAssetProperties>);

public:
  xiiMaterialAssetDocument(const char* szDocumentPath);
  ~xiiMaterialAssetDocument();

  xiiDocumentObject*       GetShaderPropertyObject();
  const xiiDocumentObject* GetShaderPropertyObject() const;

  void SetBaseMaterial(const char* szBaseMaterial);

  xiiStatus WriteMaterialAsset(xiiStreamWriter& stream, const xiiPlatformProfile* pAssetProfile, bool bEmbedLowResData) const;

  /// \brief Will make sure that the visual shader is rebuilt.
  /// Typically called during asset transformation, but can be triggered manually to enforce getting visual shader node changes in.
  xiiStatus RecreateVisualShaderFile(const xiiAssetFileHeader& AssetHeader);

  /// \brief If shader compilation failed this will modify the output shader file such that transforming it again, will trigger a full
  /// regeneration Otherwise the AssetCurator would early out
  void TagVisualShaderFileInvalid(const xiiPlatformProfile* pAssetProfile, const char* szError);

  /// \brief Deletes all Visual Shader nodes that are not connected to the output
  void RemoveDisconnectedNodes();

  static xiiUuid GetLitBaseMaterial();
  static xiiUuid GetLitAlphaTestBaseMaterial();
  static xiiUuid GetNeutralNormalMap();

  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const override;
  virtual bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;

  xiiEvent<const xiiMaterialVisualShaderEvent&> m_VisualShaderEvents;
  xiiEnum<xiiMaterialAssetPreview>              m_PreviewModel;

protected:
  xiiUuid        GetSeedFromBaseMaterial(const xiiAbstractObjectGraph* pBaseGraph);
  static xiiUuid GetMaterialNodeGuid(const xiiAbstractObjectGraph& graph);
  virtual void   UpdatePrefabObject(xiiDocumentObject* pObject, const xiiUuid& PrefabAsset, const xiiUuid& PrefabSeed, const char* szBasePrefab) override;
  virtual void   InitializeAfterLoading(bool bFirstTimeCreation) override;

  virtual xiiTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
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
