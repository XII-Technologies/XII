#include <TestFramework/TestFrameworkPCH.h>

#ifdef XII_USE_QT

#  include <QApplication>
#  include <QPainter>
#  include <TestFramework/Framework/Qt/qtTestDelegate.h>
#  include <TestFramework/Framework/Qt/qtTestModel.h>

////////////////////////////////////////////////////////////////////////
// xiiQtTestDelegate public functions
////////////////////////////////////////////////////////////////////////

xiiQtTestDelegate::xiiQtTestDelegate(QObject* pParent) :
  QStyledItemDelegate(pParent)
{
}

xiiQtTestDelegate::~xiiQtTestDelegate() {}

void xiiQtTestDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  if (index.column() == xiiQtTestModel::Columns::Duration)
  {
    // We need to draw the alternate background color here because setting it via the model would
    // overwrite our duration bar.
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(option.palette.alternateBase());
    painter->drawRect(option.rect);
    painter->restore();

    bool  bSuccess  = false;
    float fProgress = index.data(xiiQtTestModel::UserRoles::Duration).toFloat(&bSuccess);

    // If we got a valid float from the model we can draw a small duration bar on top of the background.
    if (bSuccess)
    {
      QColor               DurationColor = index.data(xiiQtTestModel::UserRoles::DurationColor).value<QColor>();
      QStyleOptionViewItem option2       = option;
      option2.palette.setBrush(QPalette::Base, QBrush(DurationColor));
      option2.rect.setWidth((int)((float)option2.rect.width() * fProgress));
      QApplication::style()->drawControl(QStyle::CE_ProgressBarGroove, &option2, painter);
    }
  }

  QStyledItemDelegate::paint(painter, option, index);
}

#endif

XII_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestDelegate);
