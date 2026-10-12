/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/MaterialDragDropHandler.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <SharedPluginScene/Scene/SceneCreation.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#include <QDataStream>
#include <QIODevice>
#include <QMimeData>

namespace
{
  class MaterialDropDocument : public xiiDocument
  {
  public:
    MaterialDropDocument() : xiiDocument("MaterialDropTest.xiiScene", XII_DEFAULT_NEW(xiiDocumentObjectManager)) { m_pDocumentInfo = CreateDocumentInfo(); }
    xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
  };

  class MaterialDropHandler : public xiiMaterialDragDropHandler
  {
  public:
    // The document-manager lookup belongs to the integrated UI. Exercise the production
    // drag update/drop/cancel commands against a real document and command history here.
    void Begin(xiiDocument& document)
    {
      m_pDocument          = &document;
      m_AppliedToComponent = xiiUuid::MakeInvalid();
      m_iAppliedToSlot     = -1;
      document.GetCommandHistory()->BeginTemporaryCommands("Drag Material", true);
    }

    using xiiMaterialDragDropHandler::OnDragCancel;
    using xiiMaterialDragDropHandler::OnDragUpdate;
    using xiiMaterialDragDropHandler::OnDrop;
    using xiiAssetDragDropHandler::GetAssetGuid;
    using xiiAssetDragDropHandler::GetAssetGuidString;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorDocuments, SceneMaterialDragDrop)
{
  MaterialDropDocument document;
  xiiUuid              owner;
  XII_TEST_STATUS(xiiCreateSceneBox(*document.GetObjectAccessor(), xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3::MakeZero(), xiiVec3(1)), {}, owner));
  const auto* object = document.GetObjectAccessor()->GetObject(owner);
  if (!XII_TEST_BOOL(object && object->GetChildren().GetCount() == 1))
    return;
  const auto* mesh = object->GetChildren()[0];
  XII_TEST_BOOL(mesh->GetType()->IsDerivedFrom<xiiMeshComponentBase>());
  const char*                material = "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }";
  QByteArray                 encoded;
  QDataStream                stream(&encoded, QIODevice::WriteOnly);
  xiiHybridArray<QString, 1> guids;
  guids.PushBack(QString::fromUtf8(material));
  stream << guids;
  QMimeData mime;
  mime.setData("application/xiiEditor.AssetGuid", encoded);
  xiiDragDropInfo info;
  info.m_pMimeData          = &mime;
  info.m_TargetComponent    = mesh->GetGuid();
  info.m_iTargetObjectSubID = 0;
  MaterialDropHandler handler;
  auto*               accessor = document.GetObjectAccessor();
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Malformed asset MIME returns an invalid identity")
  {
    xiiDragDropInfo invalid;
    XII_TEST_BOOL(!handler.GetAssetGuid(&invalid).IsValid());
    QMimeData empty;
    empty.setData("application/xiiEditor.AssetGuid", QByteArray(4, char(0)));
    invalid.m_pMimeData = &empty;
    XII_TEST_BOOL(!handler.GetAssetGuid(&invalid).IsValid());
    QMimeData truncated;
    truncated.setData("application/xiiEditor.AssetGuid", encoded.left(5));
    invalid.m_pMimeData = &truncated;
    XII_TEST_BOOL(!handler.GetAssetGuid(&invalid).IsValid());
    XII_TEST_STRING(handler.GetAssetGuidString(&info), material);
    handler.Begin(document);
    invalid.m_TargetComponent    = mesh->GetGuid();
    invalid.m_iTargetObjectSubID = 0;
    handler.OnDragUpdate(&invalid);
    handler.OnDrop(&invalid);
    XII_TEST_INT(accessor->GetCountByName(mesh, "MaterialOverrides"), 0);
    XII_TEST_INT(document.GetCommandHistory()->GetUndoStackSize(), 1);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native material override drop is one undoable operation")
  {
    handler.Begin(document);
    handler.OnDragUpdate(&info);
    handler.OnDragUpdate(&info);
    XII_TEST_STRING(accessor->GetByName<xiiString>(mesh, "MaterialOverrides", 0), material);
    handler.OnDrop(&info);
    XII_TEST_INT(document.GetCommandHistory()->GetUndoStackSize(), 2);
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    XII_TEST_INT(accessor->GetCountByName(mesh, "MaterialOverrides"), 0);
    XII_TEST_STATUS(document.GetCommandHistory()->Redo());
    XII_TEST_STRING(accessor->GetByName<xiiString>(mesh, "MaterialOverrides", 0), material);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cancelled drag rolls back grown override arrays and invalid targets")
  {
    handler.Begin(document);
    info.m_iTargetObjectSubID = 2;
    handler.OnDragUpdate(&info);
    XII_TEST_INT(accessor->GetCountByName(mesh, "MaterialOverrides"), 3);
    handler.OnDragCancel();
    XII_TEST_INT(accessor->GetCountByName(mesh, "MaterialOverrides"), 1);
    XII_TEST_INT(document.GetCommandHistory()->GetUndoStackSize(), 2);
    handler.Begin(document);
    info.m_iTargetObjectSubID = -1;
    handler.OnDragUpdate(&info);
    handler.OnDrop(&info);
    XII_TEST_INT(accessor->GetCountByName(mesh, "MaterialOverrides"), 1);
    XII_TEST_INT(document.GetCommandHistory()->GetUndoStackSize(), 2);
    handler.Begin(document);
    info.m_iTargetObjectSubID = 2;
    handler.OnDragUpdate(&info);
    info.m_TargetComponent = xiiUuid::MakeUuid();
    handler.OnDrop(&info);
    XII_TEST_INT(accessor->GetCountByName(mesh, "MaterialOverrides"), 1);
    XII_TEST_INT(document.GetCommandHistory()->GetUndoStackSize(), 2);
  }
}
