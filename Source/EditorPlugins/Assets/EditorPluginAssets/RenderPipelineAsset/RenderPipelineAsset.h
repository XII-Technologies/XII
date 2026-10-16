/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class XII_EDITORPLUGINASSETS_DLL xiiRenderPipelineNodeManager : public xiiDocumentNodeManager
{
public:
  void         GetNodeCreationTemplates(xiiDynamicArray<xiiNodeCreationTemplate>& out_templates) const override;
  virtual bool InternalIsNode(const xiiDocumentObject* pObject) const override;
  bool         InternalIsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const override;
  virtual void InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const override;

  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const override;
  bool              InternalCanConnectToSelf(const xiiPin& source, const xiiPin& target) const override;

private:
  mutable xiiDynamicArray<xiiNodePropertyValue> m_BuiltinProperties;
};

class XII_EDITORPLUGINASSETS_DLL xiiRenderPipelineAssetDocument : public xiiAssetDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineAssetDocument, xiiAssetDocument);

public:
  /// Offline authoring/asset automation can omit IPC; interactive documents use full mirroring.
  xiiRenderPipelineAssetDocument(xiiStringView sDocumentPath, xiiAssetDocEngineConnection connection = xiiAssetDocEngineConnection::FullObjectMirroring);
  ~xiiRenderPipelineAssetDocument();
  /// Replace one reference node with configured leaf nodes and remapped boundary connections.
  /// Uses one undoable transaction and leaves the document unchanged on failure.
  xiiStatus ExpandSubgraph(const xiiDocumentObject* object);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStringView sTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const override;
  virtual bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable) override;
};
