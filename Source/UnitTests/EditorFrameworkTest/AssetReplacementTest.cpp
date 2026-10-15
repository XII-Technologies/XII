/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFrameworkTest/AssetReplacementTest.h>
#include <EditorFrameworkTest/AssetTestDocument.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

void TestAssetReplacement(xiiQtEditorApp& editor, xiiStringView projectDirectory)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Asset removal follows identity when a path is reused")
  {
    auto*            curator   = xiiAssetCurator::GetSingleton();
    xiiStringBuilder directory = projectDirectory;
    directory.AppendPath("AssetReplacement");
    if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(directory)))
      return;
    xiiStringBuilder path = directory;
    path.AppendPath("Replaced.xiiResaveTest");
    xiiStringBuilder dependencyPath = directory;
    dependencyPath.AppendPath("Dependency.xiiResaveTest");
    auto* dependency = xiiDynamicCast<ResaveTestDocument*>(editor.CreateDocument(dependencyPath, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(dependency != nullptr))
      return;
    const auto dependencyId = dependency->GetGuid();
    XII_SCOPE_EXIT(if (auto* document = xiiDocumentManager::GetDocumentByGuid(dependencyId)) document->GetDocumentManager()->CloseDocument(document));
    auto* original = xiiDynamicCast<ResaveTestDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(original != nullptr))
      return;
    const auto originalId       = original->GetGuid();
    auto*      originalAccessor = original->GetObjectAccessor();
    originalAccessor->StartTransaction("Set replaced asset dependency");
    XII_TEST_STATUS(originalAccessor->SetValueByName(original->GetPropertyObject(), "Package", dependencyPath.GetView()));
    originalAccessor->FinishTransaction();
    XII_TEST_STATUS(original->SaveDocument());
    curator->NotifyOfFileChange(path);
    curator->MainThreadTick(false);
    XII_TEST_BOOL(static_cast<bool>(curator->GetSubAsset(originalId)));
    xiiSet<xiiUuid> uses;
    curator->FindAllUses(dependencyId, uses, false);
    XII_TEST_INT(uses.GetCount(), 1);
    XII_TEST_BOOL(uses.Contains(originalId));
    original->GetDocumentManager()->CloseDocument(original);
    XII_TEST_BOOL(xiiDocumentManager::GetDocumentByGuid(originalId) == nullptr);

    // Its initial save marks the original identity as removed while this path is already
    // occupied by the newly registered document. The curator must not close that new document.
    auto* replacement = xiiDynamicCast<ResaveTestDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(replacement != nullptr))
      return;
    const auto replacementId = replacement->GetGuid();
    XII_SCOPE_EXIT(if (auto* document = xiiDocumentManager::GetDocumentByGuid(replacementId)) document->GetDocumentManager()->CloseDocument(document));
    XII_TEST_BOOL(replacementId != originalId);
    curator->NotifyOfFileChange(path);
    curator->MainThreadTick(false);
    XII_TEST_BOOL(xiiDocumentManager::GetDocumentByGuid(replacementId) == replacement);
    XII_TEST_BOOL(!static_cast<bool>(curator->GetSubAsset(originalId)));
    XII_TEST_BOOL(static_cast<bool>(curator->GetSubAsset(replacementId)));
    uses.Clear();
    curator->FindAllUses(dependencyId, uses, false);
    XII_TEST_BOOL(uses.IsEmpty());
    auto* accessor = replacement->GetObjectAccessor();
    accessor->StartTransaction("Edit replacement identity");
    XII_TEST_STATUS(accessor->SetValueByName(replacement->GetPropertyObject(), "Value", "Replacement remains usable"));
    accessor->FinishTransaction();
    XII_TEST_STATUS(replacement->SaveDocument());
    XII_TEST_BOOL(!replacement->IsModified());

    replacement->GetDocumentManager()->CloseDocument(replacement);
    replacement = xiiDynamicCast<ResaveTestDocument*>(editor.OpenDocument(path, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(replacement != nullptr))
      return;
    XII_TEST_BOOL(replacement->GetGuid() == replacementId);
    xiiVariant value;
    XII_TEST_STATUS(replacement->GetObjectAccessor()->GetValueByName(replacement->GetPropertyObject(), "Value", value));
    XII_TEST_STRING(value.Get<xiiString>(), "Replacement remains usable");

    // Deleting the replacement itself must still close that exact document once.
    xiiUInt32 closing      = 0;
    auto      subscription = xiiDocumentManager::s_Events.AddEventHandler([&](const xiiDocumentManager::Event& event) {
      if (event.m_Type == xiiDocumentManager::Event::Type::DocumentClosing && event.m_pDocument->GetGuid() == replacementId)
        ++closing;
    });
    XII_SCOPE_EXIT(xiiDocumentManager::s_Events.RemoveEventHandler(subscription));
    if (!XII_TEST_RESULT(xiiOSFile::DeleteFile(path)))
      return;
    curator->NotifyOfFileChange(path);
    curator->MainThreadTick(false);
    XII_TEST_INT(closing, 1);
    XII_TEST_BOOL(xiiDocumentManager::GetDocumentByGuid(replacementId) == nullptr);
    XII_TEST_BOOL(!static_cast<bool>(curator->GetSubAsset(replacementId)));
  }
}
