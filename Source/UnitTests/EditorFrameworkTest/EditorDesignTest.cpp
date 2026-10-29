/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <GuiFoundation/Style/DarkEditorStyle.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

XII_CREATE_SIMPLE_TEST_GROUP(EditorDesign);

XII_CREATE_SIMPLE_TEST(EditorDesign, SearchAndScaling)
{
  QWidget host;
  auto*   pLayout = new QVBoxLayout(&host);
  auto*   pSearch = new xiiQtSearchWidget(&host);
  pLayout->addWidget(pSearch);
  auto* pEdit = pSearch->findChild<QLineEdit*>();
  XII_TEST_BOOL(pSearch->parentWidget() == &host);
  XII_TEST_BOOL(pEdit != nullptr);
  XII_TEST_BOOL(pEdit->isClearButtonEnabled());
  pSearch->setPlaceholderText(QStringLiteral("Search objects"));
  XII_TEST_BOOL(pEdit->accessibleName() == QStringLiteral("Search objects"));

  int iChanges    = 0;
  int iEnters     = 0;
  int iNavigation = 0;
  QObject::connect(pSearch, &xiiQtSearchWidget::textChanged, &host, [&](const QString&) { ++iChanges; });
  QObject::connect(pSearch, &xiiQtSearchWidget::enterPressed, &host, [&]() { ++iEnters; });
  QObject::connect(pSearch, &xiiQtSearchWidget::specialKeyPressed, &host, [&](Qt::Key) { ++iNavigation; });
  pSearch->setText(QStringLiteral("Mesh"));
  QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
  QApplication::sendEvent(pEdit, &escape);
  XII_TEST_BOOL(pSearch->text().isEmpty());
  XII_TEST_INT(iChanges, 2);
  QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
  QApplication::sendEvent(pEdit, &enter);
  QKeyEvent down(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
  QApplication::sendEvent(pEdit, &down);
  XII_TEST_INT(iEnters, 1);
  XII_TEST_INT(iNavigation, 1);
  pSearch->SetCaptureTabKey(false);
  QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
  QApplication::sendEvent(pEdit, &tab);
  XII_TEST_INT(iNavigation, 1);
  pSearch->SetCaptureTabKey(true);
  QApplication::sendEvent(pEdit, &tab);
  XII_TEST_INT(iNavigation, 2);
  // Use the production ownership contract: QApplication owns the installed style.
  auto* pStyle = new xiiQtDarkEditorStyle;
  QApplication::setStyle(pStyle);
  QPushButton button(QStringLiteral("Transform selection"));
  const int   iNormal = button.sizeHint().height();
  QFont       font    = button.font();
  font.setPointSizeF(font.pointSizeF() * 1.5);
  button.setFont(font);
  XII_TEST_BOOL(button.sizeHint().height() > iNormal);
  const auto palette = pStyle->standardPalette();
  XII_TEST_BOOL(palette.color(QPalette::Window) != palette.color(QPalette::Base));
  XII_TEST_BOOL(palette.color(QPalette::Text).lightness() > 200);
}
