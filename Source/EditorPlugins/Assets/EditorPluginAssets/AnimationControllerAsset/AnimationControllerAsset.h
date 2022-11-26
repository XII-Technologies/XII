#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class xiiAnimGraph;
class xiiAnimGraphNode;

class xiiAnimationControllerNodePin : public xiiPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationControllerNodePin, xiiPin);

public:
  xiiAnimationControllerNodePin(Type type, const char* szName, const xiiColorGammaUB& color, const xiiDocumentObject* pObject);
  ~xiiAnimationControllerNodePin();

  bool                  m_bMultiInputPin = false;
  xiiAnimGraphPin::Type m_DataType       = xiiAnimGraphPin::Invalid;
};

class xiiAnimationControllerNodeManager : public xiiDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const xiiDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& node) override;
  virtual void GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const override;

  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const override;
};

class xiiAnimationControllerAssetDocument : public xiiAssetDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationControllerAssetDocument, xiiAssetDocument);

public:
  xiiAnimationControllerAssetDocument(const char* szDocumentPath);

protected:
  struct PinCount
  {
    xiiUInt16 m_uiInputCount  = 0;
    xiiUInt16 m_uiInputIdx    = 0;
    xiiUInt16 m_uiOutputCount = 0;
    xiiUInt16 m_uiOutputIdx   = 0;
  };

  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  void SortNodesByPriority(xiiDynamicArray<const xiiDocumentObject*>& allNodes);

  void CountPinTypes(const xiiDocumentNodeManager* pNodeManager, xiiDynamicArray<const xiiDocumentObject*>& allNodes, xiiMap<xiiUInt8, PinCount>& pinCounts) const;
  void CreateOutputGraphNodes(const xiiDynamicArray<const xiiDocumentObject*>& allNodes, xiiAnimGraph& animController, xiiDynamicArray<xiiAnimGraphNode*>& newNodes) const;
  void SetInputPinIndices(const xiiDynamicArray<xiiAnimGraphNode*>& newNodes, const xiiDynamicArray<const xiiDocumentObject*>& allNodes, const xiiDocumentNodeManager* pNodeManager, xiiMap<xiiUInt8, PinCount>& pinCounts, xiiMap<const xiiPin*, xiiUInt16>& inputPinIndices, xiiAbstractMemberProperty* pIdxProperty, xiiAbstractMemberProperty* pNumProperty) const;
  void SetOutputPinIndices(const xiiDynamicArray<xiiAnimGraphNode*>& newNodes, const xiiDynamicArray<const xiiDocumentObject*>& allNodes, const xiiDocumentNodeManager* pNodeManager, xiiMap<xiiUInt8, PinCount>& pinCounts, xiiAnimGraph& animController, xiiAbstractMemberProperty* pIdxProperty, const xiiMap<const xiiPin*, xiiUInt16>& inputPinIndices) const;

  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const override;
  virtual bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;

  virtual void InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable) override;
};
