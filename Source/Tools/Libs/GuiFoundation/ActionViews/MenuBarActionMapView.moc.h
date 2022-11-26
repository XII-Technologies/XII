#pragma once

#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QMenuBar>
#include <QSharedPointer>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

class QWidget;
class xiiActionMap;
class QAction;
class xiiQtProxy;

class XII_GUIFOUNDATION_DLL xiiQtMenuBarActionMapView : public QMenuBar
{
  Q_OBJECT
  XII_DISALLOW_COPY_AND_ASSIGN(xiiQtMenuBarActionMapView);

public:
  explicit xiiQtMenuBarActionMapView(QWidget* parent);
  ~xiiQtMenuBarActionMapView();

  void SetActionContext(const xiiActionContext& context);

private:
  void TreeEventHandler(const xiiDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);

  void ClearView();
  void CreateView();

private:
  xiiHashTable<xiiUuid, QSharedPointer<xiiQtProxy>> m_Proxies;

  xiiActionContext m_Context;
  xiiActionMap*    m_pActionMap;
};
