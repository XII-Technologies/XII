#pragma once

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/EditorFrameworkDLL.h>

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
  xiiGameObjectContextDocument(const char* szDocumentPath, xiiDocumentObjectManager* pObjectManager, xiiAssetDocEngineConnection engineConnectionType = xiiAssetDocEngineConnection::FullObjectMirroring);
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
