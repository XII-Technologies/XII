/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/Configuration/PlayerConfig.h>
#include <QDialog>

class QTableWidget;
class QPlainTextEdit;
class QTimer;
class xiiPlayerSessionProtocol;
class xiiPlayerStatusMessage;
struct xiiNetworkChannel;

/// Editor-owned child processes. Closing this window hides it; editor shutdown stops and joins all children.
/// Each child uses a unique Local endpoint on the session's shared network host.
class XII_EDITORFRAMEWORK_DLL xiiQtPlayerSession final : public QDialog
{
public:
  explicit xiiQtPlayerSession(QWidget* pParent = nullptr);
  ~xiiQtPlayerSession();

  xiiStatus Launch(const xiiPlayerLaunchConfig& config, xiiStringView sProject, xiiArrayPtr<const xiiString> arguments);
  xiiResult TransitionInstance(xiiUInt32 uiIndex, xiiStringView sScene, xiiStringView sPreloadCollection = {});
  xiiResult CancelTransition(xiiUInt32 uiIndex);
  void      StopInstance(xiiUInt32 uiIndex);
  void      StopSession(xiiStringView sSession);
  void      StopAll();
  void      Shutdown();
  void      Tick();

  xiiUInt32     GetInstanceCount() const { return m_Instances.GetCount(); }
  xiiStringView GetInstanceState(xiiUInt32 uiIndex) const;
  xiiStringView GetInstanceId(xiiUInt32 uiIndex) const;
  xiiUInt64     GetInstanceSceneRevision(xiiUInt32 uiIndex) const;
  xiiString     GetInstanceLog(xiiUInt32 uiIndex) const;
  xiiStringView GetInstanceDetail(xiiUInt32 uiIndex) const;
  xiiStringView GetSessionId(xiiUInt32 uiIndex) const;

private:
  struct Instance;
  void ProcessControlEvents();
  void HandleStatus(xiiNetworkChannel channel, const xiiPlayerStatusMessage& status);
  void CloseControlConnection(Instance& instance);

  xiiDynamicArray<xiiUniquePtr<Instance>> m_Instances;
  xiiUniquePtr<xiiPlayerSessionProtocol>  m_pControl;
  QTableWidget*                           m_pTable        = nullptr;
  QPlainTextEdit*                         m_pLog          = nullptr;
  QTimer*                                 m_pTimer        = nullptr;
  bool                                    m_bShuttingDown = false;
};
