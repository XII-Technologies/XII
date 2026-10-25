/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/System/Window.h>
#include <TestFramework/Framework/TestFramework.h>

#if XII_ENABLED(XII_SUPPORTS_SDL)
#  include <SDL3/SDL_events.h>
#  include <SDL3/SDL_keycode.h>
#  include <SDL3/SDL_video.h>

XII_CREATE_SIMPLE_TEST(DearImguiGraphics, IndependentWindowInput)
{
  xiiWindow                    first, second;
  xiiWindowCreationDescription description;
  description.m_Title                = "XII input source one";
  description.m_uiWindowNumber       = xiiWindow::GetNextUnusedWindowNumber();
  description.m_Resolution           = {320, 240};
  description.m_bClipMouseCursor     = false;
  description.m_bSetForegroundOnInit = false;
  if (!XII_TEST_RESULT(first.Initialize(description)))
    return;
  description.m_Title          = "XII input source two";
  description.m_uiWindowNumber = xiiWindow::GetNextUnusedWindowNumber();
  if (!XII_TEST_RESULT(second.Initialize(description)))
    return;
  first.ProcessWindowMessages();
  int          count;
  auto**       windows = SDL_GetWindows(&count);
  SDL_WindowID firstId = 0, secondId = 0;
  for (int i = 0; i < count; ++i)
  {
    const xiiStringView title(SDL_GetWindowTitle(windows[i]));
    if (title == "XII input source one")
      firstId = SDL_GetWindowID(windows[i]);
    if (title == "XII input source two")
      secondId = SDL_GetWindowID(windows[i]);
  }
  SDL_free(windows);
  XII_TEST_BOOL(firstId && secondId && firstId != secondId);
  xiiUInt32                  firstCharacters = 0;
  xiiDynamicArray<xiiUInt32> secondCharacters;
  xiiDynamicArray<float>     transitions;
  first.GetInputDevice()->GetTextInputEvents().AddEventHandler([&](xiiUInt32) { ++firstCharacters; });
  second.GetInputDevice()->GetTextInputEvents().AddEventHandler([&](xiiUInt32 character) { secondCharacters.PushBack(character); });
  second.GetInputDevice()->GetInputSlotEvents().AddEventHandler([&](const xiiInputDevice::InputSlotEvent& event) {
    if (event.m_sSlot == xiiInputSlot_KeyA)
      transitions.PushBack(event.m_fValue);
  });
  SDL_Event event       = {};
  event.type            = SDL_EVENT_WINDOW_FOCUS_LOST;
  event.window.windowID = firstId;
  XII_TEST_BOOL(SDL_PushEvent(&event));
  event.type            = SDL_EVENT_WINDOW_FOCUS_GAINED;
  event.window.windowID = secondId;
  XII_TEST_BOOL(SDL_PushEvent(&event));
  event.type         = SDL_EVENT_KEY_DOWN;
  event.key.windowID = secondId;
  event.key.key      = SDLK_A;
  XII_TEST_BOOL(SDL_PushEvent(&event));
  event.type = SDL_EVENT_KEY_UP;
  XII_TEST_BOOL(SDL_PushEvent(&event));
  event               = {};
  event.type          = SDL_EVENT_TEXT_INPUT;
  event.text.windowID = secondId;
  event.text.text     = "\xCE\xA9\xF0\x9F\x99\x82";
  XII_TEST_BOOL(SDL_PushEvent(&event));
  event                 = {};
  event.type            = SDL_EVENT_WINDOW_RESIZED;
  event.window.windowID = secondId;
  event.window.data1    = 400;
  event.window.data2    = 300;
  XII_TEST_BOOL(SDL_PushEvent(&event));
  first.ProcessWindowMessages();
  XII_TEST_INT(firstCharacters, 0);
  XII_TEST_INT(secondCharacters.GetCount(), 2);
  if (secondCharacters.GetCount() == 2)
  {
    XII_TEST_INT(secondCharacters[0], 0x03A9);
    XII_TEST_INT(secondCharacters[1], 0x1F642);
  }
  XII_TEST_INT(transitions.GetCount(), 2);
  if (transitions.GetCount() == 2)
  {
    XII_TEST_FLOAT(transitions[0], 1, 0);
    XII_TEST_FLOAT(transitions[1], 0, 0);
  }
  XII_TEST_BOOL(!first.GetInputDevice()->IsFocused() && second.GetInputDevice()->IsFocused());
  XII_TEST_INT(first.GetClientAreaSize().width, 320);
  XII_TEST_INT(second.GetClientAreaSize().width, 400);
  XII_TEST_RESULT(first.Destroy());
  event               = {};
  event.type          = SDL_EVENT_TEXT_INPUT;
  event.text.windowID = secondId;
  event.text.text     = "!";
  XII_TEST_BOOL(SDL_PushEvent(&event));
  second.ProcessWindowMessages();
  XII_TEST_INT(secondCharacters.GetCount(), 3);
  XII_TEST_RESULT(second.Destroy());
}
#endif
