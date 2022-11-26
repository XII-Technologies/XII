#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/Progress.h>

XII_CREATE_SIMPLE_TEST(Utility, Progress)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simple progress")
  {
    xiiProgress progress;
    {
      xiiProgressRange progressRange = xiiProgressRange("TestProgress", 4, false, &progress);

      XII_TEST_FLOAT(progress.GetCompletion(), 0.0f, xiiMath::DefaultEpsilon<float>());
      XII_TEST_STRING(progress.GetMainDisplayText(), "TestProgress");

      progressRange.BeginNextStep("Step1");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.0f, xiiMath::DefaultEpsilon<float>());
      XII_TEST_STRING(progress.GetStepDisplayText(), "Step1");

      progressRange.BeginNextStep("Step2");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.25f, xiiMath::DefaultEpsilon<float>());
      XII_TEST_STRING(progress.GetStepDisplayText(), "Step2");

      progressRange.BeginNextStep("Step3");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.5f, xiiMath::DefaultEpsilon<float>());
      XII_TEST_STRING(progress.GetStepDisplayText(), "Step3");

      progressRange.BeginNextStep("Step4");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.75f, xiiMath::DefaultEpsilon<float>());
      XII_TEST_STRING(progress.GetStepDisplayText(), "Step4");
    }

    XII_TEST_FLOAT(progress.GetCompletion(), 1.0f, xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Weighted progress")
  {
    xiiProgress progress;
    {
      xiiProgressRange progressRange = xiiProgressRange("TestProgress", 4, false, &progress);
      progressRange.SetStepWeighting(2, 2.0f);

      XII_TEST_FLOAT(progress.GetCompletion(), 0.0f, xiiMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0+1", 2);
      XII_TEST_FLOAT(progress.GetCompletion(), 0.2f, xiiMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2"); // this step should have twice the weight as the other steps.
      XII_TEST_FLOAT(progress.GetCompletion(), 0.4f, xiiMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step3");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.8f, xiiMath::DefaultEpsilon<float>());
    }

    XII_TEST_FLOAT(progress.GetCompletion(), 1.0f, xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Nested progress")
  {
    xiiProgress progress;
    {
      xiiProgressRange progressRange = xiiProgressRange("TestProgress", 4, false, &progress);
      progressRange.SetStepWeighting(2, 2.0f);

      XII_TEST_FLOAT(progress.GetCompletion(), 0.0f, xiiMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.0f, xiiMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step1");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.2f, xiiMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.4f, xiiMath::DefaultEpsilon<float>());

      {
        xiiProgressRange nestedRange = xiiProgressRange("Nested", 5, false, &progress);
        nestedRange.SetStepWeighting(1, 4.0f);

        XII_TEST_FLOAT(progress.GetCompletion(), 0.4f, xiiMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep0");
        XII_TEST_FLOAT(progress.GetCompletion(), 0.4f, xiiMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep1");
        XII_TEST_FLOAT(progress.GetCompletion(), 0.45f, xiiMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep2");
        XII_TEST_FLOAT(progress.GetCompletion(), 0.65f, xiiMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep3");
        XII_TEST_FLOAT(progress.GetCompletion(), 0.7f, xiiMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep4");
        XII_TEST_FLOAT(progress.GetCompletion(), 0.75f, xiiMath::DefaultEpsilon<float>());
      }
      XII_TEST_FLOAT(progress.GetCompletion(), 0.8f, xiiMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step3");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.8f, xiiMath::DefaultEpsilon<float>());
    }

    XII_TEST_FLOAT(progress.GetCompletion(), 1.0f, xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Nested progress with manual completion")
  {
    xiiProgress progress;
    {
      xiiProgressRange progressRange = xiiProgressRange("TestProgress", 3, false, &progress);
      progressRange.SetStepWeighting(1, 2.0f);

      XII_TEST_FLOAT(progress.GetCompletion(), 0.0f, xiiMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.0f, xiiMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step1");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.25f, xiiMath::DefaultEpsilon<float>());

      {
        xiiProgressRange nestedRange = xiiProgressRange("Nested", false, &progress);

        XII_TEST_FLOAT(progress.GetCompletion(), 0.25f, xiiMath::DefaultEpsilon<float>());

        nestedRange.SetCompletion(0.5);
        XII_TEST_FLOAT(progress.GetCompletion(), 0.5f, xiiMath::DefaultEpsilon<float>());
      }
      XII_TEST_FLOAT(progress.GetCompletion(), 0.75f, xiiMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2");
      XII_TEST_FLOAT(progress.GetCompletion(), 0.75f, xiiMath::DefaultEpsilon<float>());
    }

    XII_TEST_FLOAT(progress.GetCompletion(), 1.0f, xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Progress Events")
  {
    xiiUInt32 uiNumProgressUpdatedEvents = 0;

    xiiProgress progress;
    progress.m_Events.AddEventHandler([&](const xiiProgressEvent& e) {
      if (e.m_Type == xiiProgressEvent::Type::ProgressChanged)
      {
        ++uiNumProgressUpdatedEvents;
        XII_TEST_FLOAT(e.m_pProgressbar->GetCompletion(), uiNumProgressUpdatedEvents * 0.25f, xiiMath::DefaultEpsilon<float>());
      }
    });

    {
      xiiProgressRange progressRange = xiiProgressRange("TestProgress", 4, false, &progress);

      progressRange.BeginNextStep("Step1");
      progressRange.BeginNextStep("Step2");
      progressRange.BeginNextStep("Step3");
      progressRange.BeginNextStep("Step4");
    }

    XII_TEST_FLOAT(progress.GetCompletion(), 1.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_INT(uiNumProgressUpdatedEvents, 4);
  }
}
