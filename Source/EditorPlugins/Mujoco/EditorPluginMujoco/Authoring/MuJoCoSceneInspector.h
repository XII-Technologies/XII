/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <EditorPluginMujoco/EditorPluginMujocoDLL.h>

#include <EnginePluginMujoco/MuJoCoInspectorMessages.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Time/Time.h>
#include <QHash>
#include <QWidget>

class QLabel;
class QComboBox;
class QTreeView;
class QStandardItem;
class QStandardItemModel;
class QPlainTextEdit;
class QLineEdit;
class QPushButton;
class QSlider;
class QCheckBox;
class xiiSceneDocument;
class xiiQtEngineDocumentWindow;
struct xiiQtDocumentWindowEvent;

class xiiQtMuJoCoSceneInspector final : public QWidget
{
public:
  xiiQtMuJoCoSceneInspector(xiiSceneDocument& document, xiiQtEngineDocumentWindow& window);
  ~xiiQtMuJoCoSceneInspector() override;

private:
  void Tick();
  void WindowEvent(const xiiQtDocumentWindowEvent& event);
  void HandleMessage(const xiiEditorEngineDocumentMsg* pMessage);
  void Request(xiiMuJoCoInspectorCommand command, xiiUInt64 uiOffset = 0);
  void ApplyValues(xiiMuJoCoInspectorCommand command);
  void ResetSelection();
  void AppendElements(const xiiVariantArray& entries);
  void ShowSelected(const xiiVariantDictionary& values, bool bPaused);
  void Locate();
  void BindSelectedElement();
  void OpenModelAsset();
  void UpdateSettings();
  void FollowSceneSelection();

  xiiSceneDocument&            m_Document;
  xiiQtEngineDocumentWindow&   m_Window;
  xiiMuJoCoInspectorSyncObject m_Sync;
  QComboBox*                   m_pModels           = nullptr;
  QTreeView*                   m_pTree             = nullptr;
  QStandardItemModel*          m_pTreeModel        = nullptr;
  QLabel*                      m_pStatus           = nullptr;
  QLabel*                      m_pStatistics       = nullptr;
  QPlainTextEdit*              m_pDetails          = nullptr;
  QLineEdit*                   m_pValues           = nullptr;
  QSlider*                     m_pSlider           = nullptr;
  QComboBox*                   m_pEditMode         = nullptr;
  QCheckBox*                   m_pContacts         = nullptr;
  QPushButton*                 m_pMore             = nullptr;
  QPushButton*                 m_pNextContacts     = nullptr;
  QPushButton*                 m_pPreviousContacts = nullptr;
  QPushButton*                 m_pStartPause       = nullptr;
  QPushButton*                 m_pReset            = nullptr;
  QPushButton*                 m_pStep             = nullptr;
  QPushButton*                 m_pApply            = nullptr;
  QPushButton*                 m_pResetJoint       = nullptr;
  QPushButton*                 m_pLocate           = nullptr;
  QPushButton*                 m_pBind             = nullptr;
  QHash<int, QStandardItem*>   m_Bodies;
  xiiVariantArray              m_Models;
  xiiUInt64                    m_uiNextElement       = 0;
  xiiUInt32                    m_uiContactOffset     = 0;
  xiiUInt32                    m_uiExpectedValues    = 0;
  xiiUInt32                    m_uiPositionCount     = 0;
  xiiUInt32                    m_uiVelocityCount     = 0;
  bool                         m_bHierarchyRequested = false;
  bool                         m_bWaiting            = false;
  bool                         m_bHasPosition        = false;
  bool                         m_bCanEdit            = false;
  xiiTime                      m_LastRequest;
  double                       m_fSliderMin = 0;
  double                       m_fSliderMax = 0;
  xiiVec3                      m_vPosition  = xiiVec3::MakeZero();
  xiiUuid                      m_BoundObject;
  xiiUuid                      m_LastSceneSelection;
  xiiString                    m_sSelectedName;
};

void xiiRegisterMuJoCoEditor();
void xiiUnregisterMuJoCoEditor();
