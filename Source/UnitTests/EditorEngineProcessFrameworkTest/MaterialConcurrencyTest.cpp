/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

namespace
{
  class MaterialRequest : public xiiThread
  {
  public:
    MaterialRequest() : xiiThread("Concurrent material acquisition") {}

    xiiThreadSignal*          m_pStart   = nullptr;
    xiiThreadSignal*          m_pRelease = nullptr;
    xiiThreadSignal           m_Acquired;
    xiiMaterialResourceHandle m_Resource;
    xiiMaterialGpuHandle      m_Result;

  private:
    xiiUInt32 Run() override
    {
      m_pStart->WaitForSignal();
      m_Result = xiiMaterialManager::AcquireMaterialResource(m_Resource);
      m_Acquired.RaiseSignal();
      m_pRelease->WaitForSignal();
      xiiMaterialManager::ReleaseMaterialResource(m_Resource);
      return 0;
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, MaterialConcurrency)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Inherited defaults remain sparse and instance changes stay local")
  {
    xiiMaterialResourceDescriptor rootDescription;
    rootDescription.m_fRoughness       = 0.23f;
    auto                          root = xiiResourceManager::CreateResource<xiiMaterialResource>("MaterialInheritance.Root", std::move(rootDescription));
    xiiMaterialResourceDescriptor childDescription;
    childDescription.m_hBaseMaterial           = root;
    auto                                 child = xiiResourceManager::CreateResource<xiiMaterialResource>("MaterialInheritance.Child", std::move(childDescription));
    xiiResourceLock<xiiMaterialResource> material(child, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    XII_TEST_FLOAT(material->GetParameter(xiiTempHashedString("Roughness")).ConvertTo<float>(), 0.23f, 0);
    auto instance = material->CreateInstance();
    XII_TEST_BOOL(instance != nullptr);
    XII_TEST_RESULT(instance->SetParameter("Roughness", 0.8f));
    XII_TEST_FLOAT(material->GetParameter(xiiTempHashedString("Roughness")).ConvertTo<float>(), 0.23f, 0);
    xiiResourceLock<xiiMaterialResource> parent(root, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    parent->SetParameter("Roughness", 0.4f);
    parent->SetParameter("BaseColor", xiiColor::Red);
    XII_TEST_FLOAT(material->GetParameter(xiiTempHashedString("Roughness")).ConvertTo<float>(), 0.4f, 0);
    XII_TEST_RESULT(instance->RebindSchema(material->GetSchema(), material->GetRuntimeState()));
    XII_TEST_FLOAT(instance->GetParameter(xiiMaterialParameterId::Make("Roughness")).ConvertTo<float>(), 0.8f, 0);
    XII_TEST_BOOL(instance->GetParameter(xiiMaterialParameterId::Make("BaseColor")).ConvertTo<xiiColor>() == xiiColor::Red);
  }

  xiiMaterialResourceDescriptor        description;
  const auto                           material = xiiResourceManager::CreateResource<xiiMaterialResource>("Concurrent material resource", std::move(description));
  xiiResourceLock<xiiMaterialResource> loaded(material, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    return;

  xiiThreadSignal start(xiiThreadSignal::Mode::ManualReset);
  xiiThreadSignal release(xiiThreadSignal::Mode::ManualReset);
  MaterialRequest requests[8];
  for (auto& request : requests)
  {
    request.m_pStart   = &start;
    request.m_pRelease = &release;
    request.m_Resource = material;
    request.Start();
  }
  start.RaiseSignal();
  for (auto& request : requests)
    request.m_Acquired.WaitForSignal();
  for (const auto& request : requests)
  {
    XII_TEST_BOOL(!request.m_Result.IsInvalidated());
    XII_TEST_BOOL(request.m_Result == requests[0].m_Result);
  }
  const auto retained = xiiMaterialManager::AcquireMaterialResource(material);
  XII_TEST_BOOL(retained == requests[0].m_Result);
  release.RaiseSignal();
  for (auto& request : requests)
    request.Join();
  XII_TEST_BOOL(xiiMaterialManager::GetGpuStorage().GetMaterial(retained) != nullptr);
  xiiMaterialManager::ReleaseMaterialResource(material);
  XII_TEST_BOOL(xiiMaterialManager::GetGpuStorage().GetMaterial(retained) == nullptr);
}
