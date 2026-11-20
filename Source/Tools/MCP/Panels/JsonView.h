/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifdef XII_USE_QT

#  include <Foundation/Strings/StringView.h>
#  include <Foundation/Types/Variant.h>
#  include <QString>
#  include <QWidget>

class QImage;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QScrollArea;
class QTabWidget;
class QTreeWidget;

class xiiMCPJsonView final : public QWidget
{
public:
  explicit xiiMCPJsonView(const QString& title);

  void SetDocument(const xiiVariant& value, xiiStringView sResponse = {});
  void SetImagePreview(const QImage& image);
  void Clear(const QString& message = {});
  void SetMessage(const QString& message);

private:
  void           RefreshSource();
  const QString& GetJsonSource();

  xiiVariant      m_Value;
  QString         m_sJson;
  QString         m_sResponse;
  QTabWidget*     m_pTabs        = nullptr;
  QTreeWidget*    m_pTree        = nullptr;
  QPlainTextEdit* m_pJson        = nullptr;
  QPlainTextEdit* m_pResponse    = nullptr;
  QScrollArea*    m_pImageScroll = nullptr;
  QLabel*         m_pImage       = nullptr;
  QPushButton*    m_pCopy        = nullptr;
  bool            m_bHasDocument = false;
};

#endif
