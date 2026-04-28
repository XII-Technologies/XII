/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphPins.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

using xiiAnimationClipResourceHandle = xiiTypedResourceHandle<class xiiAnimationClipResource>;

class xiiAnimGraphInstance;
class xiiAnimGraphNode;

class xiiAnimationGraphNodePin : public xiiPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationGraphNodePin, xiiPin);

public:
  xiiAnimationGraphNodePin(Type type, const char* szName, const xiiColorGammaUB& color, const xiiDocumentObject* pObject);
  ~xiiAnimationGraphNodePin();

  bool                  m_bMultiInputPin = false;
  xiiAnimGraphPin::Type m_DataType       = xiiAnimGraphPin::Invalid;
};

class xiiAnimationGraphNodeManager : public xiiDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const xiiDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const override;

  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const override;

private:
  virtual bool InternalIsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const override;
};

class xiiAnimationGraphAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationGraphAssetProperties, xiiReflectedClass);

public:
  xiiDynamicArray<xiiString>               m_IncludeGraphs;
  xiiDynamicArray<xiiAnimationClipMapping> m_AnimationClipMapping;
};

class xiiAnimationGraphAssetDocument : public xiiSimpleAssetDocument<xiiAnimationGraphAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationGraphAssetDocument, xiiSimpleAssetDocument<xiiAnimationGraphAssetProperties>);

public:
  xiiAnimationGraphAssetDocument(xiiStringView sDocumentPath);

protected:
  struct PinCount
  {
    xiiUInt16 m_uiInputCount  = 0;
    xiiUInt16 m_uiInputIdx    = 0;
    xiiUInt16 m_uiOutputCount = 0;
    xiiUInt16 m_uiOutputIdx   = 0;
  };

  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const override;
  virtual bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable) override;
};
