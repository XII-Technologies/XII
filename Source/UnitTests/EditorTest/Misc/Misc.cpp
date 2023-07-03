#include <EditorTest/EditorTestPCH.h>

#include "Misc.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>

static xiiEditorTestMisc s_EditorTestMisc;

const char* xiiEditorTestMisc::GetTestName() const
{
  return "Misc Tests";
}

void xiiEditorTestMisc::SetupSubTests()
{
  AddSubTest("GameObject References", SubTests::GameObjectReferences);
}

xiiResult xiiEditorTestMisc::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return XII_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return XII_FAILURE;

  if (xiiStatus res = xiiAssetCurator::GetSingleton()->TransformAllAssets(xiiTransformFlags::None); res.Failed())
  {
    xiiLog::Error("Asset transform failed: {}", res.m_sMessage);
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiEditorTestMisc::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiTestAppRun xiiEditorTestMisc::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  if (iIdentifier == SubTests::GameObjectReferences)
  {
    m_pDocument = SUPER::OpenDocument("Scenes/GameObjectReferences.xiiScene");

    if (!XII_TEST_BOOL(m_pDocument != nullptr))
      return xiiTestAppRun::Quit;

    xiiAssetCurator::GetSingleton()->TransformAsset(m_pDocument->GetGuid(), xiiTransformFlags::Default);

    xiiQtEngineDocumentWindow* pWindow = qobject_cast<xiiQtEngineDocumentWindow*>(xiiQtDocumentWindow::FindWindowByDocument(m_pDocument));

    if (!XII_TEST_BOOL(pWindow != nullptr))
      return xiiTestAppRun::Quit;

    auto viewWidgets = pWindow->GetViewWidgets();

    if (!XII_TEST_BOOL(!viewWidgets.IsEmpty()))
      return xiiTestAppRun::Quit;

    xiiQtEngineViewWidget::InteractionContext ctxt;
    ctxt.m_pLastHoveredViewWidget = viewWidgets[0];
    xiiQtEngineViewWidget::SetInteractionContext(ctxt);

    viewWidgets[0]->m_pViewConfig->m_RenderMode  = xiiViewRenderMode::Default;
    viewWidgets[0]->m_pViewConfig->m_Perspective = xiiSceneViewPerspective::Perspective;
    viewWidgets[0]->m_pViewConfig->ApplyPerspectiveSetting(90.0f);

    ExecuteDocumentAction("Scene.Camera.JumpTo.0", m_pDocument, true);

    for (int i = 0; i < 10; ++i)
    {
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(100));
      ProcessEvents();
    }

    XII_TEST_BOOL(CaptureImage(pWindow, "GoRef").Succeeded());

    XII_TEST_IMAGE(1, 100);

    // Move everything to the layer and repeat the test.
    xiiScene2Document*         pScene = xiiDynamicCast<xiiScene2Document*>(m_pDocument);
    xiiHybridArray<xiiUuid, 2> layerGuids;
    pScene->GetAllLayers(layerGuids);
    XII_TEST_INT(layerGuids.GetCount(), 2);
    xiiUuid layerGuid = layerGuids[0] == pScene->GetGuid() ? layerGuids[1] : layerGuids[0];

    auto                           pAccessor = pScene->GetObjectAccessor();
    auto                           pRoot     = pScene->GetObjectManager()->GetRootObject();
    xiiHybridArray<xiiVariant, 16> values;
    pAccessor->GetValues(pRoot, "Children", values).AssertSuccess();

    xiiDeque<const xiiDocumentObject*> assets;
    for (auto& value : values)
    {
      assets.PushBack(pAccessor->GetObject(value.Get<xiiUuid>()));
    }
    xiiDeque<const xiiDocumentObject*> newObjects;
    MoveObjectsToLayer(pScene, assets, layerGuid, newObjects);

    XII_TEST_BOOL(CaptureImage(pWindow, "GoRef").Succeeded());

    XII_TEST_IMAGE(1, 100);
  }


  // const auto& allDesc = xiiDocumentManager::GetAllDocumentDescriptors();
  // for (auto* pDesc : allDesc)
  //{
  //  if (pDesc->m_bCanCreate)
  //  {
  //    xiiStringBuilder sName = m_sProjectPath;
  //    sName.AppendPath(pDesc->m_sDocumentTypeName);
  //    sName.ChangeFileExtension(pDesc->m_sFileExtension);
  //    xiiDocument* pDoc = m_pApplication->m_pEditorApp->CreateDocument(sName, xiiDocumentFlags::RequestWindow);
  //    XII_TEST_BOOL(pDoc);
  //    ProcessEvents();
  //  }
  //}
  //// Make sure the engine process did not crash after creating every kind of document.
  // XII_TEST_BOOL(!xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());

  ////TODO: Newly created assets actually do not transform cleanly.
  // if (false)
  //{
  //  xiiAssetCurator::GetSingleton()->TransformAllAssets(xiiTransformFlags::TriggeredManually);

  //  xiiUInt32 uiNumAssets;
  //  xiiHybridArray<xiiUInt32, xiiAssetInfo::TransformState::COUNT> sections;
  //  xiiAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

  //  XII_TEST_INT(sections[xiiAssetInfo::TransformState::TransformError], 0);
  //  XII_TEST_INT(sections[xiiAssetInfo::TransformState::MissingDependency], 0);
  //  XII_TEST_INT(sections[xiiAssetInfo::TransformState::MissingReference], 0);
  //}
  return xiiTestAppRun::Quit;
}

xiiResult xiiEditorTestMisc::InitializeSubTest(xiiInt32 iIdentifier)
{
  return XII_SUCCESS;
}

xiiResult xiiEditorTestMisc::DeInitializeSubTest(xiiInt32 iIdentifier)
{
  xiiDocumentManager::CloseAllDocuments();
  return XII_SUCCESS;
}
