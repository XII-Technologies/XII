/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class xiiVisualScriptClassAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptClassAssetProperties, xiiReflectedClass);

public:
  xiiString                                m_sBaseClass;
  xiiDynamicArray<xiiVisualScriptVariable> m_Variables;
  bool                                     m_bDumpAST;
};

class xiiVisualScriptClassAssetDocument : public xiiSimpleAssetDocument<xiiVisualScriptClassAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptClassAssetDocument, xiiSimpleAssetDocument<xiiVisualScriptClassAssetProperties>);

public:
  xiiVisualScriptClassAssetDocument(xiiStringView sDocumentPath);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual void               UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;

  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const override;
  virtual bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable) override;
};
