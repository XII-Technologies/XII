/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifdef XII_USE_QT

#  include <QStyledItemDelegate>
#  include <TestFramework/Framework/Qt/qtTestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

class xiiQtTestFramework;

/// Delegate for xiiQtTestModel which shows bars for the test durations.
class XII_TEST_DLL xiiQtTestDelegate : public QStyledItemDelegate
{
  Q_OBJECT
public:
  xiiQtTestDelegate(QObject* pParent);
  virtual ~xiiQtTestDelegate();

public: // QStyledItemDelegate interface
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif
