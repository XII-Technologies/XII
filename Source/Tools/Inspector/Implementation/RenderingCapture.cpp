/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Diagnostics/TelemetryService.h>
#include <Inspector/InspectorPCH.h>
#include <Inspector/RenderingCapture.h>
#include <QCheckBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QSaveFile>
#include <QVBoxLayout>

xiiQtRenderingCapture::xiiQtRenderingCapture(ads::CDockManager* manager, const char* title, xiiUInt32 system) :
  ads::CDockWidget(manager, title), m_uiSystem(system)
{
  auto* root  = new QWidget;
  m_pLayout   = new QVBoxLayout(root);
  m_pControls = new QHBoxLayout;
  m_pLayout->addLayout(m_pControls);
  auto* capture = new QPushButton("Capture");
  capture->setToolTip("Request a capture from the connected application's InspectorPlugin.");
  auto* open = new QPushButton("Open…");
  auto* save = new QPushButton("Save…");
  m_pFilter  = new QLineEdit;
  m_pFilter->setPlaceholderText("Filter by name…");
  m_pFilter->setAccessibleName("Capture name filter");
  for (auto* button : {capture, open, save})
  {
    m_pControls->addWidget(button);
  }
  m_pControls->addWidget(m_pFilter, 1);
  m_pStatus = new QLabel("Connect to an application with InspectorPlugin, then capture. Saved captures can be opened offline.");
  m_pStatus->setWordWrap(true);
  m_pStatus->setTextFormat(Qt::PlainText);
  m_pLayout->addWidget(m_pStatus);
  setWidget(root);
  connect(capture, &QPushButton::clicked, this, [this] { RequestCapture(); });
  connect(open, &QPushButton::clicked, this, [this] { OpenCapture(); });
  connect(save, &QPushButton::clicked, this, [this] { SaveCapture(); });
  connect(m_pFilter, &QLineEdit::textChanged, this, [this] { Rebuild(); });
  auto* timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [this] {
    if (m_bPending && m_RequestTime.elapsed() > 3000)
    {
      if (m_pClient && m_pClient->IsReady())
      {
        xiiTelemetrySubscription subscription;
        subscription.m_uiSource     = xiiTelemetrySources::RenderGraphs;
        subscription.m_uiGeneration = m_uiRequest;
        subscription.m_uiRequestId  = ++m_uiRequest;
        subscription.m_bEnabled     = false;
        m_pClient->Subscribe(subscription).IgnoreResult();
      }
      m_bPending = false;
      m_pStatus->setText(m_bReceivedCapture ? "Capture window completed." : "No capture received. Check InspectorPlugin is loaded and a render graph is executing.");
    }
  });
  timer->start(1000);
}

void xiiQtRenderingCapture::ResetStats()
{
  m_bPending = false;
  m_Document = {};
  ClearCapture();
  m_pStatus->setText("New connection. Capture to inspect this application.");
}

void xiiQtRenderingCapture::RequestCapture()
{
  if (!m_pClient || !m_pClient->IsReady())
  {
    m_pStatus->setText("Disconnected. Connect to capture live data, or open a saved capture.");
    return;
  }
  if (m_bPending)
  {
    return;
  }
  xiiTelemetrySubscription subscription;
  subscription.m_uiSource               = xiiTelemetrySources::RenderGraphs;
  subscription.m_uiRequestId            = ++m_uiRequest;
  subscription.m_uiGeneration           = m_uiRequest;
  subscription.m_uiIntervalMilliseconds = 100;
  if (m_pClient->Subscribe(subscription).Failed())
  {
    m_pStatus->setText("Capture request was rejected by the connection.");
    return;
  }
  m_bPending         = true;
  m_bReceivedCapture = false;
  m_RequestTime.restart();
  m_pStatus->setText("Waiting for capture…");
}



void xiiQtRenderingCapture::ReceiveDiagnostics(const xiiTelemetrySample& sample)
{
  if (sample.m_uiSource != xiiTelemetrySources::RenderGraphs || !m_bPending)
    return;
  const QByteArray bytes(reinterpret_cast<const char*>(sample.m_Data.GetData()), sample.m_Data.GetCount());
  if (sample.m_uiKind != 0)
    m_pStatus->setText(QString::fromUtf8(bytes));
  else
    m_bReceivedCapture |= LoadCapture(bytes);
}

bool xiiQtRenderingCapture::LoadCapture(const QByteArray& bytes)
{
  if (bytes.size() > 16 * 1024 * 1024)
  {
    m_pStatus->setText("Capture exceeds 16 MiB.");
    return false;
  }
  QJsonParseError error;
  const auto      doc = QJsonDocument::fromJson(bytes, &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject() || !ApplyCapture(doc.object()))
  {
    m_pStatus->setText("Invalid or unsupported capture. The previous capture is preserved.");
    return false;
  }
  return true;
}

void xiiQtRenderingCapture::SetDocument(const QJsonObject& object)
{
  m_Document = object;
}

void xiiQtRenderingCapture::SaveCapture()
{
  if (m_Document.isEmpty())
  {
    m_pStatus->setText("Capture data before saving.");
    return;
  }
  const auto path = QFileDialog::getSaveFileName(this, "Save capture", {}, "JSON capture (*.json)");
  if (path.isEmpty())
  {
    return;
  }
  QSaveFile  file(path);
  const auto bytes = QJsonDocument(m_Document).toJson(QJsonDocument::Compact);
  if (bytes.size() > 16 * 1024 * 1024)
  {
    m_pStatus->setText("Capture exceeds the 16 MiB file limit.");
    return;
  }
  if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
  {
    m_pStatus->setText("Could not save capture: " + file.errorString());
  }
}

void xiiQtRenderingCapture::OpenCapture()
{
  const auto path = QFileDialog::getOpenFileName(this, "Open capture", {}, "JSON capture (*.json)");
  if (path.isEmpty())
  {
    return;
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
  {
    m_pStatus->setText("Could not open capture: " + file.errorString());
  }
  else if (file.size() > 16 * 1024 * 1024)
  {
    m_pStatus->setText("Capture exceeds 16 MiB.");
  }
  else
  {
    LoadCapture(file.readAll());
  }
}
