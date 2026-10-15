/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFrameworkTest/AssetTestDocument.h>
#include <EditorFrameworkTest/DocumentSaveUiTest.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/Dialogs/ModifiedDocumentsDlg.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QAbstractButton>
#include <QApplication>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  class SaveValidationWindow final : public xiiQtDocumentWindow
  {
  public:
    explicit SaveValidationWindow(xiiDocument* document) : xiiQtDocumentWindow(document) { FinishWindowCreation(); }
    xiiStringView GetWindowLayoutGroupName() const override { return "SaveValidation"; }
  };

  xiiDynamicArray<xiiUInt8> ReadSaveUiFile(xiiStringView path)
  {
    xiiOSFile                 file;
    xiiDynamicArray<xiiUInt8> bytes;
    if (XII_TEST_RESULT(file.Open(path, xiiFileOpenMode::Read)))
      file.ReadAll(bytes);
    return bytes;
  }
} // namespace

void TestDocumentSaveUi(xiiQtEditorApp& editor, xiiStringView projectDirectory)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Rejected saves keep documents and close dialogs open")
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Save validation: create document and window");
    xiiStringBuilder path = projectDirectory;
    path.AppendPath("SaveValidation.xiiResaveTest");
    auto* document = xiiDynamicCast<ResaveTestDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(document != nullptr))
      return;
    QPointer<SaveValidationWindow> window;
    XII_SCOPE_EXIT({
      if (document != nullptr)
      {
        document->MakePartiallyUnderstood(false);
        document->MakeReadOnly(false);
        document->GetDocumentManager()->CloseDocument(document);
      }
      QElapsedTimer elapsed;
      elapsed.start();
      while (!window.isNull() && elapsed.elapsed() < 2000)
        QApplication::processEvents(QEventLoop::AllEvents, 20);
      XII_TEST_BOOL(window.isNull());
    });
    window         = new SaveValidationWindow(document);
    auto* accessor = document->GetObjectAccessor();
    accessor->StartTransaction("Edit save validation payload");
    XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "Value", "Unsaved payload"));
    accessor->FinishTransaction();
    document->MakePartiallyUnderstood(true);
    const auto original = ReadSaveUiFile(path);
    if (!XII_TEST_BOOL(!original.IsEmpty()) || !XII_TEST_BOOL(!xiiQtUiServices::IsHeadless()))
      return;

    xiiUInt32 questions             = 0;
    xiiUInt32 failures              = 0;
    bool      identifiedUnknownType = false;
    QTimer    dismissMessages;
    QObject::connect(&dismissMessages, &QTimer::timeout, [&]() {
      if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
      {
        if (auto* yes = box->button(QMessageBox::Yes))
        {
          ++questions;
          yes->click();
        }
        else
        {
          ++failures;
          identifiedUnknownType |= box->text().contains("MissingResaveTestType");
          box->accept();
        }
      }
    });
    dismissMessages.start(0);
    xiiTestFramework::Output(xiiTestOutput::Message, "Save validation: reject direct save");
    XII_TEST_BOOL(window->SaveDocument().Failed());
    XII_TEST_INT(questions, 0);
    XII_TEST_INT(failures, 1);
    XII_TEST_BOOL(identifiedUnknownType);
    XII_TEST_BOOL(document->IsModified());
    XII_TEST_BOOL(ReadSaveUiFile(path) == original);

    questions = 0;
    failures  = 0;
    xiiTestFramework::Output(xiiTestOutput::Message, "Save validation: reject save before close");
    XII_TEST_BOOL(!window->CanCloseWindow());
    XII_TEST_INT(questions, 1);
    XII_TEST_INT(failures, 1);
    XII_TEST_BOOL(document->IsModified());
    XII_TEST_BOOL(!window.isNull());
    XII_TEST_BOOL(ReadSaveUiFile(path) == original);

    xiiHybridArray<xiiDocument*, 32> documents;
    documents.PushBack(document);
    xiiTestFramework::Output(xiiTestOutput::Message, "Save validation: reject modified-documents save");
    {
      xiiQtModifiedDocumentsDlg dialog(nullptr, documents);
      bool                      accepted = false;
      QObject::connect(&dialog, &QDialog::accepted, [&]() { accepted = true; });
      dialog.show();
      failures = 0;
      XII_TEST_BOOL(QMetaObject::invokeMethod(&dialog, "on_ButtonSaveSelected_clicked", Qt::DirectConnection));
      XII_TEST_BOOL(!accepted && dialog.isVisible());
      XII_TEST_INT(failures, 1);
      XII_TEST_BOOL(document->IsModified());
      XII_TEST_BOOL(ReadSaveUiFile(path) == original);
      XII_TEST_BOOL(QMetaObject::invokeMethod(&dialog, "on_ButtonDontSave_clicked", Qt::DirectConnection));
      XII_TEST_BOOL(accepted && !dialog.isVisible());
      XII_TEST_BOOL(document->IsModified());
      XII_TEST_BOOL(ReadSaveUiFile(path) == original);
    }
    document->MakePartiallyUnderstood(false);
    xiiTestFramework::Output(xiiTestOutput::Message, "Save validation: recover modified-documents save");
    {
      xiiQtModifiedDocumentsDlg dialog(nullptr, documents);
      bool                      accepted = false;
      QObject::connect(&dialog, &QDialog::accepted, [&]() { accepted = true; });
      dialog.show();
      XII_TEST_BOOL(QMetaObject::invokeMethod(&dialog, "on_ButtonSaveSelected_clicked", Qt::DirectConnection));
      XII_TEST_BOOL(accepted && !dialog.isVisible());
      XII_TEST_BOOL(!document->IsModified());
      XII_TEST_BOOL(ReadSaveUiFile(path) != original);
    }
    XII_TEST_INT(document->GetUnknownObjectTypeInstances(), 0);

    xiiTestFramework::Output(xiiTestOutput::Message, "Save validation: preserve newer edits during asynchronous save");
    accessor->StartTransaction("Save a snapshot");
    XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "Value", "Queued snapshot"));
    accessor->FinishTransaction();
    xiiUInt32  callbacks = 0;
    const auto pending   = document->SaveDocumentAsync([&](xiiDocument* saved, xiiStatus result) {
      XII_TEST_BOOL(saved == document && result.Succeeded());
      XII_TEST_BOOL(document->IsModified());
      ++callbacks;
    });
    if (!XII_TEST_BOOL(pending.IsValid()))
      return;
    accessor->StartTransaction("Edit after the snapshot");
    XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "Value", "Newer unsaved edit"));
    accessor->FinishTransaction();
    xiiTaskSystem::WaitForGroup(pending);
    XII_TEST_INT(callbacks, 1);
    XII_TEST_BOOL(document->IsModified());
    {
      const auto          bytes = ReadSaveUiFile(path);
      const xiiStringView contents(reinterpret_cast<const char*>(bytes.GetData()), bytes.GetCount());
      XII_TEST_BOOL(contents.FindSubString("Queued snapshot") != nullptr);
      XII_TEST_BOOL(contents.FindSubString("Newer unsaved edit") == nullptr);
    }
    questions = 0;
    failures  = 0;
    XII_TEST_BOOL(window->CanCloseWindow());
    XII_TEST_INT(questions, 1);
    XII_TEST_INT(failures, 0);
    XII_TEST_BOOL(!document->IsModified());
    {
      const auto          bytes = ReadSaveUiFile(path);
      const xiiStringView contents(reinterpret_cast<const char*>(bytes.GetData()), bytes.GetCount());
      XII_TEST_BOOL(contents.FindSubString("Newer unsaved edit") != nullptr);
    }

    // The production manager must finish the callback before deleting its document and docked window.
    accessor->StartTransaction("Save while closing the document");
    XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "Value", "Close drains async save"));
    accessor->FinishTransaction();
    const auto closing = document->SaveDocumentAsync([&](xiiDocument* saved, xiiStatus result) {
      XII_TEST_BOOL(saved == document && result.Succeeded());
      XII_TEST_BOOL(!document->IsModified());
      ++callbacks;
    });
    if (!XII_TEST_BOOL(closing.IsValid()))
      return;
    document->GetDocumentManager()->CloseDocument(document);
    document = nullptr;
    XII_TEST_INT(callbacks, 2);
    XII_TEST_BOOL(xiiTaskSystem::IsTaskGroupFinished(closing));
    {
      const auto          bytes = ReadSaveUiFile(path);
      const xiiStringView contents(reinterpret_cast<const char*>(bytes.GetData()), bytes.GetCount());
      XII_TEST_BOOL(contents.FindSubString("Close drains async save") != nullptr);
    }
  }
}
