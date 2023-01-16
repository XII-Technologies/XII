#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/World/World.h>
#include <Foundation/Logging/Log.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <SampleGamePlugin/GameState/SampleGameState.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(SampleGameState, 1, xiiRTTIDefaultAllocator<SampleGameState>)
XII_END_DYNAMIC_REFLECTED_TYPE;

// BEGIN-DOCS-CODE-SNIPPET: confunc-impl
SampleGameState::SampleGameState() :
  m_ConFunc_Print("Print", "(string arg1): Prints 'arg1' to the log", xiiMakeDelegate(&SampleGameState::ConFunc_Print, this))
{
}

void SampleGameState::ConFunc_Print(xiiString sText)
{
  xiiLog::Info("Text: '{}'", sText);
}
// END-DOCS-CODE-SNIPPET

void SampleGameState::OnActivation(xiiWorld* pWorld, const xiiTransform* pStartPosition)
{
  XII_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, pStartPosition);

// BEGIN-DOCS-CODE-SNIPPET: imgui-alloc
#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (xiiImgui::GetSingleton() == nullptr)
  {
    XII_DEFAULT_NEW(xiiImgui);
  }
#endif
  // END-DOCS-CODE-SNIPPET
}

void SampleGameState::OnDeactivation()
{
  XII_LOG_BLOCK("GameState::Deactivate");

// BEGIN-DOCS-CODE-SNIPPET: imgui-dealloc
#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (xiiImgui::GetSingleton() != nullptr)
  {
    xiiImgui* pImgui = xiiImgui::GetSingleton();
    XII_DEFAULT_DELETE(pImgui);
  }
#endif
  // END-DOCS-CODE-SNIPPET

  SUPER::OnDeactivation();
}

// BEGIN-DOCS-CODE-SNIPPET: cvar-1
#include <Foundation/Configuration/CVar.h>

xiiCVarBool cvar_DebugDisplay("Game.DebugDisplay", false, xiiCVarFlags::Default, "Whether the game should display debug geometry.");
// END-DOCS-CODE-SNIPPET

void SampleGameState::AfterWorldUpdate()
{
  SUPER::AfterWorldUpdate();

  // BEGIN-DOCS-CODE-SNIPPET: cvar-2
  if (cvar_DebugDisplay)
  {
    xiiDebugRenderer::DrawLineSphere(m_pMainWorld, xiiBoundingSphere(xiiVec3::ZeroVector(), 1.0f), xiiColor::Orange);
  }
  // END-DOCS-CODE-SNIPPET

  xiiDebugRenderer::Draw2DText(m_pMainWorld, "Press 'O' to spawn objects", xiiVec2I32(10, 10), xiiColor::White);
  xiiDebugRenderer::Draw2DText(m_pMainWorld, "Press 'P' to remove objects", xiiVec2I32(10, 30), xiiColor::White);
}

void SampleGameState::BeforeWorldUpdate()
{
  XII_LOCK(m_pMainWorld->GetWriteMarker());

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (xiiImgui::GetSingleton() != nullptr)
  {
    static bool  stats  = false;
    static bool  window = true;
    static float color[3];
    static float slider = 0.5f;

    // BEGIN-DOCS-CODE-SNIPPET: imgui-activate
    xiiImgui::GetSingleton()->SetCurrentContextForView(m_hMainView);
    // END-DOCS-CODE-SNIPPET

    xiiImgui::GetSingleton()->SetPassInputToImgui(false); // reset this state, to deactivate input processing as long as SampleGameState::ProcessInput() isn't called again

    // BEGIN-DOCS-CODE-SNIPPET: imgui-panel
    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
    ImGui::Begin("Imgui Window", &window);
    ImGui::Text("Hello World!");
    ImGui::SliderFloat("Slider", &slider, 0.0f, 1.0f);
    ImGui::ColorEdit3("Color", color);


    if (ImGui::Button("Toggle Stats"))
    {
      stats = !stats;
    }

    if (stats)
    {
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }

    ImGui::End();
    // END-DOCS-CODE-SNIPPET
  }
#endif
}

xiiGameStatePriority SampleGameState::DeterminePriority(xiiWorld* pWorld) const
{
  return xiiGameStatePriority::Default;
}

void SampleGameState::ConfigureMainWindowInputDevices(xiiWindow* pWindow)
{
  SUPER::ConfigureMainWindowInputDevices(pWindow);

  // setup devices here
}

// BEGIN-DOCS-CODE-SNIPPET: input-config
static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  xiiInputActionConfig cfg;
  cfg.m_bApplyTimeScaling    = true;
  cfg.m_sInputSlotTrigger[0] = szKey1;
  cfg.m_sInputSlotTrigger[1] = szKey2;
  cfg.m_sInputSlotTrigger[2] = szKey3;

  xiiInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void SampleGameState::ConfigureInputActions()
{
  SUPER::ConfigureInputActions();

  RegisterInputAction("SamplePlugin", "SpawnObject", xiiInputSlot_KeyO, xiiInputSlot_Controller0_ButtonA, xiiInputSlot_MouseButton2);
  RegisterInputAction("SamplePlugin", "DeleteObject", xiiInputSlot_KeyP, xiiInputSlot_Controller0_ButtonB);
}
// END-DOCS-CODE-SNIPPET

void SampleGameState::ProcessInput()
{
#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (xiiImgui::GetSingleton())
  {
    // SampleGameState::ProcessInput() isn't necessary called each frame, if the application decides that the game-state
    // should not get any input at the moment (this happens for instance, when the xiiConsole is open)
    // so only enable it when the game state gets input (and reset it in BeforeWorldUpdate())
    xiiImgui::GetSingleton()->SetPassInputToImgui(true);

    // if the UI wants input, do not process other game state input
    if (xiiImgui::GetSingleton()->WantsInput())
      return;
  }
#endif

  SUPER::ProcessInput();

  xiiWorld* pWorld = m_pMainWorld;

  if (xiiInputManager::GetInputActionState("SamplePlugin", "SpawnObject") == xiiKeyState::Pressed)
  {
    const xiiVec3 pos = GetMainCamera()->GetCenterPosition() + GetMainCamera()->GetCenterDirForwards();

    // make sure we are allowed to modify the world
    XII_LOCK(pWorld->GetWriteMarker());

    // create a game object at the desired position
    xiiGameObjectDesc desc;
    desc.m_LocalPosition = pos;

    xiiGameObject*      pObject = nullptr;
    xiiGameObjectHandle hObject = pWorld->CreateObject(desc, pObject);

    m_SpawnedObjects.PushBack(hObject);

    // attach a mesh component to the object
    // BEGIN-DOCS-CODE-SNIPPET: create-component
    xiiMeshComponent* pMesh;
    pWorld->GetOrCreateComponentManager<xiiMeshComponentManager>()->CreateComponent(pObject, pMesh);
    // END-DOCS-CODE-SNIPPET

    // Set the mesh to use.
    // Here we use a path relative to the project directory.
    // We have to reference the 'transformed' file, not the source file.
    // This would break if the source asset is moved or renamed.
    pMesh->SetMeshFile("AssetCache/Common/Meshes/Sphere.xiiMesh");

    // here we use the asset GUID to reference the transformed asset
    // we can copy the GUID from the asset browser
    // the GUID is stable even if the source asset gets moved or renamed
    // using asset collections we could also give a nice name like 'Blue Material' to this asset
    xiiMaterialResourceHandle hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>("{ aa1c5601-bc43-fbf8-4e07-6a3df3af51e7 }");

    // override the mesh material in the first slot with something different
    pMesh->SetMaterial(0, hMaterial);
  }

  if (xiiInputManager::GetInputActionState("SamplePlugin", "DeleteObject") == xiiKeyState::Pressed)
  {
    if (!m_SpawnedObjects.IsEmpty())
    {
      // make sure we are allowed to modify the world
      XII_LOCK(pWorld->GetWriteMarker());

      xiiGameObjectHandle hObject = m_SpawnedObjects.PeekBack();
      m_SpawnedObjects.PopBack();

      // this is only for demonstration purposes, removing the object will delete all attached components as well
      xiiGameObject* pObject = nullptr;
      if (pWorld->TryGetObject(hObject, pObject))
      {
        // BEGIN-DOCS-CODE-SNIPPET: find-component
        xiiMeshComponent* pMesh = nullptr;
        if (pObject->TryGetComponentOfBaseType(pMesh))
        {
          pMesh->DeleteComponent();
        }
        // END-DOCS-CODE-SNIPPET
      }

      // delete the object, all its children and attached components
      pWorld->DeleteObjectDelayed(hObject);
    }
  }
}


void SampleGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  // do custom camera setup here
}
