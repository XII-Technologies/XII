/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCP/MCPPCH.h>

#ifdef XII_USE_QT

#  include <MCP/Application.h>
#  include <MCPPlugin/Json.h>

#  include <GuiFoundation/Style/DarkEditorStyle.moc.h>
#  include <MCP/Gateway/GatewayThread.h>
#  include <MCP/Panels/ArgumentForm.h>
#  include <MCP/Panels/SettingsPanel.h>

#  include <QApplication>
#  include <QCheckBox>
#  include <QClipboard>
#  include <QComboBox>
#  include <QFormLayout>
#  include <QHeaderView>
#  include <QLabel>
#  include <QLineEdit>
#  include <QMainWindow>
#  include <QPlainTextEdit>
#  include <QPushButton>
#  include <QScrollArea>
#  include <QSettings>
#  include <QSplitter>
#  include <QTabWidget>
#  include <QTableWidget>
#  include <QTimer>
#  include <QVBoxLayout>

namespace
{
  QString ToQt(xiiStringView sValue)
  {
    return QString::fromUtf8(sValue.GetStartPointer(), static_cast<qsizetype>(sValue.GetElementCount()));
  }

  const xiiVariantArray& GetArray(const xiiVariantDictionary& snapshot, xiiStringView sName)
  {
    static const xiiVariantArray Empty;
    const auto*                  pValue = snapshot.GetValue(sName);
    return pValue && pValue->IsA<xiiVariantArray>() ? pValue->Get<xiiVariantArray>() : Empty;
  }

  QTableWidget* MakeTable(const QStringList& headings)
  {
    auto* pTable = new QTableWidget(0, static_cast<int>(headings.size()));
    pTable->setHorizontalHeaderLabels(headings);
    pTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    pTable->verticalHeader()->hide();
    return pTable;
  }

  void FillTable(QTableWidget& ref_table, const xiiVariantArray& values, std::initializer_list<const char*> fields)
  {
    ref_table.setRowCount(static_cast<int>(values.GetCount()));
    for (xiiUInt32 uiRow = 0; uiRow < values.GetCount(); ++uiRow)
    {
      const auto& row     = values[uiRow].Get<xiiVariantDictionary>();
      int         iColumn = 0;
      for (const auto* szField : fields)
      {
        const auto* pValue = row.GetValue(szField);
        auto*       pCell  = ref_table.item(static_cast<int>(uiRow), iColumn);
        if (pCell == nullptr)
        {
          pCell = new QTableWidgetItem();
          ref_table.setItem(static_cast<int>(uiRow), iColumn, pCell);
        }
        pCell->setText(pValue != nullptr ? ToQt(pValue->ConvertTo<xiiString>()) : QString());
        ++iColumn;
      }
    }
  }
} // namespace

int RunMCPWindow(const xiiMCPGatewayConfiguration& configuration)
{
  int          iArgumentCount  = 1;
  char         szApplication[] = "xiiMCP";
  char*        pArguments[]    = {szApplication, nullptr};
  QApplication application(iArgumentCount, pArguments);
  QApplication::setStyle(new xiiQtDarkEditorStyle());
  QSettings                  settings("XII", "xiiMCP");
  auto*                      pSettingsPanel = new xiiMCPSettingsPanel(configuration, settings, configuration.m_bRestorePreferences);
  xiiMCPGatewayConfiguration current        = configuration;
  pSettingsPanel->ReadConfiguration(current).IgnoreResult();
  xiiMCPGatewayThread gateway(current);

  QMainWindow window;
  window.setWindowTitle("XII MCP");
  window.resize(1024, 720);
  window.restoreGeometry(settings.value("geometry").toByteArray());
  auto* pTabs = new QTabWidget(&window);
  window.setCentralWidget(pTabs);
  auto* pOverview = new QWidget();
  auto* pStatus   = new QLabel("Starting gateway...");
  pStatus->setTextFormat(Qt::PlainText);
  auto* pOverviewLayout = new QVBoxLayout(pOverview);
  pOverviewLayout->addWidget(pStatus);
  pOverviewLayout->addStretch();
  pTabs->addTab(pOverview, "Overview");

  auto* pInstances = MakeTable({"Application", "Local endpoint", "Process ID", "Instance", "Connected", "Capabilities", "Status"});
  pTabs->addTab(pInstances, "Instances");

  auto* pCapabilities     = new QWidget();
  auto* pCapabilityLayout = new QVBoxLayout(pCapabilities);
  auto* pFilter           = new QLineEdit(settings.value("filter").toString());
  pFilter->setPlaceholderText("Filter by capability name or URI");
  auto* pKind = new QComboBox();
  pKind->addItems({"Tools", "Resources", "Prompts", "Resource templates"});
  auto* pCapability = new QComboBox();
  auto* pSchema     = new QPlainTextEdit();
  pSchema->setReadOnly(true);
  pSchema->setPlaceholderText("Capability descriptor");
  auto* pInput = new QPlainTextEdit("{}");
  pInput->setPlaceholderText("JSON arguments");
  auto* pArgumentForm = new xiiMCPArgumentForm();
  auto* pFormScroll   = new QScrollArea();
  pFormScroll->setWidgetResizable(true);
  pFormScroll->setWidget(pArgumentForm);
  auto* pRawArguments = new QCheckBox("Advanced JSON arguments");
  auto* pResourceUri  = new QLineEdit();
  pResourceUri->setPlaceholderText("Enter the concrete resource URI");
  pResourceUri->hide();
  pInput->hide();
  auto* pRun    = new QPushButton("Invoke");
  auto* pResult = new QPlainTextEdit();
  pResult->setReadOnly(true);
  pResult->setPlaceholderText("Operation result");
  pCapabilityLayout->addWidget(pFilter);
  pCapabilityLayout->addWidget(pKind);
  pCapabilityLayout->addWidget(pCapability);
  pCapabilityLayout->addWidget(pResourceUri);
  pCapabilityLayout->addWidget(pRawArguments);
  auto* pEditors = new QSplitter(Qt::Horizontal);
  pEditors->addWidget(pSchema);
  pEditors->addWidget(pFormScroll);
  pEditors->addWidget(pInput);
  pEditors->addWidget(pResult);
  pEditors->restoreState(settings.value("capabilitySplitter").toByteArray());
  pCapabilityLayout->addWidget(pEditors, 1);
  pCapabilityLayout->addWidget(pRun);
  auto* pCancel = new QPushButton("Cancel queued requests");
  pCapabilityLayout->addWidget(pCancel);
  QObject::connect(pCancel, &QPushButton::clicked, [&] { gateway.CancelQueued(); });
  pTabs->addTab(pCapabilities, "Capabilities");

  auto* pActivity = MakeTable({"Capability", "Target", "Elapsed (ms)", "Result"});
  pTabs->addTab(pActivity, "Activity");

  auto* pConnections      = new QWidget();
  auto* pConnectionLayout = new QVBoxLayout(pConnections);
  auto* pClientEndpoint   = new QLineEdit();
  pClientEndpoint->setReadOnly(true);
  pClientEndpoint->setPlaceholderText("Start the HTTP listener to connect a client.");
  auto* pCopy = new QPushButton("Copy VS Code HTTP configuration");
  pCopy->setEnabled(false);
  auto* pClients = MakeTable({"Client", "State", "Idle (s)"});
  pConnectionLayout->addWidget(pClientEndpoint);
  pConnectionLayout->addWidget(pCopy);
  pConnectionLayout->addWidget(pClients, 1);
  pTabs->addTab(pConnections, "Clients");
  QObject::connect(pCopy, &QPushButton::clicked, [&] {
    xiiVariantDictionary server;
    server["type"] = xiiString("http");
    server["url"]  = xiiString(pClientEndpoint->text().toUtf8().constData());
    if (current.m_Http.m_sBearerToken.IsEmpty() == false)
    {
      xiiVariantDictionary headers;
      headers["Authorization"] = xiiString("Bearer ${input:xiiMcpToken}");
      server["headers"]        = headers;
    }
    xiiVariantDictionary servers;
    servers["xii"] = server;
    xiiVariantDictionary config;
    config["servers"] = servers;
    if (current.m_Http.m_sBearerToken.IsEmpty() == false)
    {
      xiiVariantDictionary input;
      input["id"]          = xiiString("xiiMcpToken");
      input["type"]        = xiiString("promptString");
      input["description"] = xiiString("XII MCP bearer token");
      input["password"]    = true;
      xiiVariantArray inputs;
      inputs.PushBack(input);
      config["inputs"] = inputs;
    }
    xiiStringBuilder sConfig;
    if (xiiMCPJson::WriteIndented(config, sConfig).Succeeded())
    {
      QApplication::clipboard()->setText(ToQt(sConfig));
    }
  });

  auto* pOptions        = new QWidget();
  auto* pOptionsLayout  = new QVBoxLayout(pOptions);
  auto* pApply          = new QPushButton("Apply gateway settings");
  auto* pSettingsStatus = new QLabel();
  pSettingsStatus->setTextFormat(Qt::PlainText);
  pSettingsStatus->setWordWrap(true);
  pOptionsLayout->addWidget(pSettingsPanel);
  pOptionsLayout->addWidget(pSettingsStatus);
  pOptionsLayout->addWidget(pApply);
  pOptionsLayout->addStretch();
  pTabs->addTab(pOptions, "Settings");

  xiiMCPGatewayConfiguration pending;
  bool                       bAwaitingConfiguration  = false;
  xiiUInt64                  uiConfigurationRevision = 0;
  QObject::connect(pApply, &QPushButton::clicked, [&] {
    const auto status = pSettingsPanel->ReadConfiguration(pending);
    if (status.Failed())
    {
      pSettingsStatus->setText(ToQt(status.GetMessageString()));
    }
    else if (gateway.ApplyConfiguration(pending))
    {
      bAwaitingConfiguration = true;
      pSettingsPanel->setEnabled(false);
      pApply->setEnabled(false);
      pSettingsStatus->setText("Applying gateway settings...");
    }
    else
    {
      pSettingsStatus->setText("Wait for queued requests or cancel them before applying settings.");
    }
  });

  QString sPreviousDescriptor;
  auto    updateForm = [&] {
    const int  iKind      = pKind->currentIndex();
    const bool bArguments = iKind == 0 || iKind == 2;
    pRawArguments->setVisible(bArguments);
    pResourceUri->setVisible(iKind == 3);
    pInput->setVisible(bArguments && (pRawArguments->isChecked() || iKind == 2));
    pFormScroll->setVisible(iKind == 0 && !pRawArguments->isChecked());
    const auto sDescriptor = pCapability->currentData().toString();
    if (sDescriptor == sPreviousDescriptor)
    {
      return;
    }
    sPreviousDescriptor = sDescriptor;
    pSchema->setPlainText(sDescriptor);
    const auto descriptor = sDescriptor.toUtf8();
    xiiVariant value;
    if (xiiMCPJson::Parse(descriptor.constData(), value).Succeeded() && value.IsA<xiiVariantDictionary>())
    {
      if (const auto* pSchemaData = xiiMCPJson::GetObject(value.Get<xiiVariantDictionary>(), "inputSchema"))
      {
        pArgumentForm->SetSchema(*pSchemaData);
        return;
      }
    }
    pArgumentForm->SetSchema({});
  };
  QObject::connect(pCapability, &QComboBox::currentIndexChanged, updateForm);
  QObject::connect(pRawArguments, &QCheckBox::toggled, updateForm);
  xiiUInt64 uiNextRequest = 1;
  QObject::connect(pRun, &QPushButton::clicked, [&] {
    if (pCapability->currentIndex() < 0)
    {
      return;
    }
    const int            iKind     = pKind->currentIndex();
    const bool           bResource = iKind == 1 || iKind == 3;
    xiiVariantDictionary arguments;
    if (iKind == 0 && !pRawArguments->isChecked())
    {
      const auto status = pArgumentForm->ReadArguments(arguments);
      if (status.Failed())
      {
        pResult->appendPlainText(ToQt(status.GetMessageString()));
        return;
      }
    }
    else if (!bResource)
    {
      const auto rawArguments = pInput->toPlainText().toUtf8();
      xiiVariant json;
      if (xiiMCPJson::Parse(xiiStringView(rawArguments.constData(), static_cast<xiiUInt32>(rawArguments.size())), json).Failed() ||
          !json.IsA<xiiVariantDictionary>())
      {
        pResult->appendPlainText("Enter a JSON object for arguments.");
        return;
      }
      arguments = json.Get<xiiVariantDictionary>();
    }
    const auto key = (iKind == 3 ? pResourceUri->text().trimmed() : pCapability->currentText()).toUtf8();
    if (key.isEmpty())
    {
      pResult->appendPlainText("Enter the concrete resource URI.");
      return;
    }
    xiiVariantDictionary params;
    params[bResource ? "uri" : "name"] = xiiString(xiiStringView(key.constData(), static_cast<xiiUInt32>(key.size())));
    if (!bResource)
    {
      params["arguments"] = arguments;
    }
    xiiVariantDictionary request;
    request["jsonrpc"] = xiiString("2.0");
    request["id"]      = uiNextRequest++;
    request["method"]  = xiiString(pKind->currentIndex() == 0 ? "tools/call" : bResource ? "resources/read" :
                                                                                           "prompts/get");
    request["params"]  = std::move(params);
    xiiStringBuilder sRequest;
    if (xiiMCPJson::Write(request, sRequest).Failed() || !gateway.Submit(sRequest))
    {
      pResult->appendPlainText("The request could not be queued.");
    }
    else
    {
      pResult->appendPlainText(QString("Queued request %1").arg(uiNextRequest - 1));
    }
  });

  QTimer timer;
  pResult->setMaximumBlockCount(4096);
  QObject::connect(&timer, &QTimer::timeout, [&] {
    xiiVariantDictionary snapshot;
    xiiDeque<xiiString>  replies;
    gateway.Read(snapshot, replies);
    for (const auto& sReply : replies)
    {
      xiiVariant       value;
      xiiStringBuilder sFormatted;
      const bool       bFormatted = xiiMCPJson::Parse(sReply, value).Succeeded() && xiiMCPJson::WriteIndented(value, sFormatted).Succeeded();
      pResult->appendPlainText(ToQt(bFormatted ? sFormatted.GetView() : sReply.GetView()));
    }
    if (snapshot.IsEmpty())
    {
      return;
    }
    const auto sError     = xiiMCPJson::GetString(snapshot, "error");
    const auto uiRevision = snapshot["configurationRevision"].ConvertTo<xiiUInt64>();
    if (bAwaitingConfiguration && uiConfigurationRevision != uiRevision)
    {
      bAwaitingConfiguration = false;
      pSettingsPanel->setEnabled(true);
      pApply->setEnabled(true);
      if (sError.IsEmpty())
      {
        current = pending;
        pSettingsPanel->SavePreferences(settings);
        pSettingsStatus->setText("Gateway settings applied.");
      }
      else
      {
        pSettingsStatus->setText(ToQt(sError));
      }
    }
    uiConfigurationRevision = uiRevision;
    const auto& targets   = GetArray(snapshot, "targets");
    const auto& activity  = GetArray(snapshot, "activity");
    const auto& tools     = GetArray(snapshot, "tools");
    const auto  sEndpoint = xiiMCPJson::GetString(snapshot, "endpoint");
    pClientEndpoint->setText(sEndpoint.IsEmpty() ? QString() : QString("http://%1/mcp").arg(ToQt(sEndpoint)));
    pCopy->setEnabled(!sEndpoint.IsEmpty());
    pStatus->setText(QString("%1\n%2 known instances | %3 tools | %4 HTTP sessions\n%5")
                       .arg(sEndpoint.IsEmpty() ? QString("HTTP listener stopped") : pClientEndpoint->text())
                       .arg(targets.GetCount())
                       .arg(tools.GetCount())
                       .arg(snapshot["sessions"].ConvertTo<xiiUInt32>())
                       .arg(ToQt(sError)));
    FillTable(*pClients, GetArray(snapshot, "clients"), {"name", "state", "idleSeconds"});
    FillTable(*pInstances, targets, {"application", "endpoint", "processId", "instance", "connected", "capabilityCount", "status"});
    FillTable(*pActivity, activity, {"capability", "target", "elapsedMs", "status"});

    constexpr const char* Categories[] = {"tools", "resources", "prompts", "resourceTemplates"};
    constexpr const char* KeyFields[]  = {"name", "uri", "name", "uriTemplate"};
    const int             iKind        = pKind->currentIndex();
    const auto&           entries      = GetArray(snapshot, Categories[iKind]);
    const auto            sSelected    = pCapability->currentText();
    pCapability->blockSignals(true);
    pCapability->clear();
    for (const auto& entry : entries)
    {
      const auto& descriptor = entry.Get<xiiVariantDictionary>();
      const auto  sName      = ToQt(xiiMCPJson::GetString(descriptor, KeyFields[iKind]));
      if (sName.contains(pFilter->text(), Qt::CaseInsensitive))
      {
        xiiStringBuilder sDescriptor;
        xiiMCPJson::WriteIndented(entry, sDescriptor).IgnoreResult();
        pCapability->addItem(sName, ToQt(sDescriptor));
      }
    }
    const int iPrevious = pCapability->findText(sSelected);
    if (iPrevious >= 0)
    {
      pCapability->setCurrentIndex(iPrevious);
    }
    pCapability->blockSignals(false);
    updateForm();
    pRun->setEnabled(pCapability->currentIndex() >= 0);
  });
  timer.start(250);
  window.show();
  const int iExitCode = application.exec();
  settings.setValue("geometry", window.saveGeometry());
  settings.setValue("capabilitySplitter", pEditors->saveState());
  settings.setValue("filter", pFilter->text());
  return iExitCode;
}

#endif
