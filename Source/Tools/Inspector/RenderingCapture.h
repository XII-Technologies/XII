/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <QElapsedTimer>
#include <QJsonObject>
#include <ads/DockWidget.h>

class QVBoxLayout;
class QHBoxLayout;
class QCheckBox;
class QLineEdit;
class QLabel;

/// Shared capture controls. UI owns all parsed data; telemetry never retains engine references.
class xiiQtRenderingCapture : public ads::CDockWidget
{
public:
  xiiQtRenderingCapture(ads::CDockManager* manager, const char* title, xiiUInt32 system);
  virtual ~xiiQtRenderingCapture() = default;
  void         ResetStats();
  void         RequestCapture();
  void         ReceiveMessages();
  bool         LoadCapture(const QByteArray& bytes);
  virtual bool ApplyCapture(const QJsonObject& object) = 0;

protected:
  virtual void ClearCapture() = 0;
  virtual void Rebuild()      = 0;
  void         SetDocument(const QJsonObject& object);
  void         SaveCapture();
  void         OpenCapture();

  QVBoxLayout* m_pLayout;
  QHBoxLayout* m_pControls;
  QCheckBox*   m_pLive;
  QLineEdit*   m_pFilter;
  QLabel*      m_pStatus;
  QJsonObject  m_Document;

private:
  xiiUInt32     m_uiSystem;
  QElapsedTimer m_RequestTime;
  bool          m_bPending = false;
};
