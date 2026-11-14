/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphNode.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphNodeManager.h>

class xiiAnimationGraphAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationGraphAssetProperties, xiiReflectedClass);

public:
  xiiDynamicArray<xiiAnimationGraphEditorTransition> m_Transitions;
};

class xiiAnimationGraphAssetDocument : public xiiSimpleAssetDocument<xiiAnimationGraphAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationGraphAssetDocument, xiiSimpleAssetDocument<xiiAnimationGraphAssetProperties>);

public:
  xiiAnimationGraphAssetDocument(xiiStringView sDocumentPath);

protected:
  xiiStatus InternalLoadDocument() override;

  xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const override;
  bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const override;
  bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType) override;

  void InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const override;
  void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const override;
  void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable) override;
};
