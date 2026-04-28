/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginFileserve/EditorPluginFileservePCH.h>

#include <EditorPluginFileserve/FileserveUI/ActivityModel.moc.h>

xiiQtFileserveActivityModel::xiiQtFileserveActivityModel(QWidget* pParent) :
  QAbstractListModel(pParent)
{
}

int xiiQtFileserveActivityModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return m_Items.GetCount() - m_uiAddedItems;
}

int xiiQtFileserveActivityModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 2;
}

QVariant xiiQtFileserveActivityModel::data(const QModelIndex& index, int iRole /*= Qt::DisplayRole*/) const
{
  if (!index.isValid())
    return QVariant();

  const auto& item = m_Items[index.row()];

  if (iRole == Qt::ToolTipRole)
  {
    if (item.m_Type == xiiFileserveActivityType::ReadFile)
    {
      return QString("[TIME] == File was not transferred because the timestamps match on server and client.\n"
                     "[HASH] == File was not transferred because the file hashes matched on server and client.\n"
                     "[N/A] == File does not exist on the server (in the requested data directory).");
    }
  }

  if (index.column() == 0)
  {
    if (iRole == Qt::DisplayRole)
    {
      switch (item.m_Type)
      {
        case xiiFileserveActivityType::StartServer:
          return "Server Started";
        case xiiFileserveActivityType::StopServer:
          return "Server Stopped";
        case xiiFileserveActivityType::ClientConnect:
          return "Client Connected";
        case xiiFileserveActivityType::ClientReconnected:
          return "Client Re-connected";
        case xiiFileserveActivityType::ClientDisconnect:
          return "Client Disconnect";
        case xiiFileserveActivityType::Mount:
          return "Mount";
        case xiiFileserveActivityType::MountFailed:
          return "Failed Mount";
        case xiiFileserveActivityType::Unmount:
          return "Unmount";
        case xiiFileserveActivityType::ReadFile:
          return "Read";
        case xiiFileserveActivityType::WriteFile:
          return "Write";
        case xiiFileserveActivityType::DeleteFile:
          return "Delete";

        default:
          return QVariant();
      }
    }

    if (iRole == Qt::ForegroundRole)
    {
      switch (item.m_Type)
      {
        case xiiFileserveActivityType::StartServer:
          return QColor::fromRgb(0, 200, 0);
        case xiiFileserveActivityType::StopServer:
          return QColor::fromRgb(200, 200, 0);

        case xiiFileserveActivityType::ClientConnect:
        case xiiFileserveActivityType::ClientReconnected:
          return QColor::fromRgb(50, 200, 0);
        case xiiFileserveActivityType::ClientDisconnect:
          return QColor::fromRgb(250, 100, 0);

        case xiiFileserveActivityType::Mount:
          return QColor::fromRgb(0, 0, 200);
        case xiiFileserveActivityType::MountFailed:
          return QColor::fromRgb(255, 0, 0);
        case xiiFileserveActivityType::Unmount:
          return QColor::fromRgb(150, 0, 200);

        case xiiFileserveActivityType::ReadFile:
          return QColor::fromRgb(100, 100, 100);
        case xiiFileserveActivityType::WriteFile:
          return QColor::fromRgb(255, 150, 0);
        case xiiFileserveActivityType::DeleteFile:
          return QColor::fromRgb(200, 50, 50);

        default:
          return QVariant();
      }
    }
  }

  if (index.column() == 1)
  {
    if (iRole == Qt::DisplayRole)
    {
      return item.m_Text;
    }
  }

  return QVariant();
}


QVariant xiiQtFileserveActivityModel::headerData(int iSection, Qt::Orientation orientation, int iRole /*= Qt::DisplayRole*/) const
{
  if (iRole == Qt::DisplayRole)
  {
    if (iSection == 0)
    {
      return "Type";
    }

    if (iSection == 1)
    {
      return "Action";
    }
  }

  return QVariant();
}

xiiQtFileserveActivityItem& xiiQtFileserveActivityModel::AppendItem()
{
  if (!m_bTimerRunning)
  {
    m_bTimerRunning = true;

    QTimer::singleShot(250, this, &xiiQtFileserveActivityModel::UpdateViewSlot);
  }

  m_uiAddedItems++;
  return m_Items.ExpandAndGetRef();
}

void xiiQtFileserveActivityModel::UpdateView()
{
  if (m_uiAddedItems == 0)
    return;

  beginInsertRows(QModelIndex(), m_Items.GetCount() - m_uiAddedItems, m_Items.GetCount() - 1);
  insertRows(m_Items.GetCount(), m_uiAddedItems, QModelIndex());
  m_uiAddedItems = 0;
  endInsertRows();
}

void xiiQtFileserveActivityModel::Clear()
{
  m_Items.Clear();
  m_uiAddedItems = 0;

  beginResetModel();
  endResetModel();
}

void xiiQtFileserveActivityModel::UpdateViewSlot()
{
  m_bTimerRunning = false;

  UpdateView();
}
