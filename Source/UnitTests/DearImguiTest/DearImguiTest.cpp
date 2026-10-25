/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Input/InputDevice.h>
#include <DearImguiPlugin/DearImguiInput.h>
#include <DearImguiPlugin/DearImguiRenderer.h>
#include <DearImguiPlugin/DearImguiSubsystem.h>
#include <DearImguiPlugin/Pipeline/DearImguiRenderPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <TestFramework/Utilities/TestSetup.h>

XII_TESTFRAMEWORK_ENTRY_POINT("DearImguiTest", "Dear ImGui Tests")
XII_CREATE_SIMPLE_TEST_GROUP(DearImgui);

namespace
{
  class InputSource : public xiiInputDevice
  {
  public:
    void Slot(xiiStringView slot, float value) { ReportInputSlotValue(slot, value); }
    void Character(xiiUInt32 value) { ReportInputCharacter(value); }

  private:
    void InitializeDevice() override {}
    void RegisterInputSlots() override {}
    void UpdateInputSlotValues() override {}
  };
  xiiImguiFrameDescription Description()
  {
    xiiImguiFrameDescription description;
    description.m_vDisplaySize = {640, 480};
    return description;
  }

  xiiSharedPtr<const xiiImguiFrame> Draw(xiiImguiContextHandle context, bool widgets = true)
  {
    xiiSharedPtr<const xiiImguiFrame> frame;
    xiiImguiScopedContext             scope(context);
    if (!XII_TEST_BOOL(scope.IsValid()) || !XII_TEST_RESULT(scope.BeginFrame(Description())))
      return {};
    if (widgets)
    {
      ImGui::SetNextWindowPos({20, 20});
      ImGui::SetNextWindowSize({300, 200});
      ImGui::Begin("Independent context");
      ImGui::TextUnformatted("XII native rendering");
      ImGui::Button("Interactive control");
      ImGui::End();
    }
    XII_TEST_RESULT(scope.EndFrame(frame));
    return frame;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(DearImgui, OwnershipAndActivation)
{
  xiiImguiContextHandle a, b;
  XII_TEST_RESULT(xiiImguiSubsystem::CreateContext(a));
  XII_TEST_RESULT(xiiImguiSubsystem::CreateContext(b));
  XII_TEST_BOOL(a != b);
  auto* previous = ImGui::GetCurrentContext();
  {
    xiiImguiScopedContext outer(a);
    auto*                 first = ImGui::GetCurrentContext();
    XII_TEST_BOOL(first != nullptr);
    XII_TEST_BOOL(xiiImguiSubsystem::DestroyContext(a).Failed());
    {
      xiiImguiScopedContext inner(b);
      XII_TEST_BOOL(ImGui::GetCurrentContext() != first);
      ImGui::GetStyle().Alpha = 0.25f;
      xiiImguiScopedContext invalid({});
      XII_TEST_BOOL(!invalid.IsValid());
    }
    XII_TEST_BOOL(ImGui::GetCurrentContext() == first);
    XII_TEST_FLOAT(ImGui::GetStyle().Alpha, 1, 0);
  }
  XII_TEST_BOOL(ImGui::GetCurrentContext() == previous);
  XII_TEST_RESULT(xiiImguiSubsystem::DestroyContext(a));
  XII_TEST_BOOL(!xiiImguiSubsystem::IsContextValid(a));
  XII_TEST_BOOL(xiiImguiSubsystem::DestroyContext(a).Failed());
  xiiImguiContextHandle replacement;
  XII_TEST_RESULT(xiiImguiSubsystem::CreateContext(replacement));
  XII_TEST_BOOL(replacement != a);
  XII_TEST_RESULT(xiiImguiSubsystem::DestroyContext(replacement));
  XII_TEST_RESULT(xiiImguiSubsystem::DestroyContext(b));
}

XII_CREATE_SIMPLE_TEST(DearImgui, FrameStateAndImmutableLifetime)
{
  xiiImguiContextHandle context;
  XII_TEST_RESULT(xiiImguiSubsystem::CreateContext(context));
  xiiSharedPtr<const xiiImguiFrame> retained;
  {
    xiiImguiScopedContext scope(context);
    XII_TEST_BOOL(scope.EndFrame(retained).Failed());
    auto invalid        = Description();
    invalid.m_DeltaTime = xiiTime::MakeZero();
    XII_TEST_BOOL(scope.BeginFrame(invalid).Failed());
    XII_TEST_RESULT(scope.BeginFrame(Description()));
    XII_TEST_BOOL(scope.BeginFrame(Description()).Failed());
    XII_TEST_BOOL(xiiImguiInput::MouseButton(context, 0, true).Failed());
    XII_TEST_RESULT(scope.CancelFrame());
    XII_TEST_BOOL(scope.CancelFrame().Failed());
  }
  // First frame establishes auto-fit windows; the next produces visible content.
  Draw(context);
  retained = Draw(context);
  XII_TEST_BOOL(retained && !retained->GetVertices().IsEmpty() && !retained->GetCommands().IsEmpty());
  const auto count    = retained->GetVertices().GetCount();
  const auto position = retained->GetVertices()[0].pos;
  for (xiiUInt32 i = 0; i < 5; ++i)
    Draw(context);
  const auto empty = Draw(context, false);
  XII_TEST_BOOL(empty && empty->GetCommands().IsEmpty());
  {
    xiiImguiScopedContext scope(context);
    auto                  minimized = Description();
    minimized.m_vDisplaySize.SetZero();
    XII_TEST_RESULT(scope.BeginFrame(minimized));
    xiiSharedPtr<const xiiImguiFrame> frame;
    XII_TEST_RESULT(scope.EndFrame(frame));
    XII_TEST_BOOL(frame->GetCommands().IsEmpty());
  }
  XII_TEST_RESULT(xiiImguiSubsystem::DestroyContext(context));
  XII_TEST_INT(retained->GetVertices().GetCount(), count);
  XII_TEST_FLOAT(retained->GetVertices()[0].pos.x, position.x, 0);
  XII_TEST_BOOL(!retained->GetTextures().IsEmpty());
}

XII_CREATE_SIMPLE_TEST(DearImgui, InputIsolationAndDisable)
{
  xiiImguiContextHandle a, b;
  XII_TEST_RESULT(xiiImguiSubsystem::CreateContext(a));
  XII_TEST_RESULT(xiiImguiSubsystem::CreateContext(b));
  XII_TEST_RESULT(xiiImguiInput::Focus(a, true));
  XII_TEST_RESULT(xiiImguiInput::MousePosition(a, {40, 50}));
  XII_TEST_RESULT(xiiImguiInput::MouseButton(a, 0, true));
  XII_TEST_RESULT(xiiImguiInput::Key(a, ImGuiKey_A, true));
  XII_TEST_RESULT(xiiImguiInput::Text(a, 0x03A9));
  XII_TEST_BOOL(xiiImguiInput::Text(a, 0xD800).Failed());
  XII_TEST_BOOL(xiiImguiInput::MouseButton(a, 5, true).Failed());
  XII_TEST_BOOL(xiiImguiInput::Key({}, ImGuiKey_A, true).Failed());
  {
    xiiImguiScopedContext scope(a);
    bool                  keySeen = false, textSeen = false, mouseSeen = false;
    // The default ImGui event queue deliberately trickles different input categories across frames.
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      XII_TEST_RESULT(scope.BeginFrame(Description()));
      keySeen |= ImGui::IsKeyDown(ImGuiKey_A);
      mouseSeen |= ImGui::GetIO().MouseDown[0];
      textSeen |= ImGui::GetIO().InputQueueCharacters.Size == 1 && ImGui::GetIO().InputQueueCharacters[0] == 0x03A9;
      XII_TEST_RESULT(scope.CancelFrame());
    }
    XII_TEST_BOOL(keySeen && textSeen && mouseSeen);
    scope.SetInputEnabled(false);
    XII_TEST_RESULT(xiiImguiInput::Key(a, ImGuiKey_B, true));
    XII_TEST_RESULT(scope.BeginFrame(Description()));
    XII_TEST_BOOL(!ImGui::IsKeyDown(ImGuiKey_A) && !ImGui::IsKeyDown(ImGuiKey_B));
    XII_TEST_BOOL(!ImGui::GetIO().MouseDown[0]);
    XII_TEST_BOOL(!scope.GetInputCapture().m_bKeyboard);
    XII_TEST_RESULT(scope.CancelFrame());
  }
  {
    xiiImguiScopedContext scope(b);
    XII_TEST_RESULT(scope.BeginFrame(Description()));
    XII_TEST_BOOL(!ImGui::IsKeyDown(ImGuiKey_A) && !ImGui::GetIO().MouseDown[0]);
    XII_TEST_INT(ImGui::GetIO().InputQueueCharacters.Size, 0);
    XII_TEST_RESULT(scope.CancelFrame());
  }
  XII_TEST_RESULT(xiiImguiSubsystem::DestroyContext(a));
  XII_TEST_RESULT(xiiImguiSubsystem::DestroyContext(b));
}

XII_CREATE_SIMPLE_TEST(DearImgui, SourceLocalQuickClickAndTextBurst)
{
  xiiImguiContextHandle context;
  XII_TEST_RESULT(xiiImguiSubsystem::CreateContext(context));
  InputSource source;
  auto        slots   = source.GetInputSlotEvents().AddEventHandler([&](const xiiInputDevice::InputSlotEvent& event) {
    XII_TEST_RESULT(xiiImguiInput::InputSlot(context, source, event.m_sSlot, event.m_fValue));
  });
  auto        text    = source.GetTextInputEvents().AddEventHandler([&](xiiUInt32 value) {
    XII_TEST_RESULT(xiiImguiInput::Text(context, value));
  });
  bool        clicked = false;
  auto        draw    = [&]() {
    xiiImguiScopedContext scope(context);
    XII_TEST_RESULT(scope.BeginFrame(Description()));
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({300, 200});
    ImGui::Begin("Click test");
    ImGui::SetCursorScreenPos({50, 60});
    clicked |= ImGui::Button("Quick click", {100, 30});
    ImGui::End();
    xiiSharedPtr<const xiiImguiFrame> frame;
    XII_TEST_RESULT(scope.EndFrame(frame));
  };
  draw();
  draw();
  source.Slot(xiiInputSlot_MousePositionX, 75.0f / 640);
  source.Slot(xiiInputSlot_MousePositionY, 75.0f / 480);
  // Both transitions occur before the next frame. Polling only the final state would lose this click.
  source.Slot(xiiInputSlot_MouseButton0, 1);
  source.Slot(xiiInputSlot_MouseButton0, 0);
  for (xiiUInt32 i = 0; i < 4; ++i)
    draw();
  XII_TEST_BOOL(clicked);
  source.Character(0x03A9);
  source.Character(0x1F642);
  {
    xiiImguiScopedContext scope(context);
    XII_TEST_RESULT(scope.BeginFrame(Description()));
    XII_TEST_INT(ImGui::GetIO().InputQueueCharacters.Size, 2);
    if (ImGui::GetIO().InputQueueCharacters.Size == 2)
    {
      XII_TEST_INT(ImGui::GetIO().InputQueueCharacters[0], 0x03A9);
      XII_TEST_INT(ImGui::GetIO().InputQueueCharacters[1], 0x1F642);
    }
    XII_TEST_RESULT(scope.CancelFrame());
  }
  source.GetInputSlotEvents().RemoveEventHandler(slots);
  source.GetTextInputEvents().RemoveEventHandler(text);
  XII_TEST_RESULT(xiiImguiSubsystem::DestroyContext(context));
}

XII_CREATE_SIMPLE_TEST(DearImgui, FontVersionsAndRejectedDrawData)
{
  xiiImguiContextHandle context;
  XII_TEST_RESULT(xiiImguiSubsystem::CreateContext(context));
  auto                              first = Draw(context);
  xiiSharedPtr<const xiiImguiFrame> larger;
  {
    xiiImguiScopedContext scope(context);
    XII_TEST_RESULT(scope.BeginFrame(Description()));
    ImGui::PushFont(nullptr, 40);
    ImGui::GetForegroundDrawList()->AddText({10, 10}, IM_COL32_WHITE, "Larger dynamic font");
    ImGui::PopFont();
    XII_TEST_RESULT(scope.EndFrame(larger));
    XII_TEST_BOOL(!first->GetTextures().IsEmpty() && !larger->GetTextures().IsEmpty());
    XII_TEST_BOOL(first->GetTextures()[0] != larger->GetTextures()[0]);
    {
      xiiTestLogInterface log;
      xiiLogSystemScope   logScope(&log);
      log.ExpectMessage("ImGui draw references an unregistered or stale texture token.", xiiLogMsgType::ErrorMsg);
      XII_TEST_RESULT(scope.BeginFrame(Description()));
      ImGui::GetForegroundDrawList()->AddImage(ImTextureRef(0xDEADBEEF), {20, 20}, {40, 40});
      xiiSharedPtr<const xiiImguiFrame> invalid;
      XII_TEST_BOOL(scope.EndFrame(invalid).Failed() && !invalid);
    }
    {
      xiiTestLogInterface log;
      xiiLogSystemScope   logScope(&log);
      log.ExpectMessage("ImGui deferred rendering does not support user draw callbacks.", xiiLogMsgType::ErrorMsg);
      XII_TEST_RESULT(scope.BeginFrame(Description()));
      ImGui::GetForegroundDrawList()->AddCallback([](const ImDrawList*, const ImDrawCmd*) {}, nullptr);
      xiiSharedPtr<const xiiImguiFrame> invalid;
      XII_TEST_BOOL(scope.EndFrame(invalid).Failed() && !invalid);
    }
  }
  XII_TEST_BOOL(Draw(context) != nullptr); // Failed finalization still leaves a reusable context.
  XII_TEST_RESULT(xiiImguiSubsystem::DestroyContext(context));
}

XII_CREATE_SIMPLE_TEST(DearImgui, ReflectedPinsAndSharedViewBindings)
{
  xiiImguiContextHandle context;
  XII_TEST_RESULT(xiiImguiSubsystem::CreateContext(context));
  const auto                          frame = Draw(context, false);
  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.AddPass("UI", xiiGetStaticRTTI<xiiImguiRenderPass>());
  descriptor.m_Interface.PushBack({"Color", true, descriptor.m_Passes[0].m_Id, "Color"});
  descriptor.m_Interface.PushBack({"UI", true, descriptor.m_Passes[0].m_Id, "UI"});
  descriptor.m_Interface.PushBack({"Output", false, descriptor.m_Passes[0].m_Id, "Color"});
  xiiSharedPtr<const xiiRenderPipeline> pipeline;
  xiiStringBuilder                      error;
  if (!XII_TEST_RESULT_MSG(xiiRenderPipeline::Compile(descriptor, pipeline, &error), "%s", error.GetData()))
    return;
  // Two independent pipeline instances bind the same owned UI frame, with distinct setup-local wrappers.
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiRenderPipelineInstance instance;
    xiiRenderGraph            graph;
    xiiRenderGraphBlackboard  blackboard;
    XII_TEST_RESULT(instance.Initialize(pipeline));
    graph.BeginSetup(i + 1);
    xiiRenderGraphTextureHandle target;
    graph.AddPass<xiiRenderGraphTextureHandle>("Target", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription desc;
        desc.m_Type = xiiGALResourceDimension::Texture2D;
        desc.m_Size = {640, 480};
        desc.m_Format = xiiGALResourceFormat::RGBA16Float;
        desc.m_BindFlags = xiiGALBindFlags::RenderTarget;
        target = data = builder.WriteTexture("Target", desc, xiiGALResourceStateFlags::RenderTarget); }, [](const auto&, xiiRenderGraphPassContext&) {});
    xiiRenderPipelineBinding data;
    data.m_pFrameData                      = XII_DEFAULT_NEW(xiiImguiFrameData, graph.GetSetupSerial(), frame);
    xiiRenderPipelineInputBinding inputs[] = {
      {graph, pipeline->FindInterface("Color", true), target},
      {graph, pipeline->FindInterface("UI", true), data}};
    XII_TEST_RESULT(instance.Build(nullptr, graph, blackboard, {}, inputs));
    XII_TEST_BOOL(instance.GetInterfaceBinding(pipeline->FindInterface("Output", false))->m_hTexture == target);
    graph.EndSetup();
  }
  XII_TEST_RESULT(xiiImguiSubsystem::DestroyContext(context));
}
