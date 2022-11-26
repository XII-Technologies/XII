#pragma once

#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QMenu>
#include <QSharedPointer>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

class QWidget;
class xiiActionMap;
class QAction;
class xiiQtProxy;


class XII_GUIFOUNDATION_DLL xiiQtMenuActionMapView : public QMenu
{
  Q_OBJECT
  XII_DISALLOW_COPY_AND_ASSIGN(xiiQtMenuActionMapView);

public:
  explicit xiiQtMenuActionMapView(QWidget* parent);
  ~xiiQtMenuActionMapView();

  void SetActionContext(const xiiActionContext& context);

  static void AddDocumentObjectToMenu(xiiHashTable<xiiUuid, QSharedPointer<xiiQtProxy>>& Proxies, xiiActionContext& Context, xiiActionMap* pActionMap, QMenu* pCurrentRoot, const xiiActionMap::TreeNode* pObject);

private:
  void ClearView();
  void CreateView();

private:
  xiiHashTable<xiiUuid, QSharedPointer<xiiQtProxy>> m_Proxies;

  xiiActionContext m_Context;
  xiiActionMap*    m_pActionMap;
};
