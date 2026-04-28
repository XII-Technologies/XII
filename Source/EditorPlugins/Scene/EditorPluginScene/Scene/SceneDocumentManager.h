/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class xiiSceneDocumentManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneDocumentManager, xiiAssetDocumentManager);

public:
  xiiSceneDocumentManager();

private:
  virtual void InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;
  virtual void InternalCloneDocument(xiiStringView sPath, xiiStringView sClonePath, const xiiUuid& documentId, const xiiUuid& seedGuid, const xiiUuid& cloneGuid, xiiAbstractObjectGraph* pHeader, xiiAbstractObjectGraph* pObjects, xiiAbstractObjectGraph* pTypes) override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  void SetupDefaultScene(xiiDocument* pDocument);


  xiiStaticArray<xiiAssetDocumentTypeDescriptor, 4> m_DocTypeDescs;
};
