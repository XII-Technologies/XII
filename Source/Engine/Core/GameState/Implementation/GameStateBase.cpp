#include <Core/CorePCH.h>

#include <Core/GameState/GameStateBase.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameStateBase, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGameStateBase::xiiGameStateBase()  = default;
xiiGameStateBase::~xiiGameStateBase() = default;

void xiiGameStateBase::ProcessInput() {}

void xiiGameStateBase::BeforeWorldUpdate() {}

void xiiGameStateBase::AfterWorldUpdate() {}

void xiiGameStateBase::RequestQuit()
{
  m_bStateWantsToQuit = true;
}

bool xiiGameStateBase::WasQuitRequested() const
{
  return m_bStateWantsToQuit;
}



XII_STATICLINK_FILE(Core, Core_GameState_Implementation_GameStateBase);
