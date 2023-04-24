#include <GameEngineTest/GameEngineTestPCH.h>

#include "Basics.h"
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/Process.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
xiiResult TranformProject(const char* szProjectPath, xiiUInt32 uiCleanVersion)
{
  xiiGlobalLog::AddLogWriter(&xiiLogWriter::Console::LogMessageHandler);
  XII_SCOPE_EXIT(xiiGlobalLog::RemoveLogWriter(&xiiLogWriter::Console::LogMessageHandler));

  xiiStringBuilder sBinPath = xiiOSFile::GetApplicationDirectory();

  xiiStringBuilder sProjectDir;
  if (xiiPathUtils::IsAbsolutePath(szProjectPath))
  {
    sProjectDir = szProjectPath;
    sProjectDir.MakeCleanPath();
  }
  else
  {
    // Assume to be relative to xii root.
    sProjectDir = sBinPath;
    sProjectDir.PathParentDirectory(3);
    sProjectDir.AppendPath(szProjectPath);
    sProjectDir.MakeCleanPath();
  }

  xiiLog::Info("Transforming assets for project '{}'", sProjectDir);

  {
    xiiStringBuilder sProjectAssetDir = sProjectDir;
    sProjectAssetDir.PathParentDirectory();
    sProjectAssetDir.AppendPath("AssetCache");

    xiiStringBuilder sCleanFile = sProjectAssetDir;
    sCleanFile.AppendPath("CleanVersion.dat");

    xiiUInt32 uiTargetVersion = 0;
    xiiOSFile f;

    if (f.Open(sCleanFile, xiiFileOpenMode::Read, xiiFileShareMode::Default).Succeeded())
    {
      f.Read(&uiTargetVersion, sizeof(xiiUInt32));
      f.Close();

      xiiLog::Info("CleanVersion.dat exists -> project has been transformed before.");
    }

    if (uiTargetVersion != uiCleanVersion)
    {
      xiiLog::Info("Clean version {} != {} -> deleting asset cache.", uiTargetVersion, uiCleanVersion);

      if (xiiOSFile::DeleteFolder(sProjectAssetDir).Failed())
      {
        xiiLog::Warning("Deleting the asset cache folder failed.");
      }

      if (f.Open(sCleanFile, xiiFileOpenMode::Write, xiiFileShareMode::Default).Succeeded())
      {
        f.Write(&uiCleanVersion, sizeof(xiiUInt32)).IgnoreResult();
        f.Close();
      }
    }
    else
    {
      xiiLog::Info("Clean version {} == {}.", uiTargetVersion, uiCleanVersion);
    }
  }

  sBinPath.AppendPath("EditorProcessor.exe");
  sBinPath.MakeCleanPath();

  xiiStringBuilder sOutputPath = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  {
    xiiStringView sProjectPath = xiiPathUtils::GetFileDirectory(szProjectPath);
    sProjectPath.Trim("\\/");
    xiiStringView sProjectName = xiiPathUtils::GetFileName(sProjectPath);
    sOutputPath.AppendPath("Transform");
    sOutputPath.Append(sProjectName);
    if (xiiOSFile::CreateDirectoryStructure(sOutputPath).Failed())
      xiiLog::Error("Failed to create output directory: {}", sOutputPath);
  }

  xiiProcessOptions opt;
  opt.m_sProcess = sBinPath;
  opt.m_Arguments.PushBack("-project");
  opt.AddArgument("\"{0}\"", sProjectDir);
  opt.m_Arguments.PushBack("-transform");
  opt.m_Arguments.PushBack("PC");
  opt.m_Arguments.PushBack("-outputDir");
  opt.AddArgument("\"{0}\"", sOutputPath);
  opt.m_Arguments.PushBack("-noRecent");
  opt.m_Arguments.PushBack("-AssetThumbnails");
  opt.m_Arguments.PushBack("never");
  opt.m_Arguments.PushBack("-renderer");
  opt.m_Arguments.PushBack(xiiGameApplication::GetActiveRenderer());

  xiiProcess proc;
  xiiLog::Info("Launching: '{0}'", sBinPath);
  xiiResult res = proc.Launch(opt);
  if (res.Failed())
  {
    proc.Terminate().IgnoreResult();
    xiiLog::Error("Failed to start process: '{0}'", sBinPath);
  }

  xiiTime timeout = xiiTime::Minutes(15);
  res             = proc.WaitToFinish(timeout);
  if (res.Failed())
  {
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
    xiiStringBuilder sDumpFile = sOutputPath;
    sDumpFile.AppendPath("Timeout.dmp");
    xiiMiniDumpUtils::WriteExternalProcessMiniDump(sDumpFile, proc.GetProcessID()).LogFailure();
#  endif
    proc.Terminate().IgnoreResult();
    xiiLog::Error("Process timeout ({1}): '{0}'", sBinPath, timeout);
    return XII_FAILURE;
  }
  if (proc.GetExitCode() != 0)
  {
    xiiLog::Error("Process failure ({0}): ExitCode: '{1}'", sBinPath, proc.GetExitCode());
    return XII_FAILURE;
  }

  xiiLog::Success("Executed Asset Processor to transform '{}'", szProjectPath);
  return XII_SUCCESS;
}
#endif

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
XII_CREATE_SIMPLE_TEST_GROUP(00_Init);

XII_CREATE_SIMPLE_TEST(00_Init, TransformBase)
{
  XII_TEST_BOOL(TranformProject("Data/Base/xiiProject", 2).Succeeded());
}

XII_CREATE_SIMPLE_TEST(00_Init, TransformBasics)
{
  XII_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Basics/xiiProject", 2).Succeeded());
}

XII_CREATE_SIMPLE_TEST(00_Init, TransformParticles)
{
  XII_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Particles/xiiProject", 3).Succeeded());
}

XII_CREATE_SIMPLE_TEST(00_Init, TransformTypeScript)
{
  XII_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/TypeScript/xiiProject", 3).Succeeded());
}

XII_CREATE_SIMPLE_TEST(00_Init, TransformEffects)
{
  XII_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Effects/xiiProject", 4).Succeeded());
}

XII_CREATE_SIMPLE_TEST(00_Init, TransformAnimations)
{
  XII_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Animations/xiiProject", 6).Succeeded());
}

XII_CREATE_SIMPLE_TEST(00_Init, TransformStateMachine)
{
  XII_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/StateMachine/xiiProject", 6).Succeeded());
}

XII_CREATE_SIMPLE_TEST(00_Init, TransformPlatformWin)
{
  XII_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/PlatformWin/xiiProject", 5).Succeeded());
}

XII_CREATE_SIMPLE_TEST(00_Init, TransformXR)
{
  XII_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/XR/xiiProject", 5).Succeeded());
}

#endif


static xiiGameEngineTestBasics s_GameEngineTestBasics;

const char* xiiGameEngineTestBasics::GetTestName() const
{
  return "Basic Engine Tests";
}

xiiGameEngineTestApplication* xiiGameEngineTestBasics::CreateApplication()
{
  m_pOwnApplication = XII_DEFAULT_NEW(xiiGameEngineTestApplication_Basics);
  return m_pOwnApplication;
}

void xiiGameEngineTestBasics::SetupSubTests()
{
  AddSubTest("Many Meshes", SubTests::ManyMeshes);
  AddSubTest("Skybox", SubTests::Skybox);
  AddSubTest("Debug Rendering", SubTests::DebugRendering);
  AddSubTest("Debug Rendering - No Lines", SubTests::DebugRendering2);
  AddSubTest("Load Scene", SubTests::LoadScene);
}

xiiResult xiiGameEngineTestBasics::InitializeSubTest(xiiInt32 iIdentifier)
{
  XII_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;

  if (iIdentifier == SubTests::ManyMeshes)
  {
    m_pOwnApplication->SubTestManyMeshesSetup();
    return XII_SUCCESS;
  }

  if (iIdentifier == SubTests::Skybox)
  {
    m_pOwnApplication->SubTestSkyboxSetup();
    return XII_SUCCESS;
  }

  if (iIdentifier == SubTests::DebugRendering || iIdentifier == SubTests::DebugRendering2)
  {
    m_pOwnApplication->SubTestDebugRenderingSetup();
    return XII_SUCCESS;
  }

  if (iIdentifier == SubTests::LoadScene)
  {
    m_pOwnApplication->SubTestLoadSceneSetup();
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiTestAppRun xiiGameEngineTestBasics::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (iIdentifier == SubTests::ManyMeshes)
    return m_pOwnApplication->SubTestManyMeshesExec(m_iFrame);

  if (iIdentifier == SubTests::Skybox)
    return m_pOwnApplication->SubTestSkyboxExec(m_iFrame);

  if (iIdentifier == SubTests::DebugRendering)
    return m_pOwnApplication->SubTestDebugRenderingExec(m_iFrame);

  if (iIdentifier == SubTests::DebugRendering2)
    return m_pOwnApplication->SubTestDebugRenderingExec2(m_iFrame);

  if (iIdentifier == SubTests::LoadScene)
    return m_pOwnApplication->SubTestLoadSceneExec(m_iFrame);

  XII_ASSERT_NOT_IMPLEMENTED;
  return xiiTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

xiiGameEngineTestApplication_Basics::xiiGameEngineTestApplication_Basics() :
  xiiGameEngineTestApplication("Basics")
{
}

void xiiGameEngineTestApplication_Basics::SubTestManyMeshesSetup()
{
  XII_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  xiiMeshResourceHandle hMesh = xiiResourceManager::LoadResource<xiiMeshResource>("Meshes/MissingMesh.xiiMesh");

  xiiInt32 dim = 15;

  for (xiiInt32 z = -dim; z <= dim; ++z)
  {
    for (xiiInt32 y = -dim; y <= dim; ++y)
    {
      for (xiiInt32 x = -dim; x <= dim; ++x)
      {
        xiiGameObjectDesc go;
        go.m_LocalPosition.Set(x * 5.0f, y * 5.0f, z * 5.0f);

        xiiGameObject* pObject;
        m_pWorld->CreateObject(go, pObject);

        xiiMeshComponent* pMesh;
        m_pWorld->GetOrCreateComponentManager<xiiMeshComponentManager>()->CreateComponent(pObject, pMesh);

        pMesh->SetMesh(hMesh);
      }
    }
  }
}

xiiTestAppRun xiiGameEngineTestApplication_Basics::SubTestManyMeshesExec(xiiInt32 iCurFrame)
{
  {
    auto pCamera = xiiDynamicCast<xiiGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 100.0f, 1.0f, 1000.0f);
    xiiVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + xiiVec3(1, 0, 0), xiiVec3(0, 0, 1));
  }

  xiiResourceManager::ForceNoFallbackAcquisition(3);

  if (Run() == xiiApplication::Execution::Quit)
    return xiiTestAppRun::Quit;

  if (iCurFrame > 3)
  {
    XII_TEST_IMAGE(0, 150);

    return xiiTestAppRun::Quit;
  }

  return xiiTestAppRun::Continue;
}

//////////////////////////////////////////////////////////////////////////


void xiiGameEngineTestApplication_Basics::SubTestSkyboxSetup()
{
  XII_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  xiiTextureCubeResourceHandle hSkybox = xiiResourceManager::LoadResource<xiiTextureCubeResource>("Textures/Cubemap/ezLogo_Cube_DXT1_Mips_D.dds");
  xiiMeshResourceHandle        hMesh   = xiiResourceManager::LoadResource<xiiMeshResource>("Meshes/MissingMesh.xiiMesh");

  // Skybox
  {
    xiiGameObjectDesc go;
    go.m_LocalPosition.SetZero();

    xiiGameObject* pObject;
    m_pWorld->CreateObject(go, pObject);

    xiiSkyBoxComponent* pSkybox;
    m_pWorld->GetOrCreateComponentManager<xiiSkyBoxComponentManager>()->CreateComponent(pObject, pSkybox);

    pSkybox->SetCubeMap(hSkybox);
  }

  // some foreground objects
  {
    xiiInt32 dim = 5;

    for (xiiInt32 z = -dim; z <= dim; ++z)
    {
      for (xiiInt32 y = -dim; y <= dim; ++y)
      {
        for (xiiInt32 x = -dim; x <= dim; ++x)
        {
          xiiGameObjectDesc go;
          go.m_LocalPosition.Set(x * 10.0f, y * 10.0f, z * 10.0f);

          xiiGameObject* pObject;
          m_pWorld->CreateObject(go, pObject);

          xiiMeshComponent* pMesh;
          m_pWorld->GetOrCreateComponentManager<xiiMeshComponentManager>()->CreateComponent(pObject, pMesh);

          pMesh->SetMesh(hMesh);
        }
      }
    }
  }
}

xiiTestAppRun xiiGameEngineTestApplication_Basics::SubTestSkyboxExec(xiiInt32 iCurFrame)
{
  xiiResourceManager::ForceNoFallbackAcquisition(3);

  auto pCamera = xiiDynamicCast<xiiGameState*>(GetActiveGameState())->GetMainCamera();
  pCamera->SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 120.0f, 1.0f, 100.0f);
  xiiVec3 pos = xiiVec3(iCurFrame * 5.0f, 0, 0);
  pCamera->LookAt(pos, pos + xiiVec3(1, 0, 0), xiiVec3(0, 0, 1));
  pCamera->RotateGlobally(xiiAngle::Degree(0), xiiAngle::Degree(0), xiiAngle::Degree(iCurFrame * 80.0f));

  if (Run() == xiiApplication::Execution::Quit)
    return xiiTestAppRun::Quit;

  if (iCurFrame < 5)
    return xiiTestAppRun::Continue;

  XII_TEST_IMAGE(iCurFrame - 5, 150);

  if (iCurFrame < 8)
    return xiiTestAppRun::Continue;

  return xiiTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

void xiiGameEngineTestApplication_Basics::SubTestDebugRenderingSetup()
{
  XII_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  xiiRenderWorld::ResetFrameCounter();
}

xiiTestAppRun xiiGameEngineTestApplication_Basics::SubTestDebugRenderingExec(xiiInt32 iCurFrame)
{
  {
    auto pCamera = xiiDynamicCast<xiiGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    xiiVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + xiiVec3(1, 0, 0), xiiVec3(0, 0, 1));
  }

  // line box
  {
    xiiBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(xiiVec3(10, -5, 1), xiiVec3(1, 2, 3));

    xiiTransform t;
    t.SetIdentity();
    t.m_qRotation.SetFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::Degree(25));
    xiiDebugRenderer::DrawLineBox(m_pWorld.Borrow(), bbox, xiiColor::HotPink, t);
  }

  // line box
  {
    xiiBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(xiiVec3(10, -3, 1), xiiVec3(1, 2, 3));

    xiiTransform t;
    t.SetIdentity();
    t.m_vPosition.Set(0, 5, -2);
    t.m_qRotation.SetFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::Degree(25));
    xiiDebugRenderer::DrawLineBoxCorners(m_pWorld.Borrow(), bbox, 0.5f, xiiColor::DeepPink, t);
  }

  // 2D Rect
  {
    xiiDebugRenderer::Draw2DRectangle(m_pWorld.Borrow(), xiiRectFloat(10, 50, 35, 15), 0.1f, xiiColor::LawnGreen);
  }

  // Sphere
  {
    xiiBoundingSphere sphere;
    sphere.SetElements(xiiVec3(8, -5, -4), 2);
    xiiDebugRenderer::DrawLineSphere(m_pWorld.Borrow(), sphere, xiiColor::Tomato);
  }

  // Solid box
  {
    xiiBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(xiiVec3(10, -5, 1), xiiVec3(1, 2, 3));

    xiiDebugRenderer::DrawSolidBox(m_pWorld.Borrow(), bbox, xiiColor::BurlyWood);
  }

  // Text
  {
    xiiDebugRenderer::Draw2DText(m_pWorld.Borrow(), "Not 'a test\"", xiiVec2I32(30, 10), xiiColor::AntiqueWhite, 24);
    xiiDebugRenderer::Draw2DText(m_pWorld.Borrow(), "!@#$%^&*()_[]{}|", xiiVec2I32(20, 200), xiiColor::AntiqueWhite, 24);
  }

  // Frustum
  {
    xiiFrustum f;
    f.SetFrustum(xiiVec3(5, 7, 3), xiiVec3(0, -1, 0), xiiVec3(0, 0, 1), xiiAngle::Degree(30), xiiAngle::Degree(20), 0.1f, 5.0f);
    xiiDebugRenderer::DrawLineFrustum(m_pWorld.Borrow(), f, xiiColor::Cornsilk);
  }

  // Lines
  {
    xiiHybridArray<xiiDebugRenderer::Line, 4> lines;
    lines.PushBack(xiiDebugRenderer::Line(xiiVec3(3, -4, -4), xiiVec3(4, -2, -3)));
    lines.PushBack(xiiDebugRenderer::Line(xiiVec3(4, -2, -3), xiiVec3(2, 2, -2)));
    xiiDebugRenderer::DrawLines(m_pWorld.Borrow(), lines, xiiColor::SkyBlue);
  }

  // Triangles
  {
    xiiHybridArray<xiiDebugRenderer::Triangle, 4> tris;
    tris.PushBack(xiiDebugRenderer::Triangle(xiiVec3(7, 0, 0), xiiVec3(7, 2, 0), xiiVec3(7, 2, 1)));
    tris.PushBack(xiiDebugRenderer::Triangle(xiiVec3(7, 3, 0), xiiVec3(7, 1, 0), xiiVec3(7, 3, 1)));
    xiiDebugRenderer::DrawSolidTriangles(m_pWorld.Borrow(), tris, xiiColor::Gainsboro);
  }

  if (Run() == xiiApplication::Execution::Quit)
    return xiiTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return xiiTestAppRun::Continue;

  XII_TEST_IMAGE(0, 150);

  return xiiTestAppRun::Quit;
}

xiiTestAppRun xiiGameEngineTestApplication_Basics::SubTestDebugRenderingExec2(xiiInt32 iCurFrame)
{
  {
    auto pCamera = xiiDynamicCast<xiiGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    xiiVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + xiiVec3(1, 0, 0), xiiVec3(0, 0, 1));
  }

  // Text
  {
    xiiDebugRenderer::Draw2DText(m_pWorld.Borrow(), xiiFmt("Frame# {}", xiiRenderWorld::GetFrameCounter()), xiiVec2I32(10, 10), xiiColor::AntiqueWhite, 24);
    xiiDebugRenderer::DrawInfoText(m_pWorld.Borrow(), xiiDebugRenderer::ScreenPlacement::BottomLeft, "test", xiiFmt("Frame# {}", xiiRenderWorld::GetFrameCounter()));
    xiiDebugRenderer::DrawInfoText(m_pWorld.Borrow(), xiiDebugRenderer::ScreenPlacement::BottomRight, "test", "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|");
  }

  if (Run() == xiiApplication::Execution::Quit)
    return xiiTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return xiiTestAppRun::Continue;

  XII_TEST_IMAGE(0, 150);

  return xiiTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

void xiiGameEngineTestApplication_Basics::SubTestLoadSceneSetup()
{
  xiiResourceManager::ForceNoFallbackAcquisition(3);
  xiiRenderContext::GetDefaultInstance()->SetAllowAsyncShaderLoading(false);

  LoadScene("Basics/AssetCache/Common/Lighting.xiiObjectGraph").IgnoreResult();
}

xiiTestAppRun xiiGameEngineTestApplication_Basics::SubTestLoadSceneExec(xiiInt32 iCurFrame)
{
  if (Run() == xiiApplication::Execution::Quit)
    return xiiTestAppRun::Quit;

  switch (iCurFrame)
  {
    case 1:
      XII_TEST_IMAGE(0, 150);
      break;

    case 2:
      XII_TEST_IMAGE(1, 150);
      return xiiTestAppRun::Quit;
  }

  return xiiTestAppRun::Continue;
}
