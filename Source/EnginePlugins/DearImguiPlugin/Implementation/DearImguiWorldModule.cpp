#include <DearImguiPlugin/DearImguiPluginPCH.h>

#include <DearImguiPlugin/DearImguiWorldModule.h>
#include <DearImguiPlugin/DearImguiSingleton.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDearImguiWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_WORLD_MODULE(xiiDearImguiWorldModule);

xiiDearImguiWorldModule::xiiDearImguiWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiDearImguiWorldModule::~xiiDearImguiWorldModule() = default;

void xiiDearImguiWorldModule::Initialize()
{
  // Register ExtractImguiUpdate (runs on post-async single-threaded, after all extraction is complete).
  {
    auto description                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiDearImguiWorldModule::ExtractImguiUpdate, this);
    description.m_Phase                     = xiiWorldUpdatePhase::PostAsync;
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }
}

void xiiDearImguiWorldModule::Deinitialize()
{
  for (auto it = m_ViewToContextTable.GetIterator(); it.IsValid(); ++it)
  {
    Context& context = it.Value();

    ImGui::DestroyContext(context.m_pImGuiContext);

    context.m_pImGuiContext = nullptr;
  }
  m_ViewToContextTable.Clear();
}

void xiiDearImguiWorldModule::OnSimulationStarted()
{
}

void xiiDearImguiWorldModule::SetCurrentContextForView(const xiiViewHandle& hView)
{
  Context& context = m_ViewToContextTable[hView];

  if (context.m_pImGuiContext == nullptr)
  {
    context.m_pImGuiContext = xiiImguiSingleton::GetSingleton()->CreateContext();
  }

  ImGui::SetCurrentContext(context.m_pImGuiContext);
}

void xiiDearImguiWorldModule::ExtractImguiUpdate(const xiiWorldModule::UpdateContext& context)
{
  XII_IGNORE_UNUSED(context);
}

XII_STATICLINK_FILE(DearImguiPlugin, DearImguiPlugin_Implementation_DearImguiWorldModule);
