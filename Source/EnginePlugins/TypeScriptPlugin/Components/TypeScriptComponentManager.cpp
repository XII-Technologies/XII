#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>

xiiTypeScriptComponentManager::xiiTypeScriptComponentManager(xiiWorld* pWorld) :
  SUPER(pWorld)
{
}

xiiTypeScriptComponentManager::~xiiTypeScriptComponentManager() = default;

void xiiTypeScriptComponentManager::Initialize()
{
  SUPER::Initialize();

  auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiTypeScriptComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase                     = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);
}

void xiiTypeScriptComponentManager::Deinitialize()
{
  SUPER::Deinitialize();
}

void xiiTypeScriptComponentManager::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_TsBinding.Initialize(*GetWorld()).IgnoreResult();
}

void xiiTypeScriptComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  XII_PROFILE_SCOPE("TypeScript Update");

  m_TsBinding.Update();

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
    {
      it->Update(m_TsBinding);
    }
  }

  m_TsBinding.CleanupStash(10);
}
