#include <TestFramework/TestFrameworkPCH.h>

#ifdef XII_USE_QT

#  include <QApplication>
#  include <QPalette>
#  include <TestFramework/Framework/Qt/qtTestModel.h>

////////////////////////////////////////////////////////////////////////
// xiiQtTestModelEntry public functions
////////////////////////////////////////////////////////////////////////

xiiQtTestModelEntry::xiiQtTestModelEntry(const xiiTestFrameworkResult* pResult, xiiInt32 iTestIndex, xiiInt32 iSubTestIndex) :
  m_pResult(pResult), m_iTestIndex(iTestIndex), m_iSubTestIndex(iSubTestIndex)

{
}

xiiQtTestModelEntry::~xiiQtTestModelEntry()
{
  ClearEntries();
}

void xiiQtTestModelEntry::ClearEntries()
{
  for (xiiInt32 i = (xiiInt32)m_SubEntries.size() - 1; i >= 0; --i)
  {
    delete m_SubEntries[i];
  }
  m_SubEntries.clear();
}
xiiUInt32 xiiQtTestModelEntry::GetNumSubEntries() const

{
  return (xiiInt32)m_SubEntries.size();
}

xiiQtTestModelEntry* xiiQtTestModelEntry::GetSubEntry(xiiUInt32 uiIndex) const
{
  if (uiIndex >= GetNumSubEntries())
    return nullptr;

  return m_SubEntries[uiIndex];
}

void xiiQtTestModelEntry::AddSubEntry(xiiQtTestModelEntry* pEntry)
{
  pEntry->m_pParentEntry    = this;
  pEntry->m_uiIndexInParent = (xiiUInt32)m_SubEntries.size();
  m_SubEntries.push_back(pEntry);
}

xiiQtTestModelEntry::xiiTestModelEntryType xiiQtTestModelEntry::GetNodeType() const
{
  return (m_iTestIndex == -1) ? RootNode : ((m_iSubTestIndex == -1) ? TestNode : SubTestNode);
}

const xiiTestResultData* xiiQtTestModelEntry::GetTestResult() const
{
  switch (GetNodeType())
  {
    case xiiQtTestModelEntry::TestNode:
    case xiiQtTestModelEntry::SubTestNode:
      return &m_pResult->GetTestResultData(m_iTestIndex, m_iSubTestIndex);
    default:
      return nullptr;
  }
}

static QColor ToneColor(const QColor& inputColor, const QColor& toneColor)
{
  qreal fHue        = toneColor.hueF();
  qreal fSaturation = 1.0f;
  qreal fLightness  = inputColor.lightnessF();
  fLightness        = xiiMath::Clamp(fLightness, 0.20, 0.80);
  return QColor::fromHslF(fHue, fSaturation, fLightness);
}

////////////////////////////////////////////////////////////////////////
// xiiQtTestModel public functions
////////////////////////////////////////////////////////////////////////

xiiQtTestModel::xiiQtTestModel(QObject* pParent, xiiQtTestFramework* pTestFramework) :
  QAbstractItemModel(pParent), m_pTestFramework(pTestFramework), m_Root(nullptr)
{
  QPalette palette = QApplication::palette();
  m_pResult        = &pTestFramework->GetTestResult();

  // Derive state colors from the current active palette.
  m_SucessColor       = ToneColor(palette.text().color(), QColor(Qt::green)).toRgb();
  m_FailedColor       = ToneColor(palette.text().color(), QColor(Qt::red)).toRgb();
  m_CustomStatusColor = ToneColor(palette.text().color(), QColor(Qt::yellow)).toRgb();

  m_TestColor    = ToneColor(palette.base().color(), QColor(Qt::cyan)).toRgb();
  m_SubTestColor = ToneColor(palette.base().color(), QColor(Qt::blue)).toRgb();

  m_TestIcon    = QIcon(":/Icons/Icons/pie.png");
  m_TestIconOff = QIcon(":/Icons/Icons/pie_off.png");

  UpdateModel();
}

xiiQtTestModel::~xiiQtTestModel()
{
  m_Root.ClearEntries();
}

void xiiQtTestModel::Reset()
{
  beginResetModel();
  endResetModel();
}

void xiiQtTestModel::InvalidateAll()
{
  dataChanged(QModelIndex(), QModelIndex());
}

void xiiQtTestModel::TestDataChanged(xiiInt32 iTestIndex, xiiInt32 iSubTestIndex)
{
  QModelIndex TestModelIndex = index(iTestIndex, 0);
  // Invalidate whole test row
  Q_EMIT dataChanged(TestModelIndex, index(iTestIndex, columnCount() - 1));

  // Invalidate all sub-tests
  const xiiQtTestModelEntry* pEntry    = (xiiQtTestModelEntry*)TestModelIndex.internalPointer();
  xiiInt32                   iChildren = (xiiInt32)pEntry->GetNumSubEntries();
  Q_EMIT dataChanged(index(0, 0, TestModelIndex), index(iChildren - 1, columnCount() - 1, TestModelIndex));
}


////////////////////////////////////////////////////////////////////////
// xiiQtTestModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant xiiQtTestModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid())
    return QVariant();

  const xiiQtTestModelEntry*                       pEntry       = (xiiQtTestModelEntry*)index.internalPointer();
  const xiiQtTestModelEntry*                       pParentEntry = pEntry->GetParentEntry();
  const xiiQtTestModelEntry::xiiTestModelEntryType entryType    = pEntry->GetNodeType();

  const xiiInt32 iExecutingTest    = m_pTestFramework->GetCurrentTestIndex();
  const xiiInt32 iExecutingSubTest = m_pTestFramework->GetCurrentSubTestIndex();

  const bool bIsExecuting = pEntry->GetTestIndex() == iExecutingTest && pEntry->GetSubTestIndex() == iExecutingSubTest;

  bool               bTestEnabled          = true;
  bool               bParentEnabled        = true;
  bool               bIsSubTest            = entryType == xiiQtTestModelEntry::SubTestNode;
  const std::string& testUnavailableReason = m_pTestFramework->IsTestAvailable(bIsSubTest ? pParentEntry->GetTestIndex() : pEntry->GetTestIndex());

  if (bIsSubTest)
  {
    bTestEnabled   = m_pTestFramework->IsSubTestEnabled(pEntry->GetTestIndex(), pEntry->GetSubTestIndex());
    bParentEnabled = m_pTestFramework->IsTestEnabled(pParentEntry->GetTestIndex());
  }
  else
  {
    bTestEnabled = m_pTestFramework->IsTestEnabled(pEntry->GetTestIndex());
  }

  const xiiTestResultData& TestResult = *pEntry->GetTestResult();

  if (bIsExecuting && iRole == Qt::BackgroundRole)
  {
    return QColor(115, 100, 40);
  }

  // Name
  if (index.column() == Columns::Name)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return QString(TestResult.m_sName.c_str());
      }
      case Qt::CheckStateRole:
      {
        return bTestEnabled ? Qt::Checked : Qt::Unchecked;
      }
      case Qt::DecorationRole:
      {
        return (bTestEnabled && bParentEnabled) ? m_TestIcon : m_TestIconOff;
      }
      case Qt::ForegroundRole:
      {
        if (!testUnavailableReason.empty())
        {
          QPalette palette = QApplication::palette();
          return palette.color(QPalette::Disabled, QPalette::Text);
        }
      }
      case Qt::ToolTipRole:
      {
        if (!testUnavailableReason.empty())
        {
          return QString("Test not available: %1").arg(testUnavailableReason.c_str());
        }
      }
      default:
        return QVariant();
    }
  }
  // Status
  else if (index.column() == Columns::Status)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        if (!testUnavailableReason.empty())
        {
          return QString("Test not available: %1").arg(testUnavailableReason.c_str());
        }
        else if (bTestEnabled && bParentEnabled)
        {
          if (bIsSubTest)
          {
            return QString("Enabled");
          }
          else
          {
            // Count sub-test status
            const xiiUInt32 iSubTests = m_pResult->GetSubTestCount(pEntry->GetTestIndex());
            const xiiUInt32 iEnabled  = m_pTestFramework->GetSubTestEnabledCount(pEntry->GetTestIndex());

            if (iEnabled == iSubTests)
            {
              return QString("All Enabled");
            }

            return QString("%1 / %2 Enabled").arg(iEnabled).arg(iSubTests);
          }
        }
        else
        {
          return QString("Disabled");
        }
      }
      case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }
      default:
        return QVariant();
    }
  }
  // Duration
  else if (index.column() == Columns::Duration)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return QLocale(QLocale::English).toString(TestResult.m_fTestDuration, 'f', 4);
      }
      case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }
      /*case Qt::BackgroundRole:
        {
          QPalette palette = QApplication::palette();
          return palette.alternateBase().color();
        }*/
      case UserRoles::Duration:
      {
        if (bIsSubTest && TestResult.m_bExecuted)
        {
          return TestResult.m_fTestDuration / pParentEntry->GetTestResult()->m_fTestDuration;
        }
        else if (TestResult.m_bExecuted)
        {
          return TestResult.m_fTestDuration / m_pTestFramework->GetTotalTestDuration();
        }
        return QVariant();
      }
      case UserRoles::DurationColor:
      {
        if (TestResult.m_bExecuted)
        {
          return (bIsSubTest ? m_SubTestColor : m_TestColor);
        }
        return QVariant();
      }
      default:
        return QVariant();
    }
  }
  // Errors
  else if (index.column() == Columns::Errors)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return QString("%1 / %2")
          .arg(m_pResult->GetErrorMessageCount(pEntry->GetTestIndex(), pEntry->GetSubTestIndex()))
          .arg(m_pResult->GetOutputMessageCount(pEntry->GetTestIndex(), pEntry->GetSubTestIndex()));
      }
      case Qt::BackgroundRole:
      {
        QPalette palette = QApplication::palette();
        return palette.alternateBase().color();
      }
      case Qt::ForegroundRole:
      {
        if (TestResult.m_bExecuted)
        {
          return (m_pResult->GetErrorMessageCount(pEntry->GetTestIndex(), pEntry->GetSubTestIndex()) == 0) ? m_SucessColor : m_FailedColor;
        }
        return QVariant();
      }
      case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }

      default:
        return QVariant();
    }
  }
  // Assert Count
  else if (index.column() == Columns::Asserts)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return QString("%1").arg(TestResult.m_iTestAsserts);
      }
      case Qt::BackgroundRole:
      {
        QPalette palette = QApplication::palette();
        return palette.alternateBase().color();
      }
      case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }

      default:
        return QVariant();
    }
  }
  // Progress
  else if (index.column() == Columns::Progress)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        if (!testUnavailableReason.empty())
        {
          return QString("Test not available: %1").arg(testUnavailableReason.c_str());
        }
        else if (bTestEnabled && bParentEnabled)
        {
          if (bIsSubTest)
          {
            if (TestResult.m_bExecuted)
            {
              return (TestResult.m_bSuccess) ? QString("Passed") : QString("Failed");
            }
            else
            {
              if (!TestResult.m_sCustomStatus.empty())
              {
                return QString(TestResult.m_sCustomStatus.c_str());
              }

              return QString("Pending");
            }
          }
          else
          {
            // Count sub-test status

            const xiiUInt32 iEnabled   = m_pTestFramework->GetSubTestEnabledCount(pEntry->GetTestIndex());
            const xiiUInt32 iExecuted  = m_pResult->GetSubTestCount(pEntry->GetTestIndex(), xiiTestResultQuery::Executed);
            const xiiUInt32 iSucceeded = m_pResult->GetSubTestCount(pEntry->GetTestIndex(), xiiTestResultQuery::Success);

            if (TestResult.m_bExecuted && iExecuted == iEnabled)
            {
              return (TestResult.m_bSuccess && iExecuted == iSucceeded) ? QString("Passed") : QString("Failed");
            }
            else
            {
              return QString("%1 / %2 Executed").arg(iExecuted).arg(iEnabled);
            }
          }
        }
        else
        {
          return QString("Disabled");
        }
      }
      case Qt::BackgroundRole:
      {
        QPalette palette = QApplication::palette();
        return palette.alternateBase().color();
      }
      case Qt::ForegroundRole:
      {
        if (!testUnavailableReason.empty())
        {
          QPalette palette = QApplication::palette();
          return palette.color(QPalette::Disabled, QPalette::Text);
        }
        else if (TestResult.m_bExecuted)
        {
          return TestResult.m_bSuccess ? m_SucessColor : m_FailedColor;
        }
        else if (!TestResult.m_sCustomStatus.empty())
        {
          return m_CustomStatusColor;
        }

        return QVariant();
      }
      case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }

      default:
        return QVariant();
    }
  }

  return QVariant();
}

Qt::ItemFlags xiiQtTestModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  xiiQtTestModelEntry* pEntry = (xiiQtTestModelEntry*)index.internalPointer();
  if (pEntry == &m_Root)
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

QVariant xiiQtTestModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  if (orientation == Qt::Horizontal && iRole == Qt::DisplayRole)
  {
    switch (iSection)
    {
      case Columns::Name:
        return QString("Name");
      case Columns::Status:
        return QString("Status");
      case Columns::Duration:
        return QString("Duration (ms)");
      case Columns::Errors:
        return QString("Errors / Output");
      case Columns::Asserts:
        return QString("Checks");
      case Columns::Progress:
        return QString("Progress");
    }
  }
  return QVariant();
}

QModelIndex xiiQtTestModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (!hasIndex(iRow, iColumn, parent))
    return QModelIndex();

  const xiiQtTestModelEntry* pParent = nullptr;

  if (!parent.isValid())
    pParent = &m_Root;
  else
    pParent = static_cast<xiiQtTestModelEntry*>(parent.internalPointer());

  xiiQtTestModelEntry* pEntry = pParent->GetSubEntry(iRow);
  return pEntry ? createIndex(iRow, iColumn, pEntry) : QModelIndex();
}

QModelIndex xiiQtTestModel::parent(const QModelIndex& index) const
{
  if (!index.isValid())
    return QModelIndex();

  xiiQtTestModelEntry* pChild  = static_cast<xiiQtTestModelEntry*>(index.internalPointer());
  xiiQtTestModelEntry* pParent = pChild->GetParentEntry();

  if (pParent == &m_Root)
    return QModelIndex();

  return createIndex(pParent->GetIndexInParent(), 0, pParent);
}

int xiiQtTestModel::rowCount(const QModelIndex& parent) const
{
  if (parent.column() > 0)
    return 0;

  const xiiQtTestModelEntry* pParent = nullptr;

  if (!parent.isValid())
    pParent = &m_Root;
  else
    pParent = static_cast<xiiQtTestModelEntry*>(parent.internalPointer());

  return pParent->GetNumSubEntries();
}

int xiiQtTestModel::columnCount(const QModelIndex& parent) const
{
  return Columns::ColumnCount;
}

bool xiiQtTestModel::setData(const QModelIndex& index, const QVariant& value, int iRole)
{
  xiiQtTestModelEntry* pEntry = static_cast<xiiQtTestModelEntry*>(index.internalPointer());
  if (pEntry == nullptr || index.column() != Columns::Name || iRole != Qt::CheckStateRole)
    return false;

  if (pEntry->GetNodeType() == xiiQtTestModelEntry::TestNode)
  {
    m_pTestFramework->SetTestEnabled(pEntry->GetTestIndex(), value.toBool());
    TestDataChanged(pEntry->GetIndexInParent(), -1);

    // if a test gets enabled in the UI, and all sub-tests are currently disabled,
    // enable all sub-tests as well
    // if some set of sub-tests is already enabled and some are disabled,
    // do not mess with the user's choice of enabled tests
    bool bEnableSubTests = value.toBool();
    for (xiiUInt32 subIdx = 0; subIdx < pEntry->GetNumSubEntries(); ++subIdx)
    {
      if (m_pTestFramework->IsSubTestEnabled(pEntry->GetTestIndex(), subIdx))
      {
        bEnableSubTests = false;
        break;
      }
    }

    if (bEnableSubTests)
    {
      for (xiiUInt32 subIdx = 0; subIdx < pEntry->GetNumSubEntries(); ++subIdx)
      {
        m_pTestFramework->SetSubTestEnabled(pEntry->GetTestIndex(), subIdx, true);
        TestDataChanged(pEntry->GetIndexInParent(), subIdx);
      }
    }
  }
  else
  {
    m_pTestFramework->SetSubTestEnabled(pEntry->GetTestIndex(), pEntry->GetSubTestIndex(), value.toBool());
    TestDataChanged(pEntry->GetParentEntry()->GetIndexInParent(), pEntry->GetIndexInParent());
  }

  return true;
}


////////////////////////////////////////////////////////////////////////
// xiiQtTestModel public slots
////////////////////////////////////////////////////////////////////////

void xiiQtTestModel::UpdateModel()
{
  m_Root.ClearEntries();
  if (m_pResult == nullptr)
    return;

  const xiiUInt32 uiTestCount = m_pResult->GetTestCount();
  for (xiiUInt32 uiTestIndex = 0; uiTestIndex < uiTestCount; ++uiTestIndex)
  {
    xiiQtTestModelEntry* pTestModelEntry = new xiiQtTestModelEntry(m_pResult, uiTestIndex);
    m_Root.AddSubEntry(pTestModelEntry);

    const xiiUInt32 uiSubTestCount = m_pResult->GetSubTestCount(uiTestIndex);
    for (xiiUInt32 uiSubTestIndex = 0; uiSubTestIndex < uiSubTestCount; ++uiSubTestIndex)
    {
      xiiQtTestModelEntry* pSubTestModelEntry = new xiiQtTestModelEntry(m_pResult, uiTestIndex, uiSubTestIndex);
      pTestModelEntry->AddSubEntry(pSubTestModelEntry);
    }
  }
  // reset();
}

#endif

XII_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestModel);
