/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Lighting/IESProfileResource.h>

XII_CREATE_SIMPLE_TEST(Lighting, IESProfileResource)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Parse rotationally symmetric Type-C profile")
  {
    constexpr const char* szProfile =
      "IESNA:LM-63-2002\n"
      "[TEST] XII synthetic profile\n"
      "TILT=NONE\n"
      "1 1000 2 3 1 1 2 0 0 0\n"
      "1 1 10\n"
      "0 90 180\n"
      "0\n"
      "100 50 0\n";

    xiiIESProfileResourceDescriptor descriptor;
    xiiStringBuilder error;
    XII_TEST_BOOL_MSG(descriptor.ParseLM63(szProfile, &error).Succeeded(), error);
    XII_TEST_BOOL(descriptor.IsValid());
    XII_TEST_FLOAT(descriptor.m_fMaximumCandela, 200.0f, 0.001f);
    XII_TEST_FLOAT(descriptor.m_fReportedLumens, 1000.0f, 0.001f);
    XII_TEST_FLOAT(descriptor.Sample(xiiAngle::MakeFromDegree(0.0f), xiiAngle::MakeFromDegree(0.0f)), 1.0f, 0.001f);
    XII_TEST_FLOAT(descriptor.Sample(xiiAngle::MakeFromDegree(90.0f), xiiAngle::MakeFromDegree(217.0f)), 0.5f, 0.02f);
    XII_TEST_FLOAT(descriptor.Sample(xiiAngle::MakeFromDegree(180.0f), xiiAngle::MakeFromDegree(0.0f)), 0.0f, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Preserve bilateral asymmetry")
  {
    constexpr const char* szProfile =
      "IESNA:LM-63-2002\n"
      "TILT=NONE\n"
      "1 500 1 2 3 1 2 0 0 0\n"
      "1 1 5\n"
      "0 180\n"
      "0 90 180\n"
      "100 0\n"
      "50 0\n"
      "25 0\n";

    xiiIESProfileResourceDescriptor descriptor;
    XII_TEST_BOOL(descriptor.ParseLM63(szProfile).Succeeded());
    XII_TEST_FLOAT(descriptor.Sample(xiiAngle::MakeZero(), xiiAngle::MakeFromDegree(0.0f)), 1.0f, 0.001f);
    XII_TEST_FLOAT(descriptor.Sample(xiiAngle::MakeZero(), xiiAngle::MakeFromDegree(90.0f)), 0.5f, 0.02f);
    XII_TEST_FLOAT(descriptor.Sample(xiiAngle::MakeZero(), xiiAngle::MakeFromDegree(180.0f)), 0.25f, 0.02f);
    XII_TEST_FLOAT(descriptor.Sample(xiiAngle::MakeZero(), xiiAngle::MakeFromDegree(270.0f)), 0.5f, 0.02f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reject unsupported tilt data")
  {
    xiiIESProfileResourceDescriptor descriptor;
    xiiStringBuilder error;
    XII_TEST_BOOL(descriptor.ParseLM63("IESNA:LM-63-2002\nTILT=INCLUDE\n", &error).Failed());
    XII_TEST_BOOL(!descriptor.IsValid());
    XII_TEST_BOOL(!error.IsEmpty());
  }
}

