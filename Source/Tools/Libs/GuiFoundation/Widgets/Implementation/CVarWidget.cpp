#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

xiiQtCVarWidget::xiiQtCVarWidget(QWidget* parent) :
  QWidget(parent)
{
  setupUi(this);

  m_pItemModel = new xiiQtCVarModel(this);

  m_pFilterModel = new QSortFilterProxyModel(this);
  m_pFilterModel->setSourceModel(m_pItemModel);

  m_pItemDelegate           = new xiiQtCVarItemDelegate(this);
  m_pItemDelegate->m_pModel = m_pItemModel;

  CVarsView->setModel(m_pFilterModel);
  CVarsView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  CVarsView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  CVarsView->setHeaderHidden(false);
  CVarsView->setEditTriggers(QAbstractItemView::EditTrigger::CurrentChanged | QAbstractItemView::EditTrigger::SelectedClicked);
  CVarsView->setItemDelegateForColumn(1, m_pItemDelegate);

  connect(SearchWidget, &xiiQtSearchWidget::textChanged, this, &xiiQtCVarWidget::SearchTextChanged);
  connect(ConsoleInput, &xiiQtSearchWidget::enterPressed, this, &xiiQtCVarWidget::ConsoleEnterPressed);
  connect(ConsoleInput, &xiiQtSearchWidget::specialKeyPressed, this, &xiiQtCVarWidget::ConsoleSpecialKeyPressed);

  m_Console.Events().AddEventHandler(xiiMakeDelegate(&xiiQtCVarWidget::OnConsoleEvent, this));

  ConsoleInput->setPlaceholderText("> TAB to auto-complete");
}

xiiQtCVarWidget::~xiiQtCVarWidget() {}

void xiiQtCVarWidget::Clear()
{
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  clearFocus();
  m_pItemModel->BeginResetModel();
  m_pItemModel->EndResetModel();

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

void xiiQtCVarWidget::RebuildCVarUI(const xiiMap<xiiString, xiiCVarWidgetData>& cvars)
{
  // for now update and rebuild are the same
  UpdateCVarUI(cvars);
}

void xiiQtCVarWidget::UpdateCVarUI(const xiiMap<xiiString, xiiCVarWidgetData>& cvars)
{
  int row = 0;

  m_pItemModel->BeginResetModel();

  for (auto it = cvars.GetIterator(); it.IsValid(); ++it, ++row)
  {
    it.Value().m_bNewEntry = false;

    auto item            = m_pItemModel->CreateEntry(it.Key().GetData());
    item->m_sDescription = it.Value().m_sDescription;
    item->m_sPlugin      = it.Value().m_sPlugin;

    switch (it.Value().m_uiType)
    {
      case xiiCVarType::Bool:
        item->m_Value = it.Value().m_bValue;
        break;
      case xiiCVarType::Float:
        item->m_Value = it.Value().m_fValue;
        break;
      case xiiCVarType::Int:
        item->m_Value = it.Value().m_iValue;
        break;
      case xiiCVarType::String:
        item->m_Value = it.Value().m_sValue;
        break;
    }
  }

  m_pItemModel->EndResetModel();

  CVarsView->expandAll();
  CVarsView->resizeColumnToContents(0);
  CVarsView->resizeColumnToContents(1);
}

void xiiQtCVarWidget::AddConsoleStrings(const xiiStringBuilder& encoded)
{
  xiiHybridArray<xiiStringView, 64> lines;
  encoded.Split(false, lines, ";;");

  xiiStringBuilder tmp;

  for (auto l : lines)
  {
    l.Shrink(4, 0); // skip the line type number at the front (for now)

    if (l.StartsWith("<"))
    {
      l.Shrink(1, 0);
      ConsoleInput->setText(l.GetData(tmp));
    }
    else
    {
      GetConsole().AddConsoleString(l);
    }
  }
}

void xiiQtCVarWidget::SearchTextChanged(const QString& text)
{
  m_pFilterModel->setRecursiveFilteringEnabled(true);
  m_pFilterModel->setFilterRole(Qt::UserRole);
  m_pFilterModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

  QRegularExpression e;

  QString st = "(?=.*" + text + ".*)";
  st.replace(" ", ".*)(?=.*");

  e.setPattern(st);
  e.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
  m_pFilterModel->setFilterRegularExpression(e);
  CVarsView->expandAll();
}

void xiiQtCVarWidget::ConsoleEnterPressed()
{
  m_Console.AddToInputHistory(ConsoleInput->text().toUtf8().data());
  m_Console.ExecuteCommand(ConsoleInput->text().toUtf8().data());
  ConsoleInput->setText("");
}

void xiiQtCVarWidget::ConsoleSpecialKeyPressed(Qt::Key key)
{
  if (key == Qt::Key_Tab)
  {
    xiiStringBuilder input = ConsoleInput->text().toUtf8().data();

    if (m_Console.AutoComplete(input))
    {
      ConsoleInput->setText(input.GetData());
    }
  }
  if (key == Qt::Key_Up)
  {
    xiiStringBuilder input = ConsoleInput->text().toUtf8().data();
    m_Console.RetrieveInputHistory(1, input);
    ConsoleInput->setText(input.GetData());
  }
  if (key == Qt::Key_Down)
  {
    xiiStringBuilder input = ConsoleInput->text().toUtf8().data();
    m_Console.RetrieveInputHistory(-1, input);
    ConsoleInput->setText(input.GetData());
  }
  if (key == Qt::Key_F2)
  {
    if (m_Console.GetInputHistory().GetCount() >= 1)
    {
      m_Console.ExecuteCommand(m_Console.GetInputHistory()[0]);
    }
  }
  if (key == Qt::Key_F3)
  {
    if (m_Console.GetInputHistory().GetCount() >= 2)
    {
      m_Console.ExecuteCommand(m_Console.GetInputHistory()[1]);
    }
  }
}

void xiiQtCVarWidget::OnConsoleEvent(const xiiConsoleEvent& e)
{
  if (e.m_Type == xiiConsoleEvent::Type::OutputLineAdded)
  {
    QString t = ConsoleOutput->toPlainText();
    t += e.m_AddedpConsoleString->m_sText;
    t += "\n";
    ConsoleOutput->setPlainText(t);
    ConsoleOutput->verticalScrollBar()->setValue(ConsoleOutput->verticalScrollBar()->maximum());
  }
}

xiiQtCVarModel::xiiQtCVarModel(xiiQtCVarWidget* owner) :
  QAbstractItemModel(owner)
{
  m_pOwner = owner;
}

xiiQtCVarModel::~xiiQtCVarModel() = default;

void xiiQtCVarModel::BeginResetModel()
{
  beginResetModel();
  m_RootEntries.Clear();
  m_AllEntries.Clear();
}

void xiiQtCVarModel::EndResetModel()
{
  endResetModel();
}

QVariant xiiQtCVarModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
  if (role == Qt::DisplayRole)
  {
    switch (section)
    {
      case 0:
        return "Name";

      case 1:
        return "Value";

      case 2:
        return "Description";

      case 3:
        return "Plugin";
    }
  }

  return QAbstractItemModel::headerData(section, orientation, role);
}

bool xiiQtCVarModel::setData(const QModelIndex& index, const QVariant& value, int role /*= Qt::EditRole*/)
{
  if (index.column() == 1 && role == Qt::EditRole)
  {
    xiiQtCVarModel::Entry* e = reinterpret_cast<xiiQtCVarModel::Entry*>(index.internalId());

    switch (e->m_Value.GetType())
    {
      case xiiVariantType::Bool:
        e->m_Value = value.toBool();
        m_pOwner->onBoolChanged(e->m_sFullName, value.toBool());
        break;
      case xiiVariantType::Int32:
        e->m_Value = value.toInt();
        m_pOwner->onIntChanged(e->m_sFullName, value.toInt());
        break;
      case xiiVariantType::Float:
        e->m_Value = value.toFloat();
        m_pOwner->onFloatChanged(e->m_sFullName, value.toFloat());
        break;
      case xiiVariantType::String:
        e->m_Value = value.toString().toUtf8().data();
        m_pOwner->onStringChanged(e->m_sFullName, value.toString().toUtf8().data());
        break;
      default:
        break;
    }
  }

  return QAbstractItemModel::setData(index, value, role);
}

QVariant xiiQtCVarModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  xiiQtCVarModel::Entry* e = reinterpret_cast<xiiQtCVarModel::Entry*>(index.internalId());

  if (role == Qt::UserRole)
  {
    return e->m_sFullName.GetData();
  }

  if (role == Qt::DisplayRole)
  {
    switch (index.column())
    {
      case 0:
        return e->m_sDisplayString;

      case 1:
        return e->m_Value.ConvertTo<xiiString>().GetData();

      case 2:
        return e->m_sDescription;
    }
  }

  if (role == Qt::DecorationRole && index.column() == 0)
  {
    if (e->m_Value.IsValid())
    {
      return xiiQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/CVar.png");
    }
  }

  if (role == Qt::ToolTipRole)
  {
    if (e->m_Value.IsValid())
    {
      if (index.column() == 0)
      {
        return QString(e->m_sFullName) + " | " + e->m_sPlugin;
      }

      if (index.column() == 2)
      {
        return e->m_sDescription;
      }
    }
  }

  if (role == Qt::EditRole && index.column() == 1)
  {
    switch (e->m_Value.GetType())
    {
      case xiiVariantType::Bool:
        return e->m_Value.Get<bool>();
      case xiiVariantType::Int32:
        return e->m_Value.Get<xiiInt32>();
      case xiiVariantType::Float:
        return e->m_Value.ConvertTo<double>();
      case xiiVariantType::String:
        return e->m_Value.Get<xiiString>().GetData();
      default:
        break;
    }
  }
  return QVariant();
}

Qt::ItemFlags xiiQtCVarModel::flags(const QModelIndex& index) const
{
  if (index.column() == 1)
  {
    xiiQtCVarModel::Entry* e = reinterpret_cast<xiiQtCVarModel::Entry*>(index.internalId());

    if (e->m_Value.IsValid())
    {
      return Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsEditable;
    }
  }

  return Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEnabled;
}

QModelIndex xiiQtCVarModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (parent.isValid())
  {
    xiiQtCVarModel::Entry* e = reinterpret_cast<xiiQtCVarModel::Entry*>(parent.internalId());
    return createIndex(row, column, const_cast<xiiQtCVarModel::Entry*>(e->m_ChildEntries[row]));
  }
  else
  {
    return createIndex(row, column, const_cast<xiiQtCVarModel::Entry*>(m_RootEntries[row]));
  }
}

QModelIndex xiiQtCVarModel::parent(const QModelIndex& index) const
{
  if (!index.isValid())
    return QModelIndex();

  xiiQtCVarModel::Entry* e = reinterpret_cast<xiiQtCVarModel::Entry*>(index.internalId());

  if (e->m_pParentEntry == nullptr)
    return QModelIndex();

  xiiQtCVarModel::Entry* p = e->m_pParentEntry;

  // find the parent entry's row index
  if (p->m_pParentEntry == nullptr)
  {
    // if the parent has no parent itself, it is a root entry and we need to search that array
    for (xiiUInt32 row = 0; row < m_RootEntries.GetCount(); ++row)
    {
      if (m_RootEntries[row] == p)
      {
        return createIndex(row, index.column(), p);
      }
    }
  }
  else
  {
    // if the parent has a parent itself, search that array for the row index
    for (xiiUInt32 row = 0; row < p->m_pParentEntry->m_ChildEntries.GetCount(); ++row)
    {
      if (p->m_pParentEntry->m_ChildEntries[row] == e)
      {
        return createIndex(row, index.column(), p);
      }
    }
  }

  return QModelIndex();
}

int xiiQtCVarModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (parent.isValid())
  {
    xiiQtCVarModel::Entry* e = reinterpret_cast<xiiQtCVarModel::Entry*>(parent.internalId());

    return (int)e->m_ChildEntries.GetCount();
  }
  else
  {
    return (int)m_RootEntries.GetCount();
  }
}

int xiiQtCVarModel::columnCount(const QModelIndex& index /*= QModelIndex()*/) const
{
  return 3;
}

xiiQtCVarModel::Entry* xiiQtCVarModel::CreateEntry(const char* name)
{
  xiiStringBuilder tmp = name;
  xiiStringBuilder tmp2;

  xiiHybridArray<xiiStringView, 8> pieces;
  tmp.Split(false, pieces, ".", "_");

  xiiDynamicArray<Entry*>* vals        = &m_RootEntries;
  Entry*                   parentEntry = nullptr;

  for (xiiUInt32 p = 0; p < pieces.GetCount(); ++p)
  {
    QString piece = pieces[p].GetData(tmp2);
    for (xiiUInt32 v = 0; v < vals->GetCount(); ++v)
    {
      if ((*vals)[v]->m_sDisplayString == piece)
      {
        parentEntry = (*vals)[v];
        vals        = &((*vals)[v]->m_ChildEntries);
        goto found;
      }
    }

    {
      auto& newItem            = m_AllEntries.ExpandAndGetRef();
      newItem.m_sFullName      = name;
      newItem.m_sDisplayString = piece;
      newItem.m_pParentEntry   = parentEntry;

      vals->PushBack(&newItem);

      parentEntry = &newItem;
      vals        = &newItem.m_ChildEntries;
    }
  found:;
  }

  return parentEntry;
}

QWidget* xiiQtCVarItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& idx) const
{
  m_Index                  = static_cast<const QSortFilterProxyModel*>(idx.model())->mapToSource(idx);
  xiiQtCVarModel::Entry* e = reinterpret_cast<xiiQtCVarModel::Entry*>(m_Index.internalPointer());

  if (!e->m_Value.IsValid())
    return nullptr;

  if (e->m_Value.IsA<bool>())
  {
    QComboBox* ret = new QComboBox(parent);
    ret->addItem("true");
    ret->addItem("false");

    connect(ret, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboChanged(int)));
    return ret;
  }

  if (e->m_Value.IsA<xiiInt32>())
  {
    QLineEdit* ret = new QLineEdit(parent);
    ret->setValidator(new QIntValidator(ret));
    return ret;
  }

  if (e->m_Value.IsA<float>())
  {
    QLineEdit* ret = new QLineEdit(parent);
    auto       val = new QDoubleValidator(ret);
    val->setDecimals(3);
    ret->setValidator(val);
    return ret;
  }

  if (e->m_Value.IsA<xiiString>())
  {
    QLineEdit* ret = new QLineEdit(parent);
    return ret;
  }

  return nullptr;
}

void xiiQtCVarItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
  QVariant value = index.model()->data(index, Qt::EditRole);

  if (QLineEdit* pLine = qobject_cast<QLineEdit*>(editor))
  {
    if (value.type() == QVariant::Type::Double)
    {
      double f = value.toDouble();

      pLine->setText(QString("%1").arg(f, 0, (char)103, 3));
    }
    else
    {
      pLine->setText(value.toString());
    }

    pLine->selectAll();
  }

  if (QComboBox* pLine = qobject_cast<QComboBox*>(editor))
  {
    pLine->setCurrentIndex(value.toBool() ? 0 : 1);
    pLine->showPopup();
  }
}

void xiiQtCVarItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  if (QLineEdit* pLine = qobject_cast<QLineEdit*>(editor))
  {
    model->setData(index, pLine->text(), Qt::EditRole);
  }

  if (QComboBox* pLine = qobject_cast<QComboBox*>(editor))
  {
    model->setData(index, pLine->currentText(), Qt::EditRole);
  }
}

void xiiQtCVarItemDelegate::onComboChanged(int)
{
  setModelData(qobject_cast<QWidget*>(sender()), m_pModel, m_Index);
}
