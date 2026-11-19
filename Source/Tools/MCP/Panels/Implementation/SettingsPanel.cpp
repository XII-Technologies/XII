/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCP/MCPPCH.h>

#ifdef XII_USE_QT

#  include <MCP/Panels/SettingsPanel.h>

#  include <QCheckBox>
#  include <QFormLayout>
#  include <QLineEdit>
#  include <QSettings>
#  include <QSpinBox>

namespace
{
  QString Text(xiiStringView sValue)
  {
    return QString::fromUtf8(sValue.GetStartPointer(), static_cast<qsizetype>(sValue.GetElementCount()));
  }
} // namespace

xiiMCPSettingsPanel::xiiMCPSettingsPanel(const xiiMCPGatewayConfiguration& defaults, QSettings& ref_settings, bool bRestore)
{
  auto stored = [&](const char* szKey, const QVariant& defaultValue) {
    return bRestore ? ref_settings.value(szKey, defaultValue) : defaultValue;
  };
  xiiStringBuilder sEndpoint;
  defaults.m_Http.m_Endpoint.ToString(sEndpoint);
  QStringList endpoints;
  for (const auto& sTarget : defaults.m_Endpoints)
  {
    endpoints.append(Text(sTarget));
  }

  m_pHttp = new QCheckBox("Accept HTTP clients");
  m_pHttp->setChecked(stored("http/enabled", defaults.m_Http.m_Endpoint.GetAddress().IsValid()).toBool());
  m_pEndpoint = new QLineEdit(stored("http/endpoint", Text(sEndpoint)).toString());
  m_pTargets  = new QLineEdit(stored("instances/endpoints", endpoints.join(", ")).toString());
  m_pTargets->setPlaceholderText("Comma-separated Local endpoints");
  m_pDiscovery = new QCheckBox("Discover applications automatically");
  m_pDiscovery->setChecked(stored("instances/discover", defaults.m_bDiscovery).toBool());
  m_pToken = new QLineEdit(Text(defaults.m_Http.m_sBearerToken));
  m_pToken->setEchoMode(QLineEdit::Password);
  m_pToken->setPlaceholderText("Optional for inspection, required for elevated HTTP permissions");
  m_pTimeout = new QSpinBox();
  m_pTimeout->setRange(100, 10000);
  m_pTimeout->setSuffix(" ms");
  m_pTimeout->setValue(stored("requests/timeoutMs", static_cast<int>(defaults.m_Http.m_RequestTimeout.GetMilliseconds())).toInt());
  m_pCapture = new QCheckBox("Capture diagnostics");
  m_pEdit    = new QCheckBox("Modify targets");
  m_pExecute = new QCheckBox("Run supported operations");
  // Elevated permissions and credentials apply to this process only.
  m_pCapture->setChecked((defaults.m_uiPermissions & static_cast<xiiUInt8>(xiiMCPPermission::Capture)) != 0);
  m_pEdit->setChecked((defaults.m_uiPermissions & static_cast<xiiUInt8>(xiiMCPPermission::Edit)) != 0);
  m_pExecute->setChecked((defaults.m_uiPermissions & static_cast<xiiUInt8>(xiiMCPPermission::Execute)) != 0);

  auto* pForm = new QFormLayout(this);
  pForm->addRow("Server", m_pHttp);
  pForm->addRow("Loopback address and port", m_pEndpoint);
  pForm->addRow("Bearer token (this session)", m_pToken);
  pForm->addRow("Discovery", m_pDiscovery);
  pForm->addRow("Explicit targets", m_pTargets);
  pForm->addRow("Request deadline", m_pTimeout);
  pForm->addRow("Permissions (this session)", m_pCapture);
  pForm->addRow(QString(), m_pEdit);
  pForm->addRow(QString(), m_pExecute);
}

xiiStatus xiiMCPSettingsPanel::ReadConfiguration(xiiMCPGatewayConfiguration& out_configuration) const
{
  xiiMCPGatewayConfiguration configuration;
  configuration.m_bDiscovery            = m_pDiscovery->isChecked();
  configuration.m_Http.m_RequestTimeout = xiiTime::MakeFromMilliseconds(m_pTimeout->value());
  auto applyPermission                  = [&](const QCheckBox& control, xiiMCPPermission permission) {
    if (control.isChecked())
    {
      configuration.m_uiPermissions |= static_cast<xiiUInt8>(permission);
    }
  };
  applyPermission(*m_pCapture, xiiMCPPermission::Capture);
  applyPermission(*m_pEdit, xiiMCPPermission::Edit);
  applyPermission(*m_pExecute, xiiMCPPermission::Execute);

  for (const auto& target : m_pTargets->text().split(',', Qt::SkipEmptyParts))
  {
    const auto name = target.trimmed().toUtf8();
    if (!name.isEmpty())
    {
      xiiNetworkLocalEndpoint endpoint;
      if (xiiNetworkLocalEndpoint::Make(name.constData(), endpoint).Failed())
      {
        return xiiStatus("An explicit Local endpoint name is invalid.");
      }
      configuration.m_Endpoints.PushBack(xiiString(name.constData()));
    }
  }

  if (m_pHttp->isChecked())
  {
    const auto endpoint = m_pEndpoint->text().trimmed().toUtf8();
    if (xiiNetworkEndpoint::Parse(endpoint.constData(), configuration.m_Http.m_Endpoint).Failed() ||
        !configuration.m_Http.m_Endpoint.GetAddress().IsLoopback())
    {
      return xiiStatus("Enter a numeric loopback address with a port.");
    }
    const auto token                     = m_pToken->text().toUtf8();
    configuration.m_Http.m_sBearerToken  = token.constData();
    configuration.m_Http.m_uiPermissions = configuration.m_uiPermissions;
    if (configuration.m_Http.m_sBearerToken.IsEmpty() && configuration.m_uiPermissions != static_cast<xiiUInt8>(xiiMCPPermission::Read))
    {
      return xiiStatus("Enter a bearer token or disable HTTP before granting elevated permissions.");
    }
  }
  out_configuration = std::move(configuration);
  return XII_SUCCESS;
}

void xiiMCPSettingsPanel::SavePreferences(QSettings& ref_settings) const
{
  ref_settings.setValue("http/enabled", m_pHttp->isChecked());
  ref_settings.setValue("http/endpoint", m_pEndpoint->text());
  ref_settings.setValue("instances/discover", m_pDiscovery->isChecked());
  ref_settings.setValue("instances/endpoints", m_pTargets->text());
  ref_settings.setValue("requests/timeoutMs", m_pTimeout->value());
}

#endif
