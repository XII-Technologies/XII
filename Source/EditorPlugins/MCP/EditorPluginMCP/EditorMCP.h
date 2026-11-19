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
  void                    UpdateProjectPermissions();
  [[nodiscard]] xiiResult RegisterProject();
  [[nodiscard]] xiiResult RegisterDocuments();
  [[nodiscard]] xiiResult RegisterScenes();
  [[nodiscard]] xiiResult RegisterAssets();
  [[nodiscard]] xiiResult RegisterGraphs();
  static bool             IsScene(const xiiDocument& document);
  static void             DescribeDocument(const xiiDocument& document, const xiiUuid& session, xiiVariantDictionary& out_result);

  void         DocumentEvent(const xiiDocumentManager::Event& event);
  xiiUuid      GetDocumentSession(const xiiDocument& document);
  xiiDocument* FindDocument(const xiiVariantDictionary& arguments);
  xiiStatus    ValidateEdit(const xiiVariantDictionary& arguments, xiiDocument*& out_pDocument);

  xiiStatus Project(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result);
  xiiStatus CppProject(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result);
  xiiStatus Documents(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result);
  xiiStatus Document(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus InspectDocumentObject(xiiDocument& ref_document, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus SetDocumentProperty(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus DocumentHistory(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus Scene(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus EditScene(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus Save(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus Assets(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus Asset(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus OpenAsset(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus Transform(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus Graph(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus EditGraph(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);

  xiiMCPRegistry           m_Registry;
  xiiMCPProtocol           m_Protocol;
  xiiMCPBridge             m_Bridge;
  xiiMCPHttpServer         m_Http;
  QTimer                   m_Timer;
  xiiMap<xiiUuid, xiiUuid> m_DocumentSessions;
  xiiUuid                  m_AllowedProject;
  xiiUuid                  m_ArgumentProject;
  xiiTime                  m_NextPermissionUpdate;
  xiiUInt8                 m_uiArgumentPermissions = static_cast<xiiUInt8>(xiiMCPPermission::Read);
  xiiEventSubscriptionID   m_DocumentEvents        = 0;
  bool                     m_bUpdating             = false;
};
