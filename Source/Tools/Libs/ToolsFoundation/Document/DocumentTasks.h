#pragma once

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>

class xiiSaveDocumentTask final : public xiiTask
{
public:
  xiiSaveDocumentTask();
  ~xiiSaveDocumentTask();

  xiiDeferredFileWriter  file;
  xiiAbstractObjectGraph headerGraph;
  xiiAbstractObjectGraph objectGraph;
  xiiAbstractObjectGraph typesGraph;
  xiiDocument*           m_document = nullptr;

  virtual void Execute() override;
};

class xiiAfterSaveDocumentTask final : public xiiTask
{
public:
  xiiAfterSaveDocumentTask();
  ~xiiAfterSaveDocumentTask();

  xiiDocument*                   m_document = nullptr;
  xiiDocument::AfterSaveCallback m_callback;

  virtual void Execute() override;
};
