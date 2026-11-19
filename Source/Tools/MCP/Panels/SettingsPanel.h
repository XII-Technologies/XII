/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifdef XII_USE_QT

#  include <MCP/Gateway.h>

#  include <QWidget>

class QCheckBox;
class QLineEdit;
class QSettings;
class QSpinBox;

class xiiMCPSettingsPanel final : public QWidget
{
public:
  xiiMCPSettingsPanel(const xiiMCPGatewayConfiguration& defaults, QSettings& ref_settings, bool bRestore);

  [[nodiscard]] xiiStatus ReadConfiguration(xiiMCPGatewayConfiguration& out_configuration) const;
  void                    SavePreferences(QSettings& ref_settings) const;

private:
  QCheckBox* m_pHttp      = nullptr;
  QLineEdit* m_pEndpoint  = nullptr;
  QLineEdit* m_pTargets   = nullptr;
  QCheckBox* m_pDiscovery = nullptr;
  QLineEdit* m_pToken     = nullptr;
  QSpinBox*  m_pTimeout   = nullptr;
  QCheckBox* m_pCapture   = nullptr;
  QCheckBox* m_pEdit      = nullptr;
  QCheckBox* m_pExecute   = nullptr;
};

#endif
