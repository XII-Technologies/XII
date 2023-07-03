#include <EditorTest/EditorTestPCH.h>

#include "Project.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>

static xiiEditorTestProject s_EditorTestProject;

const char* xiiEditorTestProject::GetTestName() const
{
  return "Project Tests";
}

void xiiEditorTestProject::SetupSubTests()
{
  AddSubTest("Create Documents", SubTests::ST_CreateDocuments);
}

xiiResult xiiEditorTestProject::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return XII_FAILURE;

  if (SUPER::CreateAndLoadProject("TestProject").Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiResult xiiEditorTestProject::DeInitializeTest()
{
  // For profiling the doc creation.
  // SafeProfilingData();
  if (SUPER::DeInitializeTest().Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiTestAppRun xiiEditorTestProject::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  const auto& allDesc = xiiDocumentManager::GetAllDocumentDescriptors();
  for (auto it : allDesc)
  {
    auto pDesc = it.Value();

    if (pDesc->m_bCanCreate)
    {
      xiiStringBuilder sName = m_sProjectPath;
      sName.AppendPath(pDesc->m_sDocumentTypeName);
      sName.ChangeFileExtension(pDesc->m_sFileExtension);
      xiiDocument* pDoc = m_pApplication->m_pEditorApp->CreateDocument(sName, xiiDocumentFlags::RequestWindow);
      XII_TEST_BOOL(pDoc);
      ProcessEvents();
    }
  }
  // Make sure the engine process did not crash after creating every kind of document.
  XII_TEST_BOOL(!xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());

  // TODO: Newly created assets actually do not transform cleanly.
  if (false)
  {
    xiiAssetCurator::GetSingleton()->TransformAllAssets(xiiTransformFlags::TriggeredManually).IgnoreResult();

    xiiUInt32                                                      uiNumAssets;
    xiiHybridArray<xiiUInt32, xiiAssetInfo::TransformState::COUNT> sections;
    xiiAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

    XII_TEST_INT(sections[xiiAssetInfo::TransformState::TransformError], 0);
    XII_TEST_INT(sections[xiiAssetInfo::TransformState::MissingTransformDependency], 0);
    XII_TEST_INT(sections[xiiAssetInfo::TransformState::MissingThumbnailDependency], 0);
    XII_TEST_INT(sections[xiiAssetInfo::TransformState::CircularDependency], 0);
  }
  return xiiTestAppRun::Quit;
}
