/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Document/GameObjectDocument.h>

struct XII_EDITORFRAMEWORK_DLL xiiGameObjectContextEvent
{
  enum class Type
  {
    ContextAboutToBeChanged,
    ContextChanged,
  };
  Type m_Type;
};

class XII_EDITORFRAMEWORK_DLL xiiGameObjectContextDocument : public xiiGameObjectDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectContextDocument, xiiGameObjectDocument);

public:
  xiiGameObjectContextDocument(xiiStringView sDocumentPath, xiiDocumentObjectManager* pObjectManager, xiiAssetDocEngineConnection engineConnectionType = xiiAssetDocEngineConnection::FullObjectMirroring);
  ~xiiGameObjectContextDocument();

  xiiStatus                SetContext(xiiUuid documentGuid, xiiUuid objectGuid);
  xiiUuid                  GetContextDocumentGuid() const;
  xiiUuid                  GetContextObjectGuid() const;
  const xiiDocumentObject* GetContextObject() const;

  mutable xiiEvent<const xiiGameObjectContextEvent&> m_GameObjectContextEvents;

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

private:
  void ClearContext();

private:
  xiiUuid m_ContextDocument;
  xiiUuid m_ContextObject;
};
