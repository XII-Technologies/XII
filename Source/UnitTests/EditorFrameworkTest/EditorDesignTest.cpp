/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <GuiFoundation/Style/DarkEditorStyle.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <Qt>

XII_CREATE_SIMPLE_TEST_GROUP(EditorDesign);

XII_CREATE_SIMPLE_TEST(EditorDesign, PhysicalLetterKeys)
{
  const QKeyEvent synthetic(QEvent::KeyPress, Qt::Key_W, Qt::NoModifier);
  XII_TEST_BOOL(xiiQtUtils::IsEquivalentQtKey(&synthetic, Qt::Key_W));
  XII_TEST_BOOL(!xiiQtUtils::IsEquivalentQtKey(&synthetic, Qt::Key_A));

  xiiUInt32 uiOffset = 0U;
#if XII_ENABLED(XII_PLATFORM_LINUX)
  const auto sPlatform = QGuiApplication::platformName();
  if (sPlatform != "xcb" && !sPlatform.startsWith("wayland"))
  {
    return;
  }
  uiOffset = 8U;
#elif XII_DISABLED(XII_PLATFORM_WINDOWS)
  return;
#endif

  const xiiUInt32 uiScanCodes[] = {30U, 48U, 46U, 32U, 18U, 33U, 34U, 35U, 23U, 36U, 37U, 38U, 50U, 49U, 24U, 25U, 16U, 19U, 31U, 20U, 22U, 47U, 17U, 45U, 21U, 44U};
  for (xiiUInt32 i = 0U; i < XII_ARRAY_SIZE(uiScanCodes); ++i)
  {
    const auto reference = static_cast<Qt::Key>(Qt::Key_A + i);
    for (const auto type : {QEvent::KeyPress, QEvent::KeyRelease})
    {
      // Physical controls remain independent of the active keyboard layout.
      const QKeyEvent event(type, Qt::Key_0, Qt::ShiftModifier, uiScanCodes[i] + uiOffset, 0U, 0U);
      XII_TEST_BOOL(xiiQtUtils::IsEquivalentQtKey(&event, reference));
      XII_TEST_BOOL(!xiiQtUtils::IsEquivalentQtKey(&event, static_cast<Qt::Key>(Qt::Key_A + (i + 1U) % 26U)));
    }
  }
}

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
