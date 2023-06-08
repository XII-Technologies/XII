#pragma once

#include <EditorPluginFileserve/EditorPluginFileserveDLL.h>
#include <Foundation/Containers/Deque.h>
#include <QAbstractListModel>

enum class xiiFileserveActivityType
{
  StartServer,
  StopServer,
  ClientConnect,
  ClientReconnected,
  ClientDisconnect,
  Mount,
  MountFailed,
  Unmount,
  ReadFile,
  WriteFile,
  DeleteFile,
  Other
};

struct xiiQtFileserveActivityItem
{
  QString                  m_Text;
  xiiFileserveActivityType m_Type;
};

class XII_EDITORPLUGINFILESERVE_DLL xiiQtFileserveActivityModel : public QAbstractListModel
{
  Q_OBJECT

public:
  xiiQtFileserveActivityModel(QWidget* pParent);

  virtual int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int      columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index, int iRole = Qt::DisplayRole) const override;
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;

  xiiQtFileserveActivityItem& AppendItem();
  void                        UpdateView();

  void Clear();
private Q_SLOTS:
  void UpdateViewSlot();

private:
  bool                                 m_bTimerRunning = false;
  xiiUInt32                            m_uiAddedItems  = 0;
  xiiDeque<xiiQtFileserveActivityItem> m_Items;
};
