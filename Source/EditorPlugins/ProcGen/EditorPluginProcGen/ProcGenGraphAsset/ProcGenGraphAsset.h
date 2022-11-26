#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>

class xiiPin;

class xiiProcGenGraphAssetDocument : public xiiAssetDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGenGraphAssetDocument, xiiAssetDocument);

public:
  xiiProcGenGraphAssetDocument(const char* szDocumentPath);

  void SetDebugPin(const xiiPin* pDebugPin);

  xiiStatus WriteAsset(xiiStreamWriter& stream, const xiiPlatformProfile* pAssetProfile, bool bAllowDebug) const;

protected:
  virtual void               UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const override;
  virtual bool Paste(
    const xiiArrayPtr<PasteInfo>& info,
    const xiiAbstractObjectGraph& objectGraph,
    bool                          bAllowPickedPosition,
    const char*                   szMimeType) override;

  virtual void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable) override;

  void GetAllOutputNodes(xiiDynamicArray<const xiiDocumentObject*>& placementNodes, xiiDynamicArray<const xiiDocumentObject*>& vertexColorNodes) const;

private:
  friend class xiiProcGenAction;

  virtual void InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const override;

  struct GenerateContext;

  xiiExpressionAST::Node* GenerateExpressionAST(const xiiDocumentObject* outputNode, const char* szOutputName, GenerateContext& context, xiiExpressionAST& out_Ast) const;
  xiiExpressionAST::Node* GenerateDebugExpressionAST(GenerateContext& context, xiiExpressionAST& out_Ast) const;

  void DumpSelectedOutput(bool bAst, bool bDisassembly) const;

  void CreateDebugNode();

  const xiiPin*                            m_pDebugPin = nullptr;
  xiiUniquePtr<xiiProcGen_PlacementOutput> m_pDebugNode;
};
