/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Threading/Mutex.h>
#  include <Foundation/Time/Time.h>
#  include <Foundation/Types/SharedPtr.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <QMap>
#  include <QObject>
#  include <QPointer>
#  include <QString>
#  include <QWidget>
#  include <atomic>

class xiiQtEditorApp;
class xiiAssetProcessor;
class xiiLongOpControllerManager;
struct xiiAssetProcessorEvent;
struct xiiLongOpControllerEvent;
struct xiiToolsProjectEvent;

/// Live-process desktop toasts for the unpackaged Editor. Own on the GUI thread and destroy
/// before project/operation teardown. No cold activation, background server, or tray fallback.
class XII_EDITORFRAMEWORK_DLL xiiWindowsNotifications final : public QObject
{
public:
  xiiWindowsNotifications(xiiQtEditorApp& editor, QWidget& window);
  ~xiiWindowsNotifications();

private:
  friend struct xiiWindowsNotificationsTest;
  struct Native;
  struct CallbackState : xiiRefCounted
  {
    xiiMutex                 m_Mutex;
    xiiWindowsNotifications* m_pOwner = nullptr;
  };
  struct Tool
  {
    xiiTime   m_Start;
    xiiUInt32 m_uiCount = 0;
    bool      m_bFailed = false;
  };
  struct Message
  {
    QString m_sTitle;
    QString m_sText;
  };

  void             ToolStarted(const QString& sOperation);
  bool             ToolFinished(bool bSucceeded, const QString& sOperation, bool bForeground, Message& out_message);
  void             ProcessorEventHandler(const xiiAssetProcessorEvent& e);
  void             LongOpEventHandler(const xiiLongOpControllerEvent& e);
  void             ProjectEventHandler(const xiiToolsProjectEvent& e);
  void             Notify(const Message& message);
  void             Clear();
  void             Activate();
  bool             IsForeground() const;
  static bool      IsEligible(bool bForeground, double fSeconds, bool bSucceeded, bool bKnownOperation);
  static QString   BuildXml(const Message& message);
  static xiiResult PrepareShortcut(const QString& sShortcut, const QString& sExecutable);
  static void      QueueCallback(const xiiSharedPtr<CallbackState>& state, xiiUInt64 uiGeneration, bool bActivate);

  QPointer<QWidget>           m_pWindow;
  xiiAssetProcessor*          m_pProcessor = nullptr; ///< Core-lifetime borrowed sources.
  xiiLongOpControllerManager* m_pLongOps   = nullptr;
  QMap<QString, Tool>         m_Tools;
  xiiUniquePtr<Native>        m_pNative;
  xiiSharedPtr<CallbackState> m_CallbackState;
  xiiUInt64                   m_uiGeneration = 0;
  xiiTime                     m_LastAttempt;
  std::atomic<xiiUInt64>      m_uiProjectGeneration = 0;
  bool                        m_bStopped            = false;
};

#endif
