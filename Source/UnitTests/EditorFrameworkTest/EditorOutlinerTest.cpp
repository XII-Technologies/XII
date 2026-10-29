/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  class OutlinerDocument : public xiiDocument
  {
  public:
    OutlinerDocument() : xiiDocument("OutlinerTest.xiiScene", XII_DEFAULT_NEW(xiiDocumentObjectManager)) { m_pDocumentInfo = CreateDocumentInfo(); }
    xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
  };
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EditorOutliner);
XII_CREATE_SIMPLE_TEST(EditorOutliner, FilterPreservesSelection)
{
  OutlinerDocument document;
  auto*            pAccessor = document.GetObjectAccessor();
  pAccessor->StartTransaction("Create objects");
  xiiUuid first, second;
  XII_TEST_STATUS(pAccessor->AddObjectByName(document.GetObjectManager()->GetRootObject(), "Children", -1, xiiGetStaticRTTI<xiiGameObject>(), first));
  XII_TEST_STATUS(pAccessor->AddObjectByName(document.GetObjectManager()->GetRootObject(), "Children", -1, xiiGetStaticRTTI<xiiGameObject>(), second));
  XII_TEST_STATUS(pAccessor->SetValueByName(pAccessor->GetObject(first), "Name", "Robot"));
  XII_TEST_STATUS(pAccessor->SetValueByName(pAccessor->GetObject(second), "Name", "Camera"));
  pAccessor->FinishTransaction();
  auto pModel = std::make_unique<xiiQtDocumentTreeModel>(document.GetObjectManager());
  pModel->AddAdapter(new xiiQtDummyAdapter(document.GetObjectManager(), xiiGetStaticRTTI<xiiDocumentRoot>(), "Children"));
  pModel->AddAdapter(new xiiQtNamedAdapter(document.GetObjectManager(), xiiGetStaticRTTI<xiiGameObject>(), "Children", "Name"));
  xiiQtDocumentTreeView view(nullptr, &document, std::move(pModel));
  document.GetSelectionManager()->SetSelection(pAccessor->GetObject(first));
  view.SetFilterText(QStringLiteral("Camera"));
  XII_TEST_INT(view.model()->rowCount(), 1);
  XII_TEST_BOOL(document.GetSelectionManager()->GetCurrentObject() == pAccessor->GetObject(first));
  view.SetFilterText(QString());
  XII_TEST_INT(view.model()->rowCount(), 2);
  XII_TEST_BOOL(document.GetSelectionManager()->GetCurrentObject() == pAccessor->GetObject(first));
  XII_TEST_INT(view.selectionModel()->selectedRows().size(), 1);
  XII_TEST_BOOL(view.selectionModel()->selectedRows().front().data().toString() == QStringLiteral("Robot"));
}
