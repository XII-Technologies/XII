/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <TestFramework/TestFrameworkPCH.h>

#ifdef XII_USE_QT

#  include <QStringBuilder>
#  include <TestFramework/Framework/Qt/qtLogMessageDock.h>
#  include <TestFramework/Framework/TestFramework.h>

////////////////////////////////////////////////////////////////////////
// xiiQtLogMessageDock public functions
////////////////////////////////////////////////////////////////////////

xiiQtLogMessageDock::xiiQtLogMessageDock(QObject* pParent, const xiiTestFrameworkResult* pResult)
{
  setupUi(this);
  m_pModel = new xiiQtLogMessageModel(this, pResult);
  ListView->setModel(m_pModel);
}

xiiQtLogMessageDock::~xiiQtLogMessageDock()
{
  ListView->setModel(nullptr);
  delete m_pModel;
  m_pModel = nullptr;
}

void xiiQtLogMessageDock::resetModel()
{
  m_pModel->resetModel();
}

void xiiQtLogMessageDock::currentTestResultChanged(const xiiTestResultData* pTestResult)
{
  m_pModel->currentTestResultChanged(pTestResult);
  ListView->scrollToBottom();
}

void xiiQtLogMessageDock::currentTestSelectionChanged(const xiiTestResultData* pTestResult)
{
  m_pModel->currentTestSelectionChanged(pTestResult);
  ListView->scrollTo(m_pModel->GetLastIndexOfTestSelection(), QAbstractItemView::EnsureVisible);
  ListView->scrollTo(m_pModel->GetFirstIndexOfTestSelection(), QAbstractItemView::EnsureVisible);
}

////////////////////////////////////////////////////////////////////////
// xiiQtLogMessageModel public functions
////////////////////////////////////////////////////////////////////////

xiiQtLogMessageModel::xiiQtLogMessageModel(QObject* pParent, const xiiTestFrameworkResult* pResult) :
  QAbstractItemModel(pParent), m_pTestResult(pResult)
{
}

xiiQtLogMessageModel::~xiiQtLogMessageModel() = default;

void xiiQtLogMessageModel::resetModel()
{
  beginResetModel();
  currentTestResultChanged(nullptr);
  endResetModel();
}

QModelIndex xiiQtLogMessageModel::GetFirstIndexOfTestSelection()
{
  if (m_pCurrentTestSelection == nullptr || m_pCurrentTestSelection->m_iFirstOutput == -1)
    return QModelIndex();

  xiiInt32 iEntries = (xiiInt32)m_VisibleEntries.size();
  for (int i = 0; i < iEntries; ++i)
  {
    if ((xiiInt32)m_VisibleEntries[i] >= m_pCurrentTestSelection->m_iFirstOutput)
      return index(i, 0);
  }
  return index(rowCount() - 1, 0);
}

QModelIndex xiiQtLogMessageModel::GetLastIndexOfTestSelection()
{
  if (m_pCurrentTestSelection == nullptr || m_pCurrentTestSelection->m_iLastOutput == -1)
    return QModelIndex();

  xiiInt32 iEntries = (xiiInt32)m_VisibleEntries.size();
  for (int i = 0; i < iEntries; ++i)
  {
    if ((xiiInt32)m_VisibleEntries[i] >= m_pCurrentTestSelection->m_iLastOutput)
      return index(i, 0);
  }
  return index(rowCount() - 1, 0);
}

void xiiQtLogMessageModel::currentTestResultChanged(const xiiTestResultData* pTestResult)
{
  UpdateVisibleEntries();
  currentTestSelectionChanged(pTestResult);
}

void xiiQtLogMessageModel::currentTestSelectionChanged(const xiiTestResultData* pTestResult)
{
  m_pCurrentTestSelection = pTestResult;
  if (m_pCurrentTestSelection != nullptr)
  {
    dataChanged(index(m_pCurrentTestSelection->m_iFirstOutput, 0), index(m_pCurrentTestSelection->m_iLastOutput, 0));
  }
}


////////////////////////////////////////////////////////////////////////
// xiiQtLogMessageModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant xiiQtLogMessageModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || m_pTestResult == nullptr || index.column() != 0)
    return QVariant();

  const xiiInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (xiiInt32)m_VisibleEntries.size())
    return QVariant();

  const xiiUInt32             uiLogIdx    = m_VisibleEntries[iRow];
  const xiiUInt8              uiIndention = m_VisibleEntriesIndention[iRow];
  const xiiTestOutputMessage& Message     = *m_pTestResult->GetOutputMessage(uiLogIdx);
  const xiiTestErrorMessage*  pError      = (Message.m_iErrorIndex != -1) ? m_pTestResult->GetErrorMessage(Message.m_iErrorIndex) : nullptr;
  switch (iRole)
  {
    case Qt::DisplayRole:
    {
      if (pError != nullptr)
      {
        QString sBlockStart = QLatin1String("\n") % QString((uiIndention + 1) * 3, ' ');
        QString sBlockName =
          pError->m_sBlock.empty() ? QLatin1String("") : (sBlockStart % QLatin1String("Block: ") + QLatin1String(pError->m_sBlock.c_str()));
        QString sMessage =
          pError->m_sMessage.empty() ? QLatin1String("") : (sBlockStart % QLatin1String("Message: ") + QLatin1String(pError->m_sMessage.c_str()));
        QString sErrorMessage = QString(uiIndention * 3, ' ') % QString(Message.m_sMessage.c_str()) % sBlockName % sBlockStart %
          QLatin1String("File: ") % QLatin1String(pError->m_sFile.c_str()) % sBlockStart % QLatin1String("Line: ") %
          QString::number(pError->m_iLine) % sBlockStart % QLatin1String("Function: ") %
          QLatin1String(pError->m_sFunction.c_str()) % sMessage;

        return sErrorMessage;
      }
      return QString(uiIndention * 3, ' ') + QString(Message.m_sMessage.c_str());
    }
    case Qt::ForegroundRole:
    {
      switch (Message.m_Type)
      {
        case xiiTestOutput::BeginBlock:
        case xiiTestOutput::Message:
          return QColor(Qt::yellow);
        case xiiTestOutput::Error:
          return QColor(Qt::red);
        case xiiTestOutput::Success:
          return QColor(Qt::green);
        case xiiTestOutput::Warning:
          return QColor(qRgb(255, 100, 0));
        case xiiTestOutput::StartOutput:
        case xiiTestOutput::EndBlock:
        case xiiTestOutput::ImportantInfo:
        case xiiTestOutput::Details:
        case xiiTestOutput::Duration:
        case xiiTestOutput::FinalResult:
          return QVariant();
        default:
          return QVariant();
      }
    }
    case Qt::BackgroundRole:
    {
      QPalette palette = QApplication::palette();
      if (m_pCurrentTestSelection != nullptr && m_pCurrentTestSelection->m_iFirstOutput != -1)
      {
        if (m_pCurrentTestSelection->m_iFirstOutput <= (xiiInt32)uiLogIdx && (xiiInt32)uiLogIdx <= m_pCurrentTestSelection->m_iLastOutput)
        {
          return palette.midlight().color();
        }
      }
      return palette.base().color();
    }

    default:
      return QVariant();
  }
}

Qt::ItemFlags xiiQtLogMessageModel::flags(const QModelIndex& index) const
{
  if (!index.isValid() || m_pTestResult == nullptr)
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant xiiQtLogMessageModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  if (orientation == Qt::Horizontal && iRole == Qt::DisplayRole)
  {
    switch (iSection)
    {
      case 0:
        return QString("Log Entry");
    }
  }
  return QVariant();
}

QModelIndex xiiQtLogMessageModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (parent.isValid() || m_pTestResult == nullptr || iColumn != 0)
    return QModelIndex();

  return createIndex(iRow, iColumn, iRow);
}

QModelIndex xiiQtLogMessageModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int xiiQtLogMessageModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid() || m_pTestResult == nullptr)
    return 0;

  return (int)m_VisibleEntries.size();
}

int xiiQtLogMessageModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}


////////////////////////////////////////////////////////////////////////
// xiiQtLogMessageModel private functions
////////////////////////////////////////////////////////////////////////

void xiiQtLogMessageModel::UpdateVisibleEntries()
{
  m_VisibleEntries.clear();
  m_VisibleEntriesIndention.clear();
  if (m_pTestResult == nullptr)
    return;

  xiiUInt8  uiIndention = 0;
  xiiUInt32 uiEntries   = m_pTestResult->GetOutputMessageCount();
  /// \todo filter out uninteresting messages
  for (xiiUInt32 i = 0; i < uiEntries; ++i)
  {
    xiiTestOutput::Enum Type = m_pTestResult->GetOutputMessage(i)->m_Type;
    if (Type == xiiTestOutput::BeginBlock)
      uiIndention++;
    if (Type == xiiTestOutput::EndBlock)
      uiIndention--;

    m_VisibleEntries.push_back(i);
    m_VisibleEntriesIndention.push_back(uiIndention);
  }
  beginResetModel();
  endResetModel();
}

#endif

XII_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtLogMessageDock);
