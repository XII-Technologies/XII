/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCP/MCPPCH.h>

#ifdef XII_USE_QT

#  include <MCP/Application.h>
#  include <MCPPlugin/Json.h>

#  include <Foundation/Threading/Thread.h>
#  include <Foundation/Threading/ThreadUtils.h>

#  include <QApplication>
#  include <QComboBox>
#  include <QFormLayout>
#  include <QHeaderView>
#  include <QLabel>
#  include <QLineEdit>
#  include <QMainWindow>
#  include <QPlainTextEdit>
#  include <QPushButton>
#  include <QSettings>
#  include <QTabWidget>
#  include <QTableWidget>
#  include <QTimer>
#  include <QVBoxLayout>

#  include <atomic>

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

  class xiiMCPGatewayThread final : public xiiThread
  {
  public:
    explicit xiiMCPGatewayThread(const xiiMCPGatewayConfiguration& configuration) :
      xiiThread("MCP Gateway", 1024 * 1024),
      m_Configuration(configuration)
    {
      Start();
    }

    ~xiiMCPGatewayThread() override
    {
      m_bStop = true;
      Join();
    }

    bool Submit(xiiStringView sRequest)
    {
      XII_LOCK(m_Mutex);
      if (!m_sRequest.IsEmpty() || m_bInvoking || m_bStop)
      {
        return false;
      }
      m_sRequest = sRequest;
      m_sReply.Clear();
      return true;
    }

    void Read(xiiVariantDictionary& out_snapshot, xiiString& out_sReply)
    {
      XII_LOCK(m_Mutex);
      out_snapshot = m_Snapshot;
      out_sReply   = m_sReply;
    }

  private:
    xiiUInt32 Run() override
    {
      xiiMCPGateway gateway;
      const auto    status = gateway.Start(m_Configuration);
      if (status.Failed())
      {
        XII_LOCK(m_Mutex);
        m_Snapshot["error"] = status.GetMessageString();
        m_bStop             = true;
        return 1;
      }

      xiiMCPSession session;
      session.m_State = xiiMCPSession::State::Ready;
      xiiTime nextSnapshot;
      while (!m_bStop)
      {
        gateway.Update();
        xiiString sRequest;
        {
          XII_LOCK(m_Mutex);
          sRequest = std::move(m_sRequest);
          m_sRequest.Clear();
          m_bInvoking = !sRequest.IsEmpty();
        }
        if (!sRequest.IsEmpty())
        {
          xiiStringBuilder sReply;
          gateway.Process(sRequest, session, sReply);
          XII_LOCK(m_Mutex);
          m_sReply    = sReply;
          m_bInvoking = false;
        }

        const auto now = xiiTime::Now();
        if (now >= nextSnapshot)
        {
          auto snapshot = gateway.GetSnapshot();
          XII_LOCK(m_Mutex);
          m_Snapshot   = std::move(snapshot);
          nextSnapshot = now + xiiTime::MakeFromMilliseconds(200);
        }
        xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(5));
      }
      return 0;
    }

    xiiMCPGatewayConfiguration m_Configuration;
    xiiMutex                   m_Mutex;
    xiiVariantDictionary       m_Snapshot;
    xiiString                  m_sRequest;
    xiiString                  m_sReply;
    std::atomic_bool           m_bStop     = false;
    bool                       m_bInvoking = false;
  };

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
  int                 iArgumentCount  = 1;
  char                szApplication[] = "xiiMCP";
  char*               pArguments[]    = {szApplication, nullptr};
  QApplication        application(iArgumentCount, pArguments);
  QSettings           settings("XII", "xiiMCP");
  xiiMCPGatewayThread gateway(configuration);

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

  auto* pInstances = MakeTable({"Application", "Local endpoint", "Instance", "Connected", "Capabilities", "Status"});
  pTabs->addTab(pInstances, "Instances");

  auto* pCapabilities     = new QWidget();
  auto* pCapabilityLayout = new QVBoxLayout(pCapabilities);
  auto* pFilter           = new QLineEdit(settings.value("filter").toString());
  pFilter->setPlaceholderText("Filter by capability name or URI");
  auto* pKind = new QComboBox();
  pKind->addItems({"Tools", "Resources", "Prompts"});
  auto* pCapability = new QComboBox();
  auto* pSchema     = new QPlainTextEdit();
  pSchema->setReadOnly(true);
  pSchema->setPlaceholderText("Capability descriptor");
  auto* pInput = new QPlainTextEdit("{}");
  pInput->setPlaceholderText("JSON arguments");
  auto* pRun    = new QPushButton("Invoke");
  auto* pResult = new QPlainTextEdit();
  pResult->setReadOnly(true);
  pResult->setPlaceholderText("Operation result");
  pCapabilityLayout->addWidget(pFilter);
  pCapabilityLayout->addWidget(pKind);
  pCapabilityLayout->addWidget(pCapability);
  pCapabilityLayout->addWidget(pSchema);
  pCapabilityLayout->addWidget(pInput);
  pCapabilityLayout->addWidget(pRun);
  pCapabilityLayout->addWidget(pResult);
  pTabs->addTab(pCapabilities, "Capabilities");

  auto* pActivity = MakeTable({"Capability", "Target", "Elapsed (ms)", "Result"});
  pTabs->addTab(pActivity, "Activity");

  auto* pOptions       = new QWidget();
  auto* pOptionsLayout = new QFormLayout(pOptions);
  pOptionsLayout->addRow("Automatic discovery", new QLabel(configuration.m_bDiscovery ? "Enabled" : "Disabled"));
  pOptionsLayout->addRow("HTTP authentication", new QLabel(configuration.m_Http.m_sBearerToken.IsEmpty() ? "Local inspection" : "Bearer token"));
  pOptionsLayout->addRow("Request deadline", new QLabel(QString::number(configuration.m_Http.m_RequestTimeout.GetMilliseconds()) + " ms"));
  pOptionsLayout->addRow("Editing", new QLabel((configuration.m_uiPermissions & static_cast<xiiUInt8>(xiiMCPPermission::Edit)) ? "Enabled" : "Disabled"));
  pOptionsLayout->addRow("Simulation control", new QLabel((configuration.m_uiPermissions & static_cast<xiiUInt8>(xiiMCPPermission::Execute)) ? "Enabled" : "Disabled"));
  pTabs->addTab(pOptions, "Settings");

  QObject::connect(pCapability, &QComboBox::currentIndexChanged, [&] {
    pSchema->setPlainText(pCapability->currentData().toString());
  });
  QObject::connect(pRun, &QPushButton::clicked, [&] {
    if (pCapability->currentIndex() < 0)
    {
      return;
    }
    const auto arguments = pInput->toPlainText().toUtf8();
    xiiVariant json;
    if (xiiMCPJson::Parse(xiiStringView(arguments.constData(), static_cast<xiiUInt32>(arguments.size())), json).Failed() ||
        !json.IsA<xiiVariantDictionary>())
    {
      pResult->setPlainText("Enter a JSON object for arguments.");
      return;
    }
    const auto           key       = pCapability->currentText().toUtf8();
    const bool           bResource = pKind->currentIndex() == 1;
    xiiVariantDictionary params;
    params[bResource ? "uri" : "name"] = xiiString(xiiStringView(key.constData(), static_cast<xiiUInt32>(key.size())));
    if (!bResource)
    {
      params["arguments"] = std::move(json);
    }
    xiiVariantDictionary request;
    request["jsonrpc"] = xiiString("2.0");
    request["id"]      = 1u;
    request["method"]  = xiiString(pKind->currentIndex() == 0 ? "tools/call" : bResource ? "resources/read" :
                                                                                           "prompts/get");
    request["params"]  = std::move(params);
    xiiStringBuilder sRequest;
    if (xiiMCPJson::Write(request, sRequest).Failed() || !gateway.Submit(sRequest))
    {
      pResult->setPlainText("The request could not be queued.");
    }
    else
    {
      pResult->setPlainText("Running...");
    }
  });

  QTimer    timer;
  xiiString sLastReply;
  QObject::connect(&timer, &QTimer::timeout, [&] {
    xiiVariantDictionary snapshot;
    xiiString            sReply;
    gateway.Read(snapshot, sReply);
    if (sReply != sLastReply)
    {
      sLastReply = sReply;
      if (!sReply.IsEmpty())
      {
        pResult->setPlainText(ToQt(sReply));
      }
    }
    const auto sError = xiiMCPJson::GetString(snapshot, "error");
    if (!sError.IsEmpty())
    {
      pStatus->setText(ToQt(sError));
      return;
    }
    if (snapshot.IsEmpty())
    {
      return;
    }

    const auto& targets  = GetArray(snapshot, "targets");
    const auto& activity = GetArray(snapshot, "activity");
    const auto& tools    = GetArray(snapshot, "tools");
    pStatus->setText(QString("Listening at http://%1/mcp\n%2 known instances · %3 tools · %4 HTTP sessions")
                       .arg(ToQt(xiiMCPJson::GetString(snapshot, "endpoint")))
                       .arg(targets.GetCount())
                       .arg(tools.GetCount())
                       .arg(snapshot["sessions"].ConvertTo<xiiUInt32>()));
    FillTable(*pInstances, targets, {"application", "endpoint", "instance", "connected", "capabilityCount", "status"});
    FillTable(*pActivity, activity, {"capability", "target", "elapsedMs", "status"});

    const auto& entries   = GetArray(snapshot, pKind->currentIndex() == 0 ? "tools" : pKind->currentIndex() == 1 ? "resources" :
                                                                                                                   "prompts");
    const auto  sSelected = pCapability->currentText();
    pCapability->blockSignals(true);
    pCapability->clear();
    for (const auto& entry : entries)
    {
      const auto& descriptor = entry.Get<xiiVariantDictionary>();
      const auto  sName      = ToQt(xiiMCPJson::GetString(descriptor, pKind->currentIndex() == 1 ? "uri" : "name"));
      if (sName.contains(pFilter->text(), Qt::CaseInsensitive))
      {
        xiiStringBuilder sDescriptor;
        xiiMCPJson::Write(entry, sDescriptor).IgnoreResult();
        pCapability->addItem(sName, ToQt(sDescriptor));
      }
    }
    const int iPrevious = pCapability->findText(sSelected);
    if (iPrevious >= 0)
    {
      pCapability->setCurrentIndex(iPrevious);
    }
    pCapability->blockSignals(false);
    pSchema->setPlainText(pCapability->currentData().toString());
    pRun->setEnabled(pCapability->currentIndex() >= 0);
  });
  timer.start(250);
  window.show();
  const int iExitCode = application.exec();
  settings.setValue("geometry", window.saveGeometry());
  settings.setValue("filter", pFilter->text());
  return iExitCode;
}

#endif
