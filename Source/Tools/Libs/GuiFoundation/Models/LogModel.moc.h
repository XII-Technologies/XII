/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Logging/LogEntry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QAbstractItemModel>

/// The Qt model that represents log output for a view
class XII_GUIFOUNDATION_DLL xiiQtLogModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  xiiQtLogModel(QObject* pParent);
  void Clear();
  void SetLogLevel(xiiLogMsgType::Enum logLevel);
  void SetSearchText(xiiStringView sText);
  void AddLogMsg(const xiiLogEntry& msg);

  xiiUInt32 GetVisibleItemCount() const { return m_VisibleMessages.GetCount(); }

  xiiUInt32 GetNumErrors() const { return m_uiNumErrors; }
  xiiUInt32 GetNumSeriousWarnings() const { return m_uiNumSeriousWarnings; }
  xiiUInt32 GetNumWarnings() const { return m_uiNumWarnings; }

public: // QAbstractItemModel interface
  virtual QVariant      data(const QModelIndex& index, int iRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant      headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex   index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex   parent(const QModelIndex& index) const override;
  virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const override;

Q_SIGNALS:
  void NewErrorsOrWarnings(xiiStringView sLatest, bool bError);

private Q_SLOTS:
  /// Adds queued messages from a different thread to the model.
  void ProcessNewMessages();

private:
  void Invalidate();
  bool IsFiltered(const xiiLogEntry& lm) const;
  void UpdateVisibleEntries() const;

  xiiLogMsgType::Enum   m_LogLevel;
  xiiString             m_sSearchText;
  xiiDeque<xiiLogEntry> m_AllMessages;

  mutable bool                                   m_bIsValid;
  mutable xiiDeque<const xiiLogEntry*>           m_VisibleMessages;
  mutable xiiHybridArray<const xiiLogEntry*, 16> m_BlockQueue;

  mutable xiiMutex      m_NewMessagesMutex;
  xiiDeque<xiiLogEntry> m_NewMessages;

  xiiUInt32 m_uiNumErrors          = 0;
  xiiUInt32 m_uiNumSeriousWarnings = 0;
  xiiUInt32 m_uiNumWarnings        = 0;
};
