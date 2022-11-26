#pragma once

#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/GuiFoundationDLL.h>
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
  explicit xiiQtToolBarActionMapView(QString title, QWidget* parent);
  ~xiiQtToolBarActionMapView();

  void SetActionContext(const xiiActionContext& context);

  virtual void setVisible(bool visible) override;

private:
  void TreeEventHandler(const xiiDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);

  void ClearView();
  void CreateView();
  void CreateView(const xiiActionMap::TreeNode* pRoot);

private:
  xiiHashTable<xiiUuid, QSharedPointer<xiiQtProxy>> m_Proxies;

  xiiActionContext m_Context;
  xiiActionMap*    m_pActionMap;
};
