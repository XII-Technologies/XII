#pragma once

#ifdef XII_USE_QT

#  include <QAbstractItemModel>
#  include <QDockWidget>
#  include <TestFramework/TestFrameworkDLL.h>
#  include <TestFramework/ui_qtLogMessageDock.h>
#  include <vector>

class xiiQtTestFramework;
struct xiiTestResultData;
class xiiQtLogMessageModel;
class xiiTestFrameworkResult;

/// \brief Dock widget that lists the output of a given xiiResult struct.
class XII_TEST_DLL xiiQtLogMessageDock : public QDockWidget, public Ui_qtLogMessageDock
{
  Q_OBJECT
public:
  xiiQtLogMessageDock(QObject* pParent, const xiiTestFrameworkResult* pResult);
  virtual ~xiiQtLogMessageDock();

public Q_SLOTS:
  void resetModel();
  void currentTestResultChanged(const xiiTestResultData* pTestResult);
  void currentTestSelectionChanged(const xiiTestResultData* pTestResult);

private:
  xiiQtLogMessageModel* m_pModel;
};

/// \brief Model used by xiiQtLogMessageDock to list the output entries in xiiResult.
class XII_TEST_DLL xiiQtLogMessageModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  xiiQtLogMessageModel(QObject* pParent, const xiiTestFrameworkResult* pResult);
  virtual ~xiiQtLogMessageModel();

  void        resetModel();
  QModelIndex GetFirstIndexOfTestSelection();
  QModelIndex GetLastIndexOfTestSelection();

public Q_SLOTS:
  void currentTestResultChanged(const xiiTestResultData* pTestResult);
  void currentTestSelectionChanged(const xiiTestResultData* pTestResult);

public: // QAbstractItemModel interface
  virtual QVariant      data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant      headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QModelIndex   index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex   parent(const QModelIndex& index) const override;
  virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:
  void UpdateVisibleEntries();

private:
  const xiiTestResultData*      m_pCurrentTestSelection;
  const xiiTestFrameworkResult* m_pTestResult;
  std::vector<xiiUInt32>        m_VisibleEntries;
  std::vector<xiiUInt8>         m_VisibleEntriesIndention;
};

#endif
