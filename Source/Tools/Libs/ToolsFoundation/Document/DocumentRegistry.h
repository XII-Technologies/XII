/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <ToolsFoundation/Document/Document.h>

struct XII_TOOLSFOUNDATION_DLL xiiActiveDocumentChange
{
  const xiiDocument* m_pOldDocument;
  const xiiDocument* m_pNewDocument;
};

/// Tracks existing and active xiiDocument.
///
/// While the IDocumentManager manages documents of a certain context,
/// this class simply keeps track of the overall number of documents and the currently active one.
class XII_TOOLSFOUNDATION_DLL xiiDocumentRegistry
{
public:
  static bool RegisterDocument(const xiiDocument* pDocument);
  static bool UnregisterDocument(const xiiDocument* pDocument);

  static xiiArrayPtr<const xiiDocument*> GetDocuments() { return s_Documents; }

  static void               SetActiveDocument(const xiiDocument* pDocument);
  static const xiiDocument* GetActiveDocument();

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, DocumentRegistry);

  static void Startup();
  static void Shutdown();

public:
  // static xiiEvent<xiiDocumentChange&> m_DocumentAddedEvent;
  // static xiiEvent<xiiDocumentChange&> m_DocumentRemovedEvent;
  static xiiEvent<xiiActiveDocumentChange&> m_ActiveDocumentChanged;

private:
  static xiiHybridArray<const xiiDocument*, 16> s_Documents;
  static xiiDocument*                           s_pActiveDocument;
};
