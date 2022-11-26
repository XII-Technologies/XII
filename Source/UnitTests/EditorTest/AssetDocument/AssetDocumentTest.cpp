#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorTest/AssetDocument/AssetDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static xiiEditorAssetDocumentTest s_EditorAssetDocumentTest;

const char* xiiEditorAssetDocumentTest::GetTestName() const
{
  return "Asset Document Tests";
}

void xiiEditorAssetDocumentTest::SetupSubTests()
{
  AddSubTest("Async Save", SubTests::ST_AsyncSave);
  AddSubTest("Save on Transform", SubTests::ST_SaveOnTransform);
}

xiiResult xiiEditorAssetDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return XII_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiResult xiiEditorAssetDocumentTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiTestAppRun xiiEditorAssetDocumentTest::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_AsyncSave:
      AsyncSave();
      break;
    case SubTests::ST_SaveOnTransform:
      SaveOnTransform();
      break;
  }
  return xiiTestAppRun::Quit;
}

void xiiEditorAssetDocumentTest::AsyncSave()
{
  xiiAssetDocument* pDoc = nullptr;
  xiiStringBuilder  sName;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create Document")
  {
    sName = m_sProjectPath;
    sName.AppendPath("mesh.xiiMeshAsset");
    pDoc = static_cast<xiiAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, xiiDocumentFlags::RequestWindow));
    XII_TEST_BOOL(pDoc != nullptr);
    ProcessEvents();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Save Document")
  {
    // Save doc twice in a row without processing messages and then close it.
    xiiDocumentObject*     pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    xiiObjectAccessorBase* pAcc       = pDoc->GetObjectAccessor();
    xiiInt32               iOrder     = 0;
    xiiTaskGroupID         id         = pDoc->SaveDocumentAsync(
      [&iOrder](xiiDocument* doc, xiiStatus res) {
        XII_TEST_INT(iOrder, 0);
        iOrder = 1;
      },
      true);

    pAcc->StartTransaction("Edit Mesh");
    XII_TEST_BOOL(pAcc->SetValue(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();

    // Saving while another save is in progress should block. This ensures the correct state on disk.
    xiiString      sFile = pAcc->Get<xiiString>(pMeshAsset, "MeshFile");
    xiiTaskGroupID id2   = pDoc->SaveDocumentAsync([&iOrder](xiiDocument* doc, xiiStatus res) {
      XII_TEST_INT(iOrder, 1);
      iOrder = 2;
    });

    // Closing the document should wait for the async save to finish.
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    XII_TEST_INT(iOrder, 2);
    XII_TEST_BOOL(xiiTaskSystem::IsTaskGroupFinished(id));
    XII_TEST_BOOL(xiiTaskSystem::IsTaskGroupFinished(id2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Verify State of Disk")
  {
    pDoc                              = static_cast<xiiAssetDocument*>(m_pApplication->m_pEditorApp->OpenDocument(sName, xiiDocumentFlags::None));
    xiiDocumentObject*     pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    xiiObjectAccessorBase* pAcc       = pDoc->GetObjectAccessor();
    xiiString              sFile      = pAcc->Get<xiiString>(pMeshAsset, "MeshFile");
    XII_TEST_STRING(sFile, "Meshes/Cube.obj");
  }
  pDoc->GetDocumentManager()->CloseDocument(pDoc);
}

void xiiEditorAssetDocumentTest::SaveOnTransform()
{
  xiiAssetDocument* pDoc = nullptr;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create Document")
  {
    xiiStringBuilder sName = m_sProjectPath;
    sName.AppendPath("mesh2.xiiMeshAsset");
    pDoc = static_cast<xiiAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, xiiDocumentFlags::RequestWindow));
    XII_TEST_BOOL(pDoc != nullptr);
    ProcessEvents();
  }

  xiiObjectAccessorBase*   pAcc       = pDoc->GetObjectAccessor();
  const xiiDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform")
  {
    pAcc->StartTransaction("Edit Mesh");
    XII_TEST_BOOL(pAcc->SetValue(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();

    xiiTransformStatus res = pDoc->SaveDocument();
    XII_TEST_BOOL(res.Succeeded());

    // Transforming an asset in the background should fail and return NeedsImport as the asset needs to be modified which the background is not allowed to do, e.g. materials need to be created.
    res = xiiAssetCurator::GetSingleton()->TransformAsset(pDoc->GetGuid(), xiiTransformFlags::ForceTransform | xiiTransformFlags::BackgroundProcessing);
    XII_TEST_BOOL(res.m_Result == xiiTransformResult::NeedsImport);

    // Transforming a mesh asset with a mesh reference will trigger the material import and update
    // the materials table which requires a save during transform.
    res = xiiAssetCurator::GetSingleton()->TransformAsset(pDoc->GetGuid(), xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually);
    XII_TEST_BOOL(res.Succeeded());
    ProcessEvents();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Verify Transform")
  {
    // Transforming should have update the mesh asset with new material slots.
    xiiInt32 iCount = 0;
    XII_TEST_BOOL(pAcc->GetCount(pMeshAsset, "Materials", iCount).Succeeded());
    XII_TEST_INT(iCount, 1);

    xiiUuid subObject = pAcc->Get<xiiUuid>(pMeshAsset, "Materials", (xiiInt64)0);
    XII_TEST_BOOL(subObject.IsValid());
    const xiiDocumentObject* pSubObject = pAcc->GetObject(subObject);

    xiiString sLabel = pAcc->Get<xiiString>(pSubObject, "Label");
    XII_TEST_STRING(sLabel, "initialShadingGroup");
  }
  pDoc->GetDocumentManager()->CloseDocument(pDoc);
}
