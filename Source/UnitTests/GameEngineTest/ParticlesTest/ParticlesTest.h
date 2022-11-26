#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class xiiGameEngineTestApplication_Particles : public xiiGameEngineTestApplication
{
public:
  xiiGameEngineTestApplication_Particles();

  void          SetupSceneSubTest(const char* szFile);
  void          SetupParticleSubTest(const char* szFile);
  xiiTestAppRun ExecParticleSubTest(xiiInt32 iCurFrame);

  xiiUInt32 m_uiImageCompareThreshold = 110;
};

class xiiGameEngineTestParticles : public xiiGameEngineTest
{
  using SUPER = xiiGameEngineTest;

public:
  virtual const char*                   GetTestName() const override;
  virtual xiiGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    BillboardRenderer,
    ColorGradientBehavior,
    FliesBehavior,
    GravityBehavior,
    LightRenderer,
    MeshRenderer,
    RaycastBehavior,
    SizeCurveBehavior,
    TrailRenderer,
    VelocityBehavior,
    EffectRenderer,
    BoxPositionInitializer,
    SpherePositionInitializer,
    CylinderPositionInitializer,
    RandomColorInitializer,
    RandomSizeInitializer,
    RotationSpeedInitializer,
    VelocityConeInitializer,
    BurstEmitter,
    ContinuousEmitter,
    OnEventEmitter,
    QuadRotatingOrtho,
    QuadFixedEmDir,
    QuadAxisEmDir,

    Billboards,
    PullAlongBehavior,
    DistanceEmitter,
    SharedInstances,
    EventReactionEffect,
    LocalSpaceSim,
  };

  virtual void          SetupSubTests() override;
  virtual xiiResult     InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;
  xiiUInt32             GetImageCompareThreshold(xiiInt32 iIdentifier);

  xiiInt32                                m_iFrame          = 0;
  xiiGameEngineTestApplication_Particles* m_pOwnApplication = nullptr;
};
