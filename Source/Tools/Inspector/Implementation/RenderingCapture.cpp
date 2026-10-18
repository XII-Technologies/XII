/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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
  m_pLive = new QCheckBox("Live");
  m_pLive->setToolTip("Request at most one capture per second while this panel is visible.");
  auto* open = new QPushButton("Open…");
  auto* save = new QPushButton("Save…");
  m_pFilter  = new QLineEdit;
  m_pFilter->setPlaceholderText("Filter by name…");
  m_pFilter->setAccessibleName("Capture name filter");
  for (auto* button : {capture, open, save})
    m_pControls->addWidget(button);
  m_pControls->addWidget(m_pLive);
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
      m_bPending = false;
      m_pStatus->setText("No capture received. Check InspectorPlugin is loaded and a render graph is executing.");
    }
    if (m_pLive->isChecked() && !isClosed() && isVisible() && !m_bPending && xiiTelemetry::IsConnectedToServer())
      RequestCapture();
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
  if (!xiiTelemetry::IsConnectedToServer())
  {
    m_pStatus->setText("Disconnected. Connect to capture live data, or open a saved capture.");
    return;
  }
  if (m_bPending)
    return;
  xiiTelemetry::SendToServer(m_uiSystem, 'CAPT');
  m_bPending = true;
  m_RequestTime.restart();
  m_pStatus->setText("Waiting for capture…");
}

void xiiQtRenderingCapture::ReceiveMessages()
{
  xiiTelemetryMessage message;
  while (xiiTelemetry::RetrieveMessage(m_uiSystem, message).Succeeded())
  {
    if (message.GetMessageID() != 'DATA' && message.GetMessageID() != 'ERRO')
      continue;
    xiiUInt32 size = 0;
    message.GetReader() >> size;
    if (size == 0 || size > 16 * 1024 * 1024)
    {
      m_pStatus->setText("Rejected invalid capture size.");
      m_bPending = false;
      continue;
    }
    QByteArray bytes(static_cast<int>(size), Qt::Uninitialized);
    m_bPending = false;
    if (message.GetReader().ReadBytes(bytes.data(), size) != size)
    {
      m_pStatus->setText("Rejected truncated capture.");
      continue;
    }
    if (message.GetMessageID() == 'ERRO')
      m_pStatus->setText(QString::fromUtf8(bytes));
    else
      LoadCapture(bytes);
  }
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
    return;
  QSaveFile  file(path);
  const auto bytes = QJsonDocument(m_Document).toJson(QJsonDocument::Compact);
  if (bytes.size() > 16 * 1024 * 1024)
  {
    m_pStatus->setText("Capture exceeds the 16 MiB file limit.");
    return;
  }
  if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
    m_pStatus->setText("Could not save capture: " + file.errorString());
}

void xiiQtRenderingCapture::OpenCapture()
{
  const auto path = QFileDialog::getOpenFileName(this, "Open capture", {}, "JSON capture (*.json)");
  if (path.isEmpty())
    return;
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    m_pStatus->setText("Could not open capture: " + file.errorString());
  else if (file.size() > 16 * 1024 * 1024)
    m_pStatus->setText("Capture exceeds 16 MiB.");
  else
  {
    m_pLive->setChecked(false);
    LoadCapture(file.readAll());
  }
}
