/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <DearImguiPlugin/DearImguiPluginPCH.h>

#include <DearImguiPlugin/DearImguiSubsystem.h>
#include <DearImguiPlugin/Implementation/DearImguiContextData.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/UniquePtr.h>

struct xiiImguiSubsystem::State
{
  xiiIdTable<xiiImguiContextId, xiiUniquePtr<xiiImguiContext>> m_Contexts;
  ImTextureID                                                  m_uiNextTexture = 1;
};

xiiImguiSubsystem::State* xiiImguiSubsystem::s_pState = nullptr;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(DearImguiPlugin, DearImgui)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation", "Core", "RenderGraphManager"
  END_SUBSYSTEM_DEPENDENCIES
  ON_CORESYSTEMS_STARTUP { xiiImguiSubsystem::Startup(); }
  ON_HIGHLEVELSYSTEMS_STARTUP { xiiImguiSubsystem::Startup(); }
  ON_HIGHLEVELSYSTEMS_SHUTDOWN { xiiImguiSubsystem::ClearContexts(); }
  ON_CORESYSTEMS_SHUTDOWN { xiiImguiSubsystem::Shutdown(); }
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

void xiiImguiSubsystem::Startup()
{
  IMGUI_CHECKVERSION();
  XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "ImGui startup must run on the main thread.");
  if (!s_pState)
    s_pState = XII_DEFAULT_NEW(State);
}

void xiiImguiSubsystem::Shutdown()
{
  ClearContexts();
  XII_DEFAULT_DELETE(s_pState);
}

void xiiImguiSubsystem::ClearContexts()
{
  XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "ImGui shutdown must run on the main thread.");
  if (!s_pState)
    return;
  for (auto it = s_pState->m_Contexts.GetIterator(); it.IsValid(); ++it)
    XII_ASSERT_ALWAYS(it.Value()->m_uiScopes == 0, "An ImGui activation scope outlived subsystem shutdown.");
  // Remove individually to preserve generations across high-level shutdown/restart.
  while (!s_pState->m_Contexts.IsEmpty())
    s_pState->m_Contexts.Remove(s_pState->m_Contexts.GetIterator().Id());
}

xiiImguiContext* xiiImguiSubsystem::AcquireContext(xiiImguiContextHandle hContext)
{
  if (!xiiThreadUtils::IsMainThread() || !s_pState)
    return nullptr;
  xiiUniquePtr<xiiImguiContext>* pContext = nullptr;
  return s_pState->m_Contexts.TryGetValue(hContext.GetInternalID(), pContext) ? pContext->Borrow() : nullptr;
}

bool xiiImguiSubsystem::IsContextValid(xiiImguiContextHandle hContext)
{
  return AcquireContext(hContext) != nullptr;
}

xiiResult xiiImguiSubsystem::CreateContext(xiiImguiContextHandle& out_hContext)
{
  out_hContext.Invalidate();
  if (!xiiThreadUtils::IsMainThread() || !s_pState)
    return XII_FAILURE;
  out_hContext = xiiImguiContextHandle(s_pState->m_Contexts.Insert(XII_DEFAULT_NEW(xiiImguiContext)));
  return XII_SUCCESS;
}

xiiResult xiiImguiSubsystem::DestroyContext(xiiImguiContextHandle hContext)
{
  auto* pContext = AcquireContext(hContext);
  if (!pContext || pContext->m_uiScopes)
    return XII_FAILURE;
  return s_pState->m_Contexts.Remove(hContext.GetInternalID()) ? XII_SUCCESS : XII_FAILURE;
}

ImTextureID xiiImguiSubsystem::AllocateTextureId()
{
  // Never reuse texture tokens during the subsystem lifetime, even across contexts.
  XII_ASSERT_ALWAYS(s_pState->m_uiNextTexture != ImTextureID_Invalid, "ImGui texture token space exhausted.");
  return s_pState->m_uiNextTexture++;
}

xiiResult xiiImguiSubsystem::RegisterTexture(xiiImguiContextHandle hContext, xiiSharedPtr<xiiGALTexture> pTexture, ImTextureID& out_id)
{
  out_id         = ImTextureID_Invalid;
  auto* pContext = AcquireContext(hContext);
  if (!pContext || !pTexture)
    return XII_FAILURE;
  const auto& desc = pTexture->GetDescription();
  if (desc.m_Type != xiiGALResourceDimension::Texture2D || desc.m_uiArraySizeOrDepth != 1 || desc.m_uiSampleCount != 1 ||
      !desc.m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || xiiGALResourceFormat::IsDepthFormat(desc.m_Format))
    return XII_FAILURE;
  xiiSharedPtr<xiiImguiTexture> texture = XII_DEFAULT_NEW(xiiImguiTexture);
  texture->m_pTexture                   = std::move(pTexture);
  out_id                                = pContext->AddTexture(std::move(texture));
  return XII_SUCCESS;
}

xiiResult xiiImguiSubsystem::UnregisterTexture(xiiImguiContextHandle hContext, ImTextureID id)
{
  auto* pContext = AcquireContext(hContext);
  return pContext ? pContext->RemoveTexture(id) : XII_FAILURE;
}

XII_STATICLINK_FILE(DearImguiPlugin, DearImguiPlugin_Implementation_DearImguiSubsystem);
