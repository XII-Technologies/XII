/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/TopLevelAS.h>

XII_CREATE_SIMPLE_TEST(Resources, TopLevelAS)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Descriptor defaults")
  {
    xiiGALTopLevelASCreationDescription description;
    XII_TEST_INT(description.m_uiMaxInstanceCount, 0U);
    XII_TEST_BOOL(description.m_Flags.IsNoFlagSet());
    XII_TEST_INT(description.m_uiCompactedSize, 0U);
    XII_TEST_INT(description.m_uiCommandQueueMask, XII_BIT(0));

    xiiGALTopLevelASBuildDescription buildDescription;
    XII_TEST_INT(buildDescription.m_uiInstanceCount, 0U);
    XII_TEST_BOOL(buildDescription.m_BindingMode == xiiGALHitGroupBindingMode::PerGeometry);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Feature-gated device creation and initial state")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      if (environment.GetDevice()->GetGraphicsDeviceAdapterProperties().m_Features.m_RayTracing != xiiGALDeviceFeatureState::Enabled)
        continue;

      xiiGALTopLevelASCreationDescription description;
      description.m_uiMaxInstanceCount = 4U;
      description.m_Flags              = xiiGALRayTracingBuildASFlags::PreferFastBuild | xiiGALRayTracingBuildASFlags::AllowUpdate;

      xiiSharedPtr<xiiGALTopLevelAS> pAccelerationStructure = environment.GetDevice()->CreateTopLevelAS(description);
      XII_TEST_BOOL(pAccelerationStructure != nullptr);
      if (pAccelerationStructure == nullptr)
        continue;

      XII_TEST_BOOL(pAccelerationStructure->GetDescription() == description);
      XII_TEST_BOOL(pAccelerationStructure->GetScratchBufferSizeDescription().m_uiBuild > 0U);
      XII_TEST_INT(pAccelerationStructure->GetBuildDescription().m_uiInstanceCount, 0U);

      const xiiGALTopLevelASInstanceDescription missing = pAccelerationStructure->GetInstanceDescription("MissingInstance");
      XII_TEST_BOOL(missing.m_pBottomLevelAS == nullptr);
      XII_TEST_INT(missing.m_uiContributionToHitGroupIndex, xiiInvalidIndex);
      XII_TEST_INT(missing.m_uiInstanceIndex, xiiInvalidIndex);

      pAccelerationStructure->SetDebugName("Unit Test TLAS");
      XII_TEST_STRING(pAccelerationStructure->GetDebugName(), "Unit Test TLAS");
    }
  }
}
