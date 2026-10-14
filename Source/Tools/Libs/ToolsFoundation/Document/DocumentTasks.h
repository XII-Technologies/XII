/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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
  xiiString              m_sDocumentPath;
  xiiStatus              m_Result = xiiStatus("Document save did not complete.");

  virtual void Execute() override;
};

class xiiAfterSaveDocumentTask final : public xiiTask
{
public:
  xiiAfterSaveDocumentTask();
  ~xiiAfterSaveDocumentTask();

  xiiDocument*                      m_document = nullptr;
  xiiDocument::AfterSaveCallback    m_callback;
  xiiSharedPtr<xiiSaveDocumentTask> m_pSaveTask;
  xiiTaskGroupID                    m_TaskGroup;
  xiiUInt64                         m_uiModificationRevision = 0;

  virtual void Execute() override;
};
