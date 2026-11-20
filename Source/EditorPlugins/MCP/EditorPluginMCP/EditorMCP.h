/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginMCP/EditorPluginMCPDLL.h>

#include <MCPPlugin/Bridge.h>
#include <MCPPlugin/HttpServer.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#include <QProcess>
#include <QTimer>

class xiiAssetProcessor;
class xiiViewportCaptureMsgToEditor;

class xiiEditorMCP final
{
public:
  xiiEditorMCP();
  ~xiiEditorMCP();
  [[nodiscard]] xiiStatus Start();

private:
  void                    UpdateProjectPermissions();
  [[nodiscard]] xiiResult RegisterCppBuild();
  [[nodiscard]] xiiResult RegisterProject();
  [[nodiscard]] xiiResult RegisterDocuments();
  [[nodiscard]] xiiResult RegisterScenes();
  [[nodiscard]] xiiResult RegisterAssets();
  [[nodiscard]] xiiResult RegisterGraphs();
  [[nodiscard]] xiiResult RegisterViewport();
  static bool             IsScene(const xiiDocument& document);
  static void             DescribeDocument(const xiiDocument& document, const xiiUuid& session, xiiVariantDictionary& out_result);

  void         DocumentEvent(const xiiDocumentManager::Event& event);
  xiiUuid      GetDocumentSession(const xiiDocument& document);
  xiiDocument* FindDocument(const xiiVariantDictionary& arguments);
  xiiStatus    ValidateEdit(const xiiVariantDictionary& arguments, xiiDocument*& out_pDocument);

  xiiStatus                        Project(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result);
  xiiStatus                        CppProject(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result);
  xiiStatus                        StartCppBuild(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        GetCppBuild(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        Documents(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result);
  xiiStatus                        Document(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        CreateDocument(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        CloseDocument(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        InspectDocumentObject(xiiDocument& ref_document, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        SetDocumentProperty(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        DocumentHistory(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        Scene(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        EditScene(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        Save(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        Assets(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        Asset(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        OpenAsset(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        Transform(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        TransformAssets(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        AssetProcessing(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result);
  xiiStatus                        ControlAssetProcessing(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  [[nodiscard]] xiiAssetProcessor* GetAuthorizedAssetProcessor(const xiiVariantDictionary& arguments) const;
  xiiStatus                        Graph(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        EditGraph(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        Viewport(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        ConfigureViewport(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        CaptureViewport(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        GetViewportCapture(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);
  xiiStatus                        ReadViewportCapture(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);

  struct ViewportCapture
  {
    xiiTime   m_Started;
    xiiUInt32 m_uiView   = 0;
    xiiUInt32 m_uiWidth  = 0;
    xiiUInt32 m_uiHeight = 0;
    xiiString m_sBlob;
    xiiString m_sFailure;

    [[nodiscard]] bool IsPending() const { return m_sBlob.IsEmpty() && m_sFailure.IsEmpty(); }
  };

  [[nodiscard]] bool                   IsViewportCaptureAuthorized() const;
  [[nodiscard]] const ViewportCapture* FindViewportCapture(xiiStringView sKey) const;
  void                                 UpdateViewportCaptures();
  void                                 OnViewportCapture(const xiiViewportCaptureMsgToEditor& message);

  void               ConnectCppBuildProcess();
  void               DrainCppBuild();
  void               ReadCppBuildOutput();
  void               FinishCppBuild(const xiiStatus& status);
  void               DescribeCppBuild(xiiUInt64 uiOutputOffset, xiiVariantDictionary& out_result) const;
  void               OnProjectRequest(xiiToolsProjectRequest& ref_request);
  void               OnProjectEvent(const xiiToolsProjectEvent& event);
  [[nodiscard]] bool IsCppBuildAuthorized() const;

  struct CppBuild
  {
    enum class State : xiiUInt8
    {
      Idle,
      Starting,
      Building,
      Succeeded,
      Failed
    };

    [[nodiscard]] bool IsActive() const { return m_State == State::Starting || m_State == State::Building; }

    xiiUuid    m_Id;
    xiiUInt64  m_uiProjectGeneration = 0;
    State      m_State               = State::Idle;
    xiiTime    m_Started;
    xiiTime    m_Finished;
    xiiString  m_sPluginName;
    xiiString  m_sOutputDirectory;
    QByteArray m_Output;
    xiiUInt64  m_uiOutputBytes = 0;
    xiiString  m_sMessage;
    xiiInt32   m_iExitCode    = 0;
    bool       m_bHasExitCode = false;
  };

  xiiMCPRegistry                   m_Registry;
  xiiMCPProtocol                   m_Protocol;
  xiiMCPBridge                     m_Bridge;
  xiiMCPHttpServer                 m_Http;
  QTimer                           m_Timer;
  QProcess                         m_CppBuildProcess;
  CppBuild                         m_CppBuild;
  xiiMap<xiiUuid, ViewportCapture> m_ViewportCaptures;
  xiiMap<xiiUuid, xiiUuid>         m_DocumentSessions;
  xiiUuid                          m_AllowedProject;
  xiiUuid                          m_ArgumentProject;
  xiiTime                          m_NextPermissionUpdate;
  xiiUInt8                         m_uiArgumentPermissions = static_cast<xiiUInt8>(xiiMCPPermission::Read);
  xiiEventSubscriptionID           m_DocumentEvents        = 0;
  xiiEventSubscriptionID           m_ProjectRequests       = 0;
  xiiEventSubscriptionID           m_ProjectEvents         = 0;
  xiiEventSubscriptionID           m_EngineEvents          = 0;
  bool                             m_bAllowAssetProcessing = false;
  bool                             m_bAllowCppBuilds       = false;
  bool                             m_bAllowViewportCapture = false;
  bool                             m_bUpdating             = false;
};
