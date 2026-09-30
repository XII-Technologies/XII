/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Lighting/DisplayOutput.h>

XII_CREATE_SIMPLE_TEST(Lighting, DisplayOutput)
{
  XII_TEST_BOOL(xiiDisplayOutputManager::IsInitialized());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Validated subsystem defaults")
  {
    const xiiDisplayOutputSettings original = xiiDisplayOutputManager::GetDefaults();
    const xiiUInt64                uiRevision = xiiDisplayOutputManager::GetDefaultsRevision();
    XII_TEST_BOOL(xiiDisplayOutputManager::IsValid(original));

    xiiDisplayOutputSettings modified = original;
    modified.m_OutputMode              = xiiDisplayOutputMode::HDR10PQ;
    modified.m_fPaperWhiteNits         = 200.0f;
    modified.m_fMaximumDisplayNits     = 1200.0f;
    modified.m_Exposure.m_fExposureCompensation = 1.0f;

    XII_TEST_BOOL(xiiDisplayOutputManager::ConfigureDefaults(modified).Succeeded());
    XII_TEST_BOOL(xiiDisplayOutputManager::GetDefaultsRevision() > uiRevision);

    const xiiDisplayOutputSettings resolved = xiiDisplayOutputManager::GetDefaults();
    XII_TEST_INT(resolved.m_OutputMode.GetValue(), xiiDisplayOutputMode::HDR10PQ);
    XII_TEST_FLOAT(resolved.m_fMaximumDisplayNits, 1200.0f, 0.0f);
    XII_TEST_FLOAT(resolved.m_Exposure.m_fExposureCompensation, 1.0f, 0.0f);

    XII_TEST_BOOL(xiiDisplayOutputManager::ConfigureDefaults(original).Succeeded());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reject invalid calibrations")
  {
    xiiDisplayOutputSettings invalid = xiiDisplayOutputManager::GetDefaults();
    invalid.m_fMaximumDisplayNits = invalid.m_fPaperWhiteNits - 1.0f;
    XII_TEST_BOOL(!xiiDisplayOutputManager::IsValid(invalid));
    XII_TEST_BOOL(xiiDisplayOutputManager::ConfigureDefaults(invalid).Failed());

    invalid = xiiDisplayOutputManager::GetDefaults();
    invalid.m_Exposure.m_fLowPercentile = invalid.m_Exposure.m_fHighPercentile;
    XII_TEST_BOOL(!xiiDisplayOutputManager::IsValid(invalid));
  }
}

