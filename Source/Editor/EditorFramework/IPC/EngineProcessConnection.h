/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

class xiiEditorEngineConnection;
class xiiDocument;
class xiiDocumentObject;
struct xiiDocumentObjectPropertyEvent;
struct xiiDocumentObjectStructureEvent;
class xiiQtEngineDocumentWindow;
class xiiAssetDocument;

class XII_EDITORFRAMEWORK_DLL xiiEditorEngineProcessConnection
{
  XII_DECLARE_SINGLETON(xiiEditorEngineProcessConnection);

public:
  xiiEditorEngineProcessConnection();
  ~xiiEditorEngineProcessConnection();

  /// The given file system configuration will be used by the engine process to setup the runtime data directories.
  ///        This only takes effect if the editor process is restarted.
  void SetFileSystemConfig(const xiiApplicationFileSystemConfig& cfg) { m_FileSystemConfig = cfg; }

  /// The given plugin configuration will be used by the engine process to load runtime plugins.
  ///        This only takes effect if the editor process is restarted.
  void SetPluginConfig(const xiiApplicationPluginConfig& cfg) { m_PluginConfig = cfg; }

  void      Update();
  xiiResult RestartProcess();
  void      ShutdownProcess();
  bool      IsProcessCrashed() const { return m_bProcessCrashed; }

  xiiEditorEngineConnection* CreateEngineConnection(xiiAssetDocument* pDocument);
  void                       DestroyEngineConnection(xiiAssetDocument* pDocument);

  bool SendMessage(xiiProcessMessage* pMessage);

  /// Waits for a message of type pMessageType. If tTimeout is zero, the function will not timeout. If the timeout is valid
  ///        and is it, XII_FAILURE is returned. If the message type matches and pCallback is valid, the function will be called
  ///        and the return values decides whether the message is to be accepted and the waiting has ended.
  xiiResult WaitForMessage(const xiiRTTI* pMessageType, xiiTime timeout, xiiProcessCommunicationChannel ::WaitForMessageCallback* pCallback = nullptr);
  /// Same as WaitForMessage but the message must be to a specific document. Therefore,
  ///        pMessageType must be derived from xiiEditorEngineDocumentMsg and the function will only return if the received
  ///        message matches both type, document and is accepted by pCallback.
  xiiResult WaitForDocumentMessage(const xiiUuid& assetGuid, const xiiRTTI* pMessageType, xiiTime timeout, xiiProcessCommunicationChannel::WaitForMessageCallback* pCallback = nullptr);

  bool IsEngineSetup() const { return m_bClientIsConfigured; }

  void ActivateRemoteProcess(const xiiAssetDocument* pDocument, xiiUInt32 uiViewID);

  xiiProcessCommunicationChannel& GetCommunicationChannel() { return m_IPC; }

  struct Event
  {
    enum class Type
    {
      Invalid,
      ProcessStarted,
      ProcessCrashed,
      ProcessShutdown,
      ProcessMessage,
      ProcessRestarted,
    };

    Event()
    {
      m_Type = Type::Invalid;
      m_pMsg = nullptr;
    }

    Type                     m_Type;
    const xiiProcessMessage* m_pMsg;
  };

  static xiiEvent<const Event&> s_Events;

private:
  void Initialize(const xiiRTTI* pFirstAllowedMessageType);
  void HandleIPCEvent(const xiiProcessCommunicationChannel::Event& e);
  void UIServicesTickEventHandler(const xiiQtUiServices::TickEvent& e);
  bool ConnectToRemoteProcess();
  void ShutdownRemoteProcess();

  bool                   m_bProcessShouldBeRunning;
  bool                   m_bProcessCrashed;
  bool                   m_bClientIsConfigured;
  xiiEventSubscriptionID m_TickEventSubscriptionID = 0;
  xiiUInt32              m_uiRedrawCountSent       = 0;
  xiiUInt32              m_uiRedrawCountReceived   = 0;

  xiiEditorProcessCommunicationChannel                     m_IPC;
  xiiUniquePtr<xiiEditorProcessRemoteCommunicationChannel> m_pRemoteProcess;
  xiiApplicationFileSystemConfig                           m_FileSystemConfig;
  xiiApplicationPluginConfig                               m_PluginConfig;
  xiiHashTable<xiiUuid, xiiAssetDocument*>                 m_DocumentByGuid;
};

class XII_EDITORFRAMEWORK_DLL xiiEditorEngineConnection
{
public:
  bool SendMessage(xiiEditorEngineDocumentMsg* pMessage);
  void SendHighlightObjectMessage(xiiViewHighlightMsgToEngine* pMessage);

  xiiDocument* GetDocument() const { return m_pDocument; }

private:
  friend class xiiEditorEngineProcessConnection;
  xiiEditorEngineConnection(xiiDocument* pDocument) { m_pDocument = pDocument; }
  ~xiiEditorEngineConnection() = default;

  xiiDocument* m_pDocument;
};
