/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/Action/ActionMap.h>
#include <QSharedPointer>
#include <QToolBar>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

class QWidget;
class xiiActionMap;
class QAction;
class xiiQtProxy;
class QMenu;

class XII_GUIFOUNDATION_DLL xiiQtToolBarActionMapView : public QToolBar
{
  Q_OBJECT
  XII_DISALLOW_COPY_AND_ASSIGN(xiiQtToolBarActionMapView);

public:
  explicit xiiQtToolBarActionMapView(QString sTitle, QWidget* pParent);
  ~xiiQtToolBarActionMapView();

  void                    SetActionContext(const xiiActionContext& context);
  const xiiActionContext& GetActionContext() const { return m_Context; }
  void                    SetGroupedPresentation(bool bGrouped) { m_bGrouped = bGrouped; }
  void                    ClearActionContext();

  /// Keep primary buttons compact while making menu choices readable.
  void SetShowMenuLabels(bool bShow);

  virtual void setVisible(bool bVisible) override;

private:
  void TreeEventHandler(const xiiDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);

  void ClearView();
  void CreateView();
  void CreateView(const xiiActionMap::TreeNode* pRoot, QToolBar* pTarget);

private:
  xiiHashTable<xiiUuid, QSharedPointer<xiiQtProxy>> m_Proxies;

  xiiActionContext m_Context;
  xiiActionMap*    m_pActionMap;
  bool             m_bShowMenuLabels = false;
  bool             m_bGrouped        = false;
  QList<QAction*>  m_OwnedActions;
};
