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
#  include <QFrame>
#  include <QGridLayout>
#  include <QHBoxLayout>
#  include <QHeaderView>
#  include <QIcon>
#  include <QLabel>
#  include <QLineEdit>
#  include <QListWidget>
#  include <QMainWindow>
#  include <QPlainTextEdit>
#  include <QPushButton>
#  include <QScrollArea>
#  include <QSettings>
#  include <QSplitter>
#  include <QStackedWidget>
#  include <QStyle>
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

  QFrame* SummaryCard(const QString& label, QLabel*& out_pValue)
  {
    auto* pCard = new QFrame();
    pCard->setProperty("summaryCard", true);
    auto* pLayout = new QVBoxLayout(pCard);
    pLayout->setContentsMargins(14, 12, 14, 12);
    pLayout->addWidget(new QLabel(label));
    out_pValue = new QLabel("-");
    out_pValue->setTextFormat(Qt::PlainText);
    out_pValue->setWordWrap(true);
    QFont font = out_pValue->font();
    font.setPointSize(font.pointSize() + 4);
    font.setBold(true);
    out_pValue->setFont(font);
    pLayout->addWidget(out_pValue);
    return pCard;
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
  QApplication::setWindowIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/MCP.svg")));
  QSettings                  settings("XII", "xiiMCP");
  auto*                      pSettingsPanel = new xiiMCPSettingsPanel(configuration, settings, configuration.m_bRestorePreferences);
  xiiMCPGatewayConfiguration current        = configuration;
  pSettingsPanel->ReadConfiguration(current).IgnoreResult();
  xiiMCPGatewayThread gateway(current);

  QMainWindow window;
  window.setWindowTitle("XII MCP");
  window.resize(1024, 720);
  window.restoreGeometry(settings.value("geometry").toByteArray());
  auto* pWorkspace       = new QWidget();
  auto* pWorkspaceLayout = new QHBoxLayout(pWorkspace);
  pWorkspaceLayout->setContentsMargins(12, 12, 12, 12);
  pWorkspaceLayout->setSpacing(12);
  pWorkspace->setStyleSheet("QFrame[summaryCard=\"true\"] { background: palette(base); border: 1px solid palette(mid); border-radius: 5px; }"
                            "QListWidget::item { padding: 8px; }");
  auto* pNavigation = new QListWidget();
  pNavigation->setAccessibleName("MCP workspace navigation");
  pNavigation->setMinimumWidth(145);
  pNavigation->setMaximumWidth(185);
  pNavigation->setUniformItemSizes(true);
  auto* pContent       = new QWidget();
  auto* pContentLayout = new QVBoxLayout(pContent);
  pContentLayout->setContentsMargins(0, 0, 0, 0);
  auto* pPageTitle = new QLabel();
  QFont titleFont  = pPageTitle->font();
  titleFont.setPointSize(titleFont.pointSize() + 7);
  titleFont.setBold(true);
  pPageTitle->setFont(titleFont);
  auto* pPages = new QStackedWidget();
  pContentLayout->addWidget(pPageTitle);
  pContentLayout->addWidget(pPages, 1);
  pWorkspaceLayout->addWidget(pNavigation);
  pWorkspaceLayout->addWidget(pContent, 1);
  window.setCentralWidget(pWorkspace);
  auto addPage = [&](QWidget* pPage, const QString& title, QStyle::StandardPixmap icon) {
    pPages->addWidget(pPage);
    return new QListWidgetItem(window.style()->standardIcon(icon), title, pNavigation);
  };

  auto* pOverview       = new QWidget();
  auto* pOverviewLayout = new QVBoxLayout(pOverview);
  pOverviewLayout->setContentsMargins(0, 0, 0, 0);
  auto*   pMetrics       = new QGridLayout();
  QLabel* pListenValue   = nullptr;
  QLabel* pTargetsValue  = nullptr;
  QLabel* pToolsValue    = nullptr;
  QLabel* pSessionsValue = nullptr;
  pMetrics->addWidget(SummaryCard("HTTP listener", pListenValue), 0, 0, 1, 3);
  pMetrics->addWidget(SummaryCard("Connected applications", pTargetsValue), 1, 0);
  pMetrics->addWidget(SummaryCard("Available tools", pToolsValue), 1, 1);
  pMetrics->addWidget(SummaryCard("HTTP sessions", pSessionsValue), 1, 2);
  pOverviewLayout->addLayout(pMetrics);
  auto* pStatus = new QLabel("Starting gateway...");
  pStatus->setTextFormat(Qt::PlainText);
  pStatus->setWordWrap(true);
  pOverviewLayout->addWidget(pStatus);
  auto* pOverviewActions = new QHBoxLayout();
  auto* pGoSettings      = new QPushButton("Manage listener");
  auto* pGoClients       = new QPushButton("Connect a client");
  pOverviewActions->addWidget(pGoSettings);
  pOverviewActions->addWidget(pGoClients);
  pOverviewActions->addStretch();
  pOverviewLayout->addLayout(pOverviewActions);
  auto* pRecent = MakeTable({"Capability", "Target", "Elapsed (ms)", "Result"});
  pOverviewLayout->addWidget(new QLabel("Recent target operations"));
  pOverviewLayout->addWidget(pRecent, 1);
  addPage(pOverview, "Overview", QStyle::SP_ComputerIcon);

  auto* pInstancePage   = new QWidget();
  auto* pInstanceLayout = new QVBoxLayout(pInstancePage);
  pInstanceLayout->setContentsMargins(0, 0, 0, 0);
  auto* pInstanceActions = new QHBoxLayout();
  auto* pManualTarget    = new QLineEdit();
  pManualTarget->setPlaceholderText("Local endpoint name");
  pManualTarget->setAccessibleName("Local endpoint name");
  auto* pConnectTarget    = new QPushButton("Connect");
  auto* pDisconnectTarget = new QPushButton("Disconnect selected");
  auto* pUseTarget        = new QPushButton("Browse selected");
  pInstanceActions->addWidget(pManualTarget, 1);
  pInstanceActions->addWidget(pConnectTarget);
  pInstanceActions->addWidget(pDisconnectTarget);
  pInstanceActions->addWidget(pUseTarget);
  auto* pTargetStatus = new QLabel("Discovered applications appear here when ready.");
  pTargetStatus->setTextFormat(Qt::PlainText);
  auto* pInstances = MakeTable({"Application", "Local endpoint", "Process ID", "Instance", "Connected", "Capabilities", "Status"});
  pInstances->setSelectionMode(QAbstractItemView::SingleSelection);
  pInstanceLayout->addLayout(pInstanceActions);
  pInstanceLayout->addWidget(pTargetStatus);
  pInstanceLayout->addWidget(pInstances, 1);
  addPage(pInstancePage, "Instances", QStyle::SP_DriveNetIcon);
  auto submitTarget = [&](const QString& endpoint, bool bEnabled) {
    const auto name = endpoint.trimmed().toUtf8();
    if (name.isEmpty())
    {
      pTargetStatus->setText("Enter or select a Local endpoint.");
      return;
    }
    if (!gateway.SetTargetEnabled(name.constData(), bEnabled))
    {
      pTargetStatus->setText("The gateway is busy. Retry in a moment.");
    }
  };
  QObject::connect(pConnectTarget, &QPushButton::clicked, [&] { submitTarget(pManualTarget->text(), true); });
  QObject::connect(pDisconnectTarget, &QPushButton::clicked, [&] {
    if (const auto* pEndpoint = pInstances->item(pInstances->currentRow(), 1))
    {
      submitTarget(pEndpoint->text(), false);
    }
  });

  auto* pCapabilities     = new QWidget();
  auto* pCapabilityLayout = new QVBoxLayout(pCapabilities);
  auto* pFilter           = new QLineEdit(settings.value("filter").toString());
  pFilter->setPlaceholderText("Filter by capability name or URI");
  auto* pKind = new QComboBox();
  pKind->addItems({"Tools", "Resources", "Prompts", "Resource templates"});
  auto* pCapability = new QComboBox();
  auto* pSchema     = new QPlainTextEdit();
  pSchema->setReadOnly(true);
  pSchema->setAccessibleName("Capability descriptor");
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
  pResult->setAccessibleName("Operation results");
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
  pEditors->setChildrenCollapsible(false);
  pFormScroll->setMinimumWidth(220);
  pEditors->setStretchFactor(0, 1);
  pEditors->setStretchFactor(1, 1);
  pEditors->setStretchFactor(2, 1);
  pEditors->setStretchFactor(3, 1);
  const auto splitterState = settings.value("workspaceSplitter").toByteArray();
  if (splitterState.isEmpty())
  {
    pEditors->setSizes({290, 300, 300, 320});
  }
  else
  {
    pEditors->restoreState(splitterState);
  }
  pCapabilityLayout->addWidget(pEditors, 1);
  pCapabilityLayout->addWidget(pRun);
  auto* pCancel = new QPushButton("Cancel requests");
  pCapabilityLayout->addWidget(pCancel);
  QObject::connect(pCancel, &QPushButton::clicked, [&] {
    gateway.CancelRequests();
    pResult->appendPlainText("Queued requests cleared. Cancellation requested for any running call.");
  });
  auto* pCapabilityNav = addPage(pCapabilities, "Capabilities", QStyle::SP_FileDialogDetailedView);
  QObject::connect(pUseTarget, &QPushButton::clicked, [&] {
    if (const auto* pEndpoint = pInstances->item(pInstances->currentRow(), 1))
    {
      pFilter->setText(pEndpoint->data(Qt::UserRole).toString());
      pNavigation->setCurrentItem(pCapabilityNav);
    }
  });

  auto* pActivityPage   = new QWidget();
  auto* pActivityLayout = new QVBoxLayout(pActivityPage);
  auto* pActivityFilter = new QLineEdit();
  pActivityFilter->setPlaceholderText("Filter by capability, target or result");
  auto* pActivity = MakeTable({"Capability", "Target", "Elapsed (ms)", "Result", "Details"});
  pActivityLayout->addWidget(pActivityFilter);
  pActivityLayout->addWidget(pActivity, 1);
  addPage(pActivityPage, "Activity", QStyle::SP_FileDialogContentsView);

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
  auto* pClientNav = addPage(pConnections, "Clients", QStyle::SP_MessageBoxInformation);
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
  auto* pSettingsNav = addPage(pOptions, "Settings", QStyle::SP_DialogApplyButton);
  QObject::connect(pNavigation, &QListWidget::currentRowChanged, [&](int iRow) {
    if (iRow >= 0)
    {
      pPages->setCurrentIndex(iRow);
      pPageTitle->setText(pNavigation->currentItem()->text());
    }
  });
  QObject::connect(pGoSettings, &QPushButton::clicked, [&] { pNavigation->setCurrentItem(pSettingsNav); });
  QObject::connect(pGoClients, &QPushButton::clicked, [&] { pNavigation->setCurrentItem(pClientNav); });
  pNavigation->setCurrentRow(0);

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
    const auto& targets     = GetArray(snapshot, "targets");
    const auto& activity    = GetArray(snapshot, "activity");
    const auto& tools       = GetArray(snapshot, "tools");
    const auto  sEndpoint   = xiiMCPJson::GetString(snapshot, "endpoint");
    pClientEndpoint->setText(sEndpoint.IsEmpty() ? QString() : QString("http://%1/mcp").arg(ToQt(sEndpoint)));
    pCopy->setEnabled(!sEndpoint.IsEmpty());
    pListenValue->setText(sEndpoint.IsEmpty() ? QString("Stopped") : pClientEndpoint->text());
    xiiUInt32 uiConnected = 0;
    for (const auto& entry : targets)
    {
      const auto& row        = entry.Get<xiiVariantDictionary>();
      const auto* pConnected = row.GetValue("connected");
      uiConnected += pConnected != nullptr && pConnected->IsA<bool>() && pConnected->Get<bool>() ? 1U : 0U;
    }
    pTargetsValue->setText(QString::number(uiConnected));
    pToolsValue->setText(QString::number(tools.GetCount()));
    pSessionsValue->setText(QString::number(snapshot["sessions"].ConvertTo<xiiUInt32>()));
    pStatus->setVisible(!sError.IsEmpty() || uiConnected == 0);
    pStatus->setText(sError.IsEmpty() ? QString("Launch an MCP-enabled XII application or connect to an instance by Local endpoint.") : ToQt(sError));
    const auto sTargetStatus = xiiMCPJson::GetString(snapshot, "targetStatus");
    if (!sTargetStatus.IsEmpty())
    {
      pTargetStatus->setText(ToQt(sTargetStatus));
    }
    FillTable(*pClients, GetArray(snapshot, "clients"), {"name", "state", "idleSeconds"});
    QString selectedEndpoint;
    if (const auto* pSelected = pInstances->item(pInstances->currentRow(), 1))
    {
      selectedEndpoint = pSelected->text();
    }
    FillTable(*pInstances, targets, {"application", "endpoint", "processId", "instance", "connected", "capabilityCount", "status"});
    int iSelectedRow = -1;
    for (xiiUInt32 uiRow = 0; uiRow < targets.GetCount(); ++uiRow)
    {
      const auto& row       = targets[uiRow].Get<xiiVariantDictionary>();
      auto*       pEndpoint = pInstances->item(static_cast<int>(uiRow), 1);
      pEndpoint->setData(Qt::UserRole, ToQt(xiiMCPJson::GetString(row, "namespace")));
      if (pEndpoint->text() == selectedEndpoint)
      {
        iSelectedRow = static_cast<int>(uiRow);
      }
    }
    if (pInstances->currentRow() != iSelectedRow)
    {
      pInstances->setCurrentCell(iSelectedRow, iSelectedRow < 0 ? -1 : 1);
    }
    pDisconnectTarget->setEnabled(iSelectedRow >= 0);
    pUseTarget->setEnabled(iSelectedRow >= 0 && !pInstances->item(iSelectedRow, 1)->data(Qt::UserRole).toString().isEmpty());
    xiiVariantArray recent;
    xiiVariantArray filtered;
    const auto      filter = pActivityFilter->text();
    for (xiiUInt32 uiIndex = activity.GetCount(); uiIndex > 0; --uiIndex)
    {
      const auto& entry = activity[uiIndex - 1];
      const auto& row   = entry.Get<xiiVariantDictionary>();
      if (recent.GetCount() < 8)
      {
        recent.PushBack(entry);
      }
      if (filter.isEmpty() || ToQt(xiiMCPJson::GetString(row, "capability")).contains(filter, Qt::CaseInsensitive) ||
          ToQt(xiiMCPJson::GetString(row, "target")).contains(filter, Qt::CaseInsensitive) ||
          ToQt(xiiMCPJson::GetString(row, "status")).contains(filter, Qt::CaseInsensitive) ||
          ToQt(xiiMCPJson::GetString(row, "error")).contains(filter, Qt::CaseInsensitive))
      {
        filtered.PushBack(entry);
      }
    }
    FillTable(*pRecent, recent, {"capability", "target", "elapsedMs", "status"});
    FillTable(*pActivity, filtered, {"capability", "target", "elapsedMs", "status", "error"});

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
  settings.setValue("workspaceSplitter", pEditors->saveState());
  settings.setValue("filter", pFilter->text());
  return iExitCode;
}

#endif
