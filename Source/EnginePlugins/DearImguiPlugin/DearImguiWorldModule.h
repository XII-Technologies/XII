#pragma once

#include <DearImguiPlugin/DearImguiPluginDLL.h>

#include <Core/World/WorldModule.h>

class xiiView;

struct xiiMsgExtractImguiUpdate;
struct ImGuiContext;

class XII_DEARIMGUIPLUGIN_DLL xiiDearImguiWorldModule : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();

  XII_ADD_DYNAMIC_REFLECTION(xiiDearImguiWorldModule, xiiWorldModule);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiDearImguiWorldModule);

public:
  xiiDearImguiWorldModule(xiiWorld* pWorld);

  virtual ~xiiDearImguiWorldModule();

  virtual void Initialize() override;

  virtual void Deinitialize() override;

  virtual void OnSimulationStarted() override;

  /// \brief Sets the ImGui context for the given view.
  void SetCurrentContextForView(const xiiViewHandle& hView);

private:
  void ExtractImguiUpdate(const xiiWorldModule::UpdateContext& context);

private:
  struct Context
  {
    ImGuiContext* m_pImGuiContext = nullptr;
  };

  xiiHashTable<xiiViewHandle, Context> m_ViewToContextTable;
};
