/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <DearImguiPlugin/DearImguiPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

#include <Imgui/imgui.h>

class xiiView;
class xiiWorld;

struct xiiGameApplicationExecutionEvent;
struct xiiRenderWorldModuleExtractionEvent;

/// Singleton class through which one can control the third-party library 'Dear Imgui'.
class XII_DEARIMGUIPLUGIN_DLL xiiImguiSingleton
{
  XII_DECLARE_SINGLETON(xiiImguiSingleton);

public:
  xiiImguiSingleton();
  ~xiiImguiSingleton();

  /// Returns the value that was passed to BeginFrame(). Useful for positioning UI elements.
  XII_ALWAYS_INLINE xiiSizeU32 GetCurrentWindowResolution() const { return m_CurrentWindowResolution; }

  /// When this is disabled, the GUI will be rendered, but it will not react to any input. Useful if something else shall get
  /// exclusive input.
  XII_ALWAYS_INLINE void SetPassInputToImgui(bool bPassInput) { m_bPassInputToImgui = bPassInput; }

  /// If this returns true, the GUI wants to use the input, and thus you might want to not use the input for anything else.
  ///
  /// This is the case when the mouse hovers over any window or a text field has keyboard focus.
  XII_ALWAYS_INLINE bool WantsInput() const { return m_bImguiWantsInput; }

  /// Returns the shared font atlas
  XII_ALWAYS_INLINE ImFontAtlas& GetFontAtlas() { return *m_pSharedFontAtlas; }

  XII_ALWAYS_INLINE xiiEvent<const xiiView*, xiiMutex>& GetUpdateEvent() { return s_UpdateEvent; }

private:
  void Startup();
  void Shutdown();

  void OnViewModified(const xiiRenderWorldModuleExtractionEvent& viewEvent);

  ImGuiContext* CreateContext();
  void          SetupContext(const xiiView* pView);
  void          GameApplicationEventHandler(const xiiGameApplicationExecutionEvent& e);

private:
  xiiProxyAllocator m_Allocator;

  xiiEventSubscriptionID m_ViewModifiedEventSubscriptionID;

  bool                                          m_bPassInputToImgui = true;
  bool                                          m_bImguiWantsInput  = false;
  xiiSizeU32                                    m_CurrentWindowResolution;
  xiiHybridArray<xiiTexture2DResourceHandle, 4> m_Textures;

  xiiUniquePtr<ImFontAtlas> m_pSharedFontAtlas;

  static xiiEvent<const xiiView*, xiiMutex> s_UpdateEvent;
};
