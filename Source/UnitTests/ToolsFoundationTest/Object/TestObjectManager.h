#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>


class xiiTestDocumentObjectManager : public xiiDocumentObjectManager
{
public:
  xiiTestDocumentObjectManager();
  ~xiiTestDocumentObjectManager();
};


class xiiTestDocument : public xiiDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestDocument, xiiDocument);

public:
  xiiTestDocument(const char* szDocumentPath, bool bUseIPCObjectMirror = false);
  ~xiiTestDocument();

  virtual void             InitializeAfterLoading(bool bFirstTimeCreation) override;
  void                     ApplyNativePropertyChangesToObjectManager(xiiDocumentObject* pObject);
  virtual xiiDocumentInfo* CreateDocumentInfo() override;

  xiiDocumentObjectMirror m_ObjectMirror;
  xiiRttiConverterContext m_Context;



private:
  bool m_bUseIPCObjectMirror;
};
