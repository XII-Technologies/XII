#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

enum Columns
{
  Method,
  InputFile,
  ENUM_COUNT
};

xiiQtAssetImportDlg::xiiQtAssetImportDlg(QWidget* pParent, xiiDynamicArray<xiiAssetDocumentGenerator::ImportGroupOptions>& ref_allImports) :
  QDialog(pParent), m_AllImports(ref_allImports)
{
  setupUi(this);

  QStringList headers;
  headers.push_back(QString::fromUtf8("Import Method"));
  headers.push_back(QString::fromUtf8("Input File"));

  QTableWidget* table = AssetTable;
  table->setColumnCount(headers.size());
  table->setRowCount(m_AllImports.GetCount());
  table->setHorizontalHeaderLabels(headers);

  {
    xiiQtScopedBlockSignals _1(table);

    for (xiiUInt32 i = 0; i < m_AllImports.GetCount(); ++i)
    {
      InitRow(i);
    }
  }

  table->horizontalHeader()->setSectionResizeMode(Columns::Method, QHeaderView::ResizeMode::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(Columns::InputFile, QHeaderView::ResizeMode::Stretch);
}

xiiQtAssetImportDlg::~xiiQtAssetImportDlg() = default;

void xiiQtAssetImportDlg::InitRow(xiiUInt32 uiRow)
{
  QTableWidget* table = AssetTable;
  const auto&   data2 = m_AllImports[uiRow];

  table->setItem(uiRow, Columns::InputFile, new QTableWidgetItem(data2.m_sInputFileRelative.GetData()));
  table->item(uiRow, Columns::InputFile)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);

  QComboBox* pCombo = new QComboBox();
  table->setCellWidget(uiRow, Columns::Method, pCombo);

  pCombo->addItem(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/NoEntry.svg"), "No Import");
  for (const auto& option : data2.m_ImportOptions)
  {
    QIcon icon = xiiQtUiServices::GetSingleton()->GetCachedIconResource(option.m_sIcon);
    pCombo->addItem(icon, xiiMakeQString(xiiTranslate(option.m_sName)));
  }

  pCombo->setProperty("row", uiRow);
  pCombo->setCurrentIndex(data2.m_iSelectedOption + 1);
  connect(pCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(SelectedOptionChanged(int)));
}

void xiiQtAssetImportDlg::SelectedOptionChanged(int index)
{
  QComboBox*      pCombo = qobject_cast<QComboBox*>(sender());
  const xiiUInt32 uiRow  = pCombo->property("row").toInt();

  m_AllImports[uiRow].m_iSelectedOption = index - 1;
}

void xiiQtAssetImportDlg::on_ButtonImport_clicked()
{
  for (auto& data : m_AllImports)
  {
    if (data.m_iSelectedOption < 0)
      continue;

    XII_LOG_BLOCK("Asset Import", data.m_sInputFileRelative);

    const auto& option = data.m_ImportOptions[data.m_iSelectedOption];

    option.m_pGenerator->Import(data.m_sInputFileAbsolute, option.m_sName, false).LogFailure();
  }

  accept();
}
