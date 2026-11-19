/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginMCP/EditorPluginMCPDLL.h>

#include <MCPPlugin/Bridge.h>
#include <MCPPlugin/HttpServer.h>
#include <ToolsFoundation/Document/DocumentManager.h>

#include <QTimer>

class xiiEditorMCP final
{
public:
  xiiEditorMCP();
  ~xiiEditorMCP();
  [[nodiscard]] xiiStatus Start();

private:
  void DocumentEvent(const xiiDocumentManager::Event& event);
  xiiUuid GetDocumentSession(const xiiDocument& document);
  xiiDocument* FindDocument(const xiiVariantDictionary& arguments);
  xiiStatus ValidateEdit(const xiiVariantDictionary& arguments, xiiDocument*& out_pDocument);

  xiiStatus Project(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result);
  xiiStatus Documents(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result);
  xiiStatus Scene(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus EditScene(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus Save(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus Assets(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus Asset(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus Transform(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);

  xiiMCPRegistry m_Registry;
  xiiMCPProtocol m_Protocol;
  xiiMCPBridge m_Bridge;
  xiiMCPHttpServer m_Http;
  QTimer m_Timer;
  xiiMap<xiiUuid, xiiUuid> m_DocumentSessions;
  xiiUuid m_AllowedProject;
  xiiEventSubscriptionID m_DocumentEvents = 0;
  bool m_bUpdating = false;
};
