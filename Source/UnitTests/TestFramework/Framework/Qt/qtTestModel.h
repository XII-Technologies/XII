/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifdef XII_USE_QT

#  include <QAbstractItemModel>
#  include <QColor>
#  include <QIcon>
#  include <TestFramework/Framework/Qt/qtTestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

class xiiQtTestFramework;

/// Helper class that stores the test hierarchy used in xiiQtTestModel.
class xiiQtTestModelEntry
{
public:
  xiiQtTestModelEntry(const xiiTestFrameworkResult* pResult, xiiInt32 iTestIndex = -1, xiiInt32 iSubTestIndex = -1);
  ~xiiQtTestModelEntry();

private:
  xiiQtTestModelEntry(xiiQtTestModelEntry&);
  void operator=(xiiQtTestModelEntry&);

public:
  enum xiiTestModelEntryType
  {
    RootNode,
    TestNode,
    SubTestNode
  };

  void                     ClearEntries();
  xiiUInt32                GetNumSubEntries() const;
  xiiQtTestModelEntry*     GetSubEntry(xiiUInt32 uiIndex) const;
  void                     AddSubEntry(xiiQtTestModelEntry* pEntry);
  xiiQtTestModelEntry*     GetParentEntry() const { return m_pParentEntry; }
  xiiUInt32                GetIndexInParent() const { return m_uiIndexInParent; }
  xiiTestModelEntryType    GetNodeType() const;
  const xiiTestResultData* GetTestResult() const;
  xiiInt32                 GetTestIndex() const { return m_iTestIndex; }
  xiiInt32                 GetSubTestIndex() const { return m_iSubTestIndex; }

private:
  const xiiTestFrameworkResult* m_pResult;
  xiiInt32                      m_iTestIndex;
  xiiInt32                      m_iSubTestIndex;

  xiiQtTestModelEntry*             m_pParentEntry    = nullptr;
  xiiUInt32                        m_uiIndexInParent = 0;
  std::deque<xiiQtTestModelEntry*> m_SubEntries;
};

/// A Model that lists all unit tests and sub-tests in a tree.
class XII_TEST_DLL xiiQtTestModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  xiiQtTestModel(QObject* pParent, xiiQtTestFramework* pTestFramework);
  virtual ~xiiQtTestModel();

  void Reset();
  void InvalidateAll();
  void TestDataChanged(xiiInt32 iTestIndex, xiiInt32 iSubTestIndex);

  struct UserRoles
  {
    enum Enum
    {
      Duration      = Qt::UserRole,
      DurationColor = Qt::UserRole + 1,
    };
  };

  struct Columns
  {
    enum Enum
    {
      Name = 0,
      Status,
      Duration,
      Errors,
      Asserts,
      Progress,
      ColumnCount,
    };
  };

public: // QAbstractItemModel interface
  virtual QVariant      data(const QModelIndex& index, int iRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant      headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex   index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex   parent(const QModelIndex& index) const override;
  virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual bool          setData(const QModelIndex& index, const QVariant& value, int iRole = Qt::EditRole) override;

public Q_SLOTS:
  void UpdateModel();

private:
  xiiQtTestFramework*     m_pTestFramework;
  xiiTestFrameworkResult* m_pResult;
  xiiQtTestModelEntry     m_Root;
  QColor                  m_SucessColor;
  QColor                  m_FailedColor;
  QColor                  m_CustomStatusColor;
  QColor                  m_TestColor;
  QColor                  m_SubTestColor;
  QIcon                   m_TestIcon;
  QIcon                   m_TestIconOff;
};

#endif
