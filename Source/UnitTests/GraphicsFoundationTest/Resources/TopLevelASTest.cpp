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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Portable instance ABI and packing")
  {
    xiiGALTLASInstanceData instance;
    XII_TEST_INT(sizeof(instance), 64U);
    XII_TEST_INT(instance.GetMask(), 0xFFU);

    const xiiMat4 transform = xiiMat4::MakeTranslation(xiiVec3(3.0f, -2.0f, 7.0f));
    instance.SetTransform(transform);
    instance.SetInstanceID(0x00ABCDEFU);
    instance.SetMask(0x5AU);
    instance.SetHitGroupContribution(0x00123456U);
    instance.SetFlags(xiiGALRayTracingInstanceFlags::TriangleCullDisable | xiiGALRayTracingInstanceFlags::ForceOpaque);
    instance.m_uiBottomLevelASDeviceAddress = 0x123456789ABCDEF0ULL;

    XII_TEST_BOOL(instance.GetTransform().IsEqual(transform, 0.0f));
    XII_TEST_INT(instance.GetInstanceID(), 0x00ABCDEFU);
    XII_TEST_INT(instance.GetMask(), 0x5AU);
    XII_TEST_INT(instance.GetHitGroupContribution(), 0x00123456U);
    XII_TEST_BOOL(instance.GetFlags().AreAllSet(xiiGALRayTracingInstanceFlags::TriangleCullDisable | xiiGALRayTracingInstanceFlags::ForceOpaque));
    XII_TEST_INT(instance.m_uiBottomLevelASDeviceAddress, 0x123456789ABCDEF0ULL);

    const xiiRTTI* pType = xiiGetStaticRTTI<xiiGALTLASInstanceData>();
    XII_TEST_BOOL(pType->FindPropertyByName("Transform") != nullptr);
    XII_TEST_BOOL(pType->FindPropertyByName("BottomLevelASDeviceAddress") != nullptr);
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
