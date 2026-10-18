/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

namespace
{
  class GeometryInitializer final : public xiiThread
  {
  public:
    explicit GeometryInitializer(xiiThreadSignal& start) : xiiThread("Concurrent geometry initialization"), m_Start(start) {}
    xiiThreadSignal            m_Ready;
    bool                       m_bSucceeded = true;
    xiiSharedPtr<xiiGALBuffer> m_Metadata, m_Meshlets;

  private:
    xiiUInt32 Run() override
    {
      m_Ready.RaiseSignal();
      m_Start.WaitForSignal();
      for (xiiUInt32 iteration = 0; iteration < 32; ++iteration)
      {
        if (xiiGeometryResidencyManager::EnsureInitialized().Failed())
        {
          m_bSucceeded = false;
          break;
        }
        auto metadata = xiiGeometryResidencyManager::GetMetadataBuffer();
        auto meshlets = xiiGeometryResidencyManager::GetMeshletMetadataBuffer();
        m_bSucceeded &= xiiGeometryResidencyManager::IsInitialized() && metadata && meshlets;
        if (iteration)
          m_bSucceeded &= metadata == m_Metadata && meshlets == m_Meshlets;
        m_Metadata = metadata;
        m_Meshlets = meshlets;
      }
      return 0;
    }
    xiiThreadSignal& m_Start;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(PipelineResources, ConcurrentGeometryInitialization)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized());
  xiiGeometryResidencyDescription description;
  description.m_uiMaxGeometries = 64;
  description.m_uiMaxMeshlets   = 64;
  XII_TEST_RESULT(xiiGeometryResidencyManager::Configure(description));
  xiiThreadSignal                   start(xiiThreadSignal::Mode::ManualReset);
  xiiUniquePtr<GeometryInitializer> initializers[4];
  for (auto& initializer : initializers)
  {
    initializer = XII_DEFAULT_NEW(GeometryInitializer, start);
    initializer->Start();
  }
  for (const auto& initializer : initializers)
    XII_TEST_BOOL(initializer->m_Ready.WaitForSignal(xiiTime::MakeFromSeconds(5)) == xiiThreadSignal::WaitResult::Signaled);
  start.RaiseSignal();
  for (auto& initializer : initializers)
    initializer->Join();
  for (const auto& initializer : initializers)
  {
    XII_TEST_BOOL(initializer->m_bSucceeded);
    XII_TEST_BOOL(initializer->m_Metadata == initializers[0]->m_Metadata);
    XII_TEST_BOOL(initializer->m_Meshlets == initializers[0]->m_Meshlets);
    XII_TEST_INT(initializer->m_Metadata->GetDescription().m_uiSize, sizeof(xiiGpuGeometryRecord) * description.m_uiMaxGeometries * description.m_uiFramesInFlight);
  }
  XII_TEST_INT(xiiGeometryResidencyManager::GetStats().m_uiGeometryCount, 0);
}
