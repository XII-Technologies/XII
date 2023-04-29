#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"
#include <Core/Graphics/Camera.h>

xiiResult xiiRendererTestBasics::InitializeSubTest(xiiInt32 iIdentifier)
{
  m_iFrame = -1;

  if (xiiGraphicsTest::InitializeSubTest(iIdentifier).Failed())
    return XII_FAILURE;

  if (SetupRenderer().Failed())
    return XII_FAILURE;

  if (iIdentifier == SubTests::ST_ClearScreen)
  {
    return CreateWindow(320, 240);
  }

  if (CreateWindow().Failed())
    return XII_FAILURE;

  m_hSphere  = CreateSphere(3, 1.0f);
  m_hSphere2 = CreateSphere(1, 0.75f);
  m_hTorus   = CreateTorus(16, 0.5f, 0.75f);
  m_hLongBox = CreateBox(0.4f, 0.2f, 2.0f);
  m_hLineBox = CreateLineBox(0.4f, 0.2f, 2.0f);

  return XII_SUCCESS;
}

xiiResult xiiRendererTestBasics::DeInitializeSubTest(xiiInt32 iIdentifier)
{
  m_hSphere.Invalidate();
  m_hSphere2.Invalidate();
  m_hTorus.Invalidate();
  m_hLongBox.Invalidate();
  m_hLineBox.Invalidate();
  m_hTexture2D.Invalidate();
  m_hTextureCube.Invalidate();

  DestroyWindow();
  ShutdownRenderer();

  if (xiiGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}


xiiTestAppRun xiiRendererTestBasics::SubtestClearScreen()
{
  BeginFrame();

  switch (m_iFrame)
  {
    case 0:
      ClearScreen(xiiColor(1, 0, 0));
      break;
    case 1:
      ClearScreen(xiiColor(0, 1, 0));
      break;
    case 2:
      ClearScreen(xiiColor(0, 0, 1));
      break;
    case 3:
      ClearScreen(xiiColor(0.5f, 0.5f, 0.5f, 0.5f));
      break;
  }

  XII_TEST_IMAGE(m_iFrame, 1);

  EndFrame();

  return m_iFrame < 3 ? xiiTestAppRun::Continue : xiiTestAppRun::Quit;
}

void xiiRendererTestBasics::RenderObjects(xiiBitflags<xiiShaderBindFlags> ShaderBindFlags)
{
  xiiCamera cam;
  cam.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(xiiVec3(0, 0, 0), xiiVec3(0, 0, -1), xiiVec3(0, 1, 0));
  xiiMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  xiiMat4 mView = cam.GetViewMatrix();

  xiiMat4 mTransform, mOther, mRot;

  mRot.SetRotationMatrixX(xiiAngle::Degree(-90));

  mOther.SetScalingMatrix(xiiVec3(1.0f, 1.0f, 1.0f));
  mTransform.SetTranslationMatrix(xiiVec3(-0.3f, -0.3f, 0.0f));
  RenderObject(m_hLongBox, mProj * mView * mTransform * mOther, xiiColor(1, 0, 1, 0.25f), ShaderBindFlags);

  mOther.SetRotationMatrixX(xiiAngle::Degree(80.0f));
  mTransform.SetTranslationMatrix(xiiVec3(0.75f, 0, -1.8f));
  RenderObject(m_hTorus, mProj * mView * mTransform * mOther * mRot, xiiColor(1, 0, 0, 0.5f), ShaderBindFlags);

  mOther.SetIdentity();
  mTransform.SetTranslationMatrix(xiiVec3(0, 0.1f, -2.0f));
  RenderObject(m_hSphere, mProj * mView * mTransform * mOther, xiiColor(0, 1, 0, 0.75f), ShaderBindFlags);

  mOther.SetScalingMatrix(xiiVec3(1.5f, 1.0f, 1.0f));
  mTransform.SetTranslationMatrix(xiiVec3(-0.6f, -0.2f, -2.2f));
  RenderObject(m_hSphere2, mProj * mView * mTransform * mOther * mRot, xiiColor(0, 0, 1, 1), ShaderBindFlags);
}

void xiiRendererTestBasics::RenderLineObjects(xiiBitflags<xiiShaderBindFlags> ShaderBindFlags)
{
  xiiCamera cam;
  cam.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(xiiVec3(0, 0, 0), xiiVec3(0, 0, -1), xiiVec3(0, 1, 0));
  xiiMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  xiiMat4 mView = cam.GetViewMatrix();

  xiiMat4 mTransform, mOther, mRot;

  mRot.SetRotationMatrixX(xiiAngle::Degree(-90));

  mOther.SetScalingMatrix(xiiVec3(1.0f, 1.0f, 1.0f));
  mTransform.SetTranslationMatrix(xiiVec3(-0.3f, -0.3f, 0.0f));
  RenderObject(m_hLineBox, mProj * mView * mTransform * mOther, xiiColor(1, 0, 1, 0.25f), ShaderBindFlags);
}

static xiiRendererTestBasics g_Test;
