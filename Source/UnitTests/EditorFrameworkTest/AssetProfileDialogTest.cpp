/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Dialogs/AssetProfilesDlg.moc.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <EditorFrameworkTest/AssetProfileTest.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <QAbstractButton>
#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/Strings/StringConversion.h>
#endif

namespace
{
  xiiDynamicArray<xiiUInt8> ReadFile(xiiStringView path)
  {
    xiiFileReader file;
    if (!XII_TEST_RESULT(file.Open(path)))
      return {};
    xiiDynamicArray<xiiUInt8> bytes;
    bytes.SetCountUninitialized(static_cast<xiiUInt32>(file.GetFileSize()));
    XII_TEST_BOOL(file.ReadBytes(bytes.GetData(), bytes.GetCount()) == bytes.GetCount());
    return bytes;
  }

  const xiiDocument* GetDraft(xiiQtAssetProfilesDlg& dialog)
  {
    const auto* model = static_cast<const xiiQtDocumentTreeModel*>(dialog.Tree->GetProxyFilterModel()->sourceModel());
    return model->GetDocumentTree()->GetDocument();
  }

  QModelIndex IndexFor(xiiQtAssetProfilesDlg& dialog, const xiiDocumentObject* object)
  {
    const auto* model = static_cast<const xiiQtDocumentTreeModel*>(dialog.Tree->GetProxyFilterModel()->sourceModel());
    return dialog.Tree->GetProxyFilterModel()->mapFromSource(model->ComputeModelIndex(object));
  }
} // namespace

void TestAssetProfileDialog(xiiStringView projectDirectory)
{
  auto*                                  curator        = xiiAssetCurator::GetSingleton();
  const auto                             originalActive = curator->GetActiveAssetProfileIndex();
  xiiHybridArray<xiiPlatformProfile*, 8> original;
  for (xiiUInt32 i = 0; i < curator->GetNumAssetProfiles(); ++i)
    original.PushBack(xiiReflectionSerializer::Clone(curator->GetAssetProfile(i)));
  XII_SCOPE_EXIT(XII_TEST_STATUS(curator->ApplyAssetProfiles({original.GetData(), original.GetCount()}, originalActive));
                 for (auto* profile : original) XII_DEFAULT_DELETE(profile));

  xiiPlatformProfile old;
  old.SetConfigName("OldDialogProfile");
  old.AddMissingConfigs();
  const xiiPlatformProfile* initial[] = {original[0], &old};
  XII_TEST_STATUS(curator->ApplyAssetProfiles(initial, 1));
  xiiStringBuilder definitions(projectDirectory), runtimeDefault(projectDirectory), runtimeOld(projectDirectory);
  definitions.AppendPath("Editor/AssetProfiles.ddl");
  runtimeDefault.AppendPath("RuntimeConfigs/Default.xiiProfile");
  runtimeOld.AppendPath("RuntimeConfigs/OldDialogProfile.xiiProfile");
  const auto before = ReadFile(definitions);
  {
    xiiQtAssetProfilesDlg cancelled(nullptr);
    const auto*           draft    = GetDraft(cancelled);
    auto*                 accessor = draft->GetObjectAccessor();
    const auto*           profile  = draft->GetObjectManager()->GetRootObject()->GetChildren()[0];
    accessor->StartTransaction("Draft profile edit");
    XII_TEST_STATUS(accessor->SetValueByName(profile, "TargetPlatform", "Linux"));
    accessor->FinishTransaction();
    XII_TEST_STRING(curator->GetAssetProfile(0)->GetTargetPlatform(), original[0]->GetTargetPlatform());
    XII_TEST_BOOL(QMetaObject::invokeMethod(&cancelled, "on_ButtonCancel_clicked", Qt::DirectConnection));
    XII_TEST_BOOL(ReadFile(definitions) == before);
    XII_TEST_STRING(curator->GetActiveAssetProfile()->GetConfigName(), "OldDialogProfile");
  }

  xiiQtAssetProfilesDlg dialog(nullptr);
  const auto*           draft     = GetDraft(dialog);
  auto*                 accessor  = draft->GetObjectAccessor();
  const auto*           root      = draft->GetObjectManager()->GetRootObject();
  const auto*           oldObject = root->GetChildren()[1];
  xiiUuid               first, second;
  accessor->StartTransaction("Add two draft profiles");
  XII_TEST_STATUS(accessor->AddObjectByName(root, "Children", -1, xiiGetStaticRTTI<xiiPlatformProfile>(), first));
  XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(first), "Name", "FirstDialogProfile"));
  XII_TEST_STATUS(accessor->AddObjectByName(root, "Children", -1, xiiGetStaticRTTI<xiiPlatformProfile>(), second));
  XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(second), "Name", "SecondDialogProfile"));
  accessor->FinishTransaction();
  const auto selected = IndexFor(dialog, accessor->GetObject(second));
  XII_TEST_BOOL(QMetaObject::invokeMethod(&dialog, "OnItemDoubleClicked", Qt::DirectConnection, Q_ARG(QModelIndex, selected)));
  dialog.Tree->selectionModel()->select(IndexFor(dialog, oldObject), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  QTimer::singleShot(0, []() {
    if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
      box->button(QMessageBox::Yes)->click();
  });
  XII_TEST_BOOL(QMetaObject::invokeMethod(&dialog, "on_DeleteButton_clicked", Qt::DirectConnection));
  XII_TEST_INT(root->GetChildren().GetCount(), 3);
  XII_TEST_BOOL(IndexFor(dialog, accessor->GetObject(second)).data().toString().contains("(switch to)"));
  XII_TEST_INT(curator->GetNumAssetProfiles(), 2);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  {
    const auto*  liveDefault  = curator->GetAssetProfile(0);
    const auto*  liveActive   = curator->GetActiveAssetProfile();
    const auto   defaultBytes = ReadFile(runtimeDefault);
    const auto   oldBytes     = ReadFile(runtimeOld);
    const HANDLE file         = CreateFileW(xiiStringWChar(definitions).GetData(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!XII_TEST_BOOL(file != INVALID_HANDLE_VALUE))
      return;
    XII_SCOPE_EXIT(CloseHandle(file));
    bool reported = false;
    QTimer::singleShot(0, [&]() {
      if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
      {
        reported = box->text().contains("Could not save asset profiles") && box->text().contains("Could not install profile file");
        box->accept();
      }
    });
    XII_TEST_BOOL(QMetaObject::invokeMethod(&dialog, "on_ButtonOk_clicked", Qt::DirectConnection));
    XII_TEST_BOOL(reported);
    XII_TEST_INT(dialog.result(), QDialog::Rejected);
    XII_TEST_BOOL(curator->GetAssetProfile(0) == liveDefault && curator->GetActiveAssetProfile() == liveActive);
    XII_TEST_BOOL(ReadFile(definitions) == before);
    XII_TEST_BOOL(ReadFile(runtimeDefault) == defaultBytes);
    XII_TEST_BOOL(ReadFile(runtimeOld) == oldBytes);
    xiiStringBuilder absent(projectDirectory);
    absent.AppendPath("RuntimeConfigs/FirstDialogProfile.xiiProfile");
    XII_TEST_BOOL(!xiiOSFile::ExistsFile(absent));
    absent = projectDirectory;
    absent.AppendPath("RuntimeConfigs/SecondDialogProfile.xiiProfile");
    XII_TEST_BOOL(!xiiOSFile::ExistsFile(absent));
    XII_TEST_INT(root->GetChildren().GetCount(), 3);
  }
#endif
  XII_TEST_BOOL(QMetaObject::invokeMethod(&dialog, "on_ButtonOk_clicked", Qt::DirectConnection));
  XII_TEST_INT(dialog.result(), QDialog::Accepted);
  XII_TEST_INT(curator->GetNumAssetProfiles(), 3);
  XII_TEST_STRING(curator->GetAssetProfile(0)->GetConfigName(), "Default");
  XII_TEST_STRING(curator->GetAssetProfile(1)->GetConfigName(), "FirstDialogProfile");
  XII_TEST_STRING(curator->GetAssetProfile(2)->GetConfigName(), "SecondDialogProfile");
  XII_TEST_INT(curator->GetActiveAssetProfileIndex(), 2);
  XII_TEST_BOOL(!xiiOSFile::ExistsFile(runtimeOld));
  // The disk snapshot and active pointers are already current; an unchanged reload must preserve them.
  const auto* active = curator->GetActiveAssetProfile();
  XII_TEST_RESULT(curator->ReloadAssetProfiles());
  XII_TEST_BOOL(curator->GetActiveAssetProfile() == active);
}
