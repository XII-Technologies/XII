/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <GuiFoundation/Widgets/CommandPalette.moc.h>
#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QTreeWidget>

XII_CREATE_SIMPLE_TEST_GROUP(EditorCommands);
XII_CREATE_SIMPLE_TEST(EditorCommands, DiscoveryAndLifetime)
{
  QAction save(QStringLiteral("&Save document"));
  save.setShortcut(QKeySequence(QStringLiteral("Ctrl+S")));
  QAction disabled(QStringLiteral("Save all documents"));
  disabled.setEnabled(false);
  QAction hidden(QStringLiteral("Hidden command"));
  hidden.setVisible(false);
  auto* pTemporary  = new QAction(QStringLiteral("Temporary command"));
  int   iExecutions = 0;
  QObject::connect(&save, &QAction::triggered, [&]() { ++iExecutions; });
  xiiQtCommandPalette palette(nullptr, {&save, &disabled, &hidden, pTemporary});
  auto*               pSearch  = palette.findChild<QLineEdit*>();
  auto*               pResults = palette.findChild<QTreeWidget*>();
  XII_TEST_INT(pResults->topLevelItemCount(), 3);
  delete pTemporary;
  XII_TEST_INT(pResults->topLevelItemCount(), 2);
  pSearch->setText(QStringLiteral("document save"));
  XII_TEST_INT(pResults->topLevelItemCount(), 2);
  XII_TEST_BOOL(pResults->currentItem()->text(0) == QStringLiteral("Save document"));
  save.setEnabled(false);
  XII_TEST_BOOL(pResults->currentItem() == nullptr);
  save.setEnabled(true);
  pSearch->setText(QStringLiteral("no matching command"));
  QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
  QApplication::sendEvent(pSearch, &enter);
  QApplication::processEvents();
  XII_TEST_INT(iExecutions, 0);
  pSearch->setText(QStringLiteral("save document"));
  QApplication::sendEvent(pSearch, &enter);
  QApplication::processEvents();
  XII_TEST_INT(iExecutions, 1);
}
