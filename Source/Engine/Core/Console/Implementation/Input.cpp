#include <Core/CorePCH.h>

#include <Core/Console/QuakeConsole.h>
#include <Core/Input/InputManager.h>

bool xiiQuakeConsole::ProcessInputCharacter(xiiUInt32 uiChar)
{
  switch (uiChar)
  {
    case 27: // Escape
    {
      ClearInputLine();
      return false;
    }

    case '\b': // backspace
    {
      if (!m_sInputLine.IsEmpty() && m_iCaretPosition > 0)
      {
        RemoveCharacter(m_iCaretPosition - 1);
        MoveCaret(-1);
      }
      return false;
    }

    case '\t':
    {
      if (AutoComplete(m_sInputLine))
      {
        MoveCaret(500);
      }
      return false;
    }

    case 13: // Enter
    {
      AddToInputHistory(m_sInputLine);
      ExecuteCommand(m_sInputLine);
      ClearInputLine();
      return false;
    }
  }

  return true;
}

bool xiiQuakeConsole::FilterInputCharacter(xiiUInt32 uiChar)
{
  // filter out not only all non-ASCII characters, but also all the non-printable ASCII characters
  // if you want to support full Unicode characters in the console, override this function and change this restriction
  if (uiChar < 32 || uiChar > 126)
    return false;

  return true;
}

void xiiQuakeConsole::ClampCaretPosition()
{
  m_iCaretPosition = xiiMath::Clamp<xiiInt32>(m_iCaretPosition, 0, m_sInputLine.GetCharacterCount());
}

void xiiQuakeConsole::MoveCaret(xiiInt32 iMoveOffset)
{
  m_iCaretPosition += iMoveOffset;

  ClampCaretPosition();
}

void xiiQuakeConsole::Scroll(xiiInt32 iLines)
{
  if (m_bUseFilteredStrings)
    m_iScrollPosition = xiiMath::Clamp<xiiInt32>(m_iScrollPosition + iLines, 0, xiiMath::Max<xiiInt32>(m_FilteredConsoleStrings.GetCount() - 10, 0));
  else
    m_iScrollPosition = xiiMath::Clamp<xiiInt32>(m_iScrollPosition + iLines, 0, xiiMath::Max<xiiInt32>(m_ConsoleStrings.GetCount() - 10, 0));
}

void xiiQuakeConsole::ClearInputLine()
{
  m_sInputLine.Clear();
  m_iCaretPosition              = 0;
  m_iScrollPosition             = 0;
  m_iCurrentInputHistoryElement = -1;

  m_FilteredConsoleStrings.Clear();
  m_bUseFilteredStrings = false;

  InputStringChanged();
}

void xiiQuakeConsole::ClearConsoleStrings()
{
  m_ConsoleStrings.Clear();
  m_FilteredConsoleStrings.Clear();
  m_bUseFilteredStrings = false;
  m_iScrollPosition     = 0;
}

void xiiQuakeConsole::DeleteNextCharacter()
{
  RemoveCharacter(m_iCaretPosition);
}

void xiiQuakeConsole::RemoveCharacter(xiiUInt32 uiInputLinePosition)
{
  if (uiInputLinePosition >= m_sInputLine.GetCharacterCount())
    return;

  auto it = m_sInputLine.GetIteratorFront();
  it += uiInputLinePosition;

  auto itNext = it;
  ++itNext;

  m_sInputLine.Remove(it.GetData(), itNext.GetData());

  InputStringChanged();
}

void xiiQuakeConsole::AddInputCharacter(xiiUInt32 uiChar)
{
  if (uiChar == '\0')
    return;

  if (!ProcessInputCharacter(uiChar))
    return;

  if (!FilterInputCharacter(uiChar))
    return;

  ClampCaretPosition();

  auto it = m_sInputLine.GetIteratorFront();
  it += m_iCaretPosition;

  xiiUInt32 uiString[2] = {uiChar, 0};

  m_sInputLine.Insert(it.GetData(), xiiStringUtf8(uiString).GetData());

  MoveCaret(1);

  InputStringChanged();
}

void xiiQuakeConsole::DoDefaultInputHandling(bool bConsoleOpen)
{
  if (!m_bDefaultInputHandlingInitialized)
  {
    m_bDefaultInputHandlingInitialized = true;

    xiiInputActionConfig cfg;
    cfg.m_bApplyTimeScaling = true;

    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyLeft;
    xiiInputManager::SetInputActionConfig("Console", "MoveCaretLeft", cfg, true);

    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyRight;
    xiiInputManager::SetInputActionConfig("Console", "MoveCaretRight", cfg, true);

    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyHome;
    xiiInputManager::SetInputActionConfig("Console", "MoveCaretStart", cfg, true);

    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyEnd;
    xiiInputManager::SetInputActionConfig("Console", "MoveCaretEnd", cfg, true);

    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyDelete;
    xiiInputManager::SetInputActionConfig("Console", "DeleteCharacter", cfg, true);

    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyPageUp;
    xiiInputManager::SetInputActionConfig("Console", "ScrollUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyPageDown;
    xiiInputManager::SetInputActionConfig("Console", "ScrollDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyUp;
    xiiInputManager::SetInputActionConfig("Console", "HistoryUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyDown;
    xiiInputManager::SetInputActionConfig("Console", "HistoryDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyF2;
    xiiInputManager::SetInputActionConfig("Console", "RepeatLast", cfg, true);

    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyF3;
    xiiInputManager::SetInputActionConfig("Console", "RepeatSecondLast", cfg, true);

    return;
  }

  if (bConsoleOpen)
  {
    if (xiiInputManager::GetInputActionState("Console", "MoveCaretLeft") == xiiKeyState::Pressed)
      MoveCaret(-1);
    if (xiiInputManager::GetInputActionState("Console", "MoveCaretRight") == xiiKeyState::Pressed)
      MoveCaret(1);
    if (xiiInputManager::GetInputActionState("Console", "MoveCaretStart") == xiiKeyState::Pressed)
      MoveCaret(-1000);
    if (xiiInputManager::GetInputActionState("Console", "MoveCaretEnd") == xiiKeyState::Pressed)
      MoveCaret(1000);
    if (xiiInputManager::GetInputActionState("Console", "DeleteCharacter") == xiiKeyState::Pressed)
      DeleteNextCharacter();
    if (xiiInputManager::GetInputActionState("Console", "ScrollUp") == xiiKeyState::Pressed)
      Scroll(10);
    if (xiiInputManager::GetInputActionState("Console", "ScrollDown") == xiiKeyState::Pressed)
      Scroll(-10);
    if (xiiInputManager::GetInputActionState("Console", "HistoryUp") == xiiKeyState::Pressed)
    {
      RetrieveInputHistory(1, m_sInputLine);
      m_iCaretPosition = m_sInputLine.GetCharacterCount();
    }
    if (xiiInputManager::GetInputActionState("Console", "HistoryDown") == xiiKeyState::Pressed)
    {
      RetrieveInputHistory(-1, m_sInputLine);
      m_iCaretPosition = m_sInputLine.GetCharacterCount();
    }

    const xiiUInt32 uiChar = xiiInputManager::RetrieveLastCharacter();

    if (uiChar != '\0')
      AddInputCharacter(uiChar);
  }
  else
  {
    const xiiUInt32 uiChar = xiiInputManager::RetrieveLastCharacter(false);

    char  szCmd[16]  = "";
    char* szIterator = szCmd;
    xiiUnicodeUtils::EncodeUtf32ToUtf8(uiChar, szIterator);
    *szIterator = '\0';
    ExecuteBoundKey(szCmd);
  }

  if (xiiInputManager::GetInputActionState("Console", "RepeatLast") == xiiKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 1)
      ExecuteCommand(GetInputHistory()[0]);
  }

  if (xiiInputManager::GetInputActionState("Console", "RepeatSecondLast") == xiiKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 2)
      ExecuteCommand(GetInputHistory()[1]);
  }
}

XII_STATICLINK_FILE(Core, Core_Console_Implementation_Input);
