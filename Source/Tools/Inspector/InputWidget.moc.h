/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Input/InputManager.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_InputWidget.h>
#include <ads/DockWidget.h>

class xiiQtInputWidget : public ads::CDockWidget, public Ui_InputWidget
{
public:
  Q_OBJECT

public:
  xiiQtInputWidget(ads::CDockManager* pDockManager, QWidget* pParent = nullptr);

  static xiiQtInputWidget* s_pWidget;

private Q_SLOTS:
  virtual void on_ButtonClearSlots_clicked();
  virtual void on_ButtonClearActions_clicked();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  void ClearSlots();
  void ClearActions();

  void UpdateSlotTable(bool bRecreate);
  void UpdateActionTable(bool bRecreate);

  struct SlotData
  {
    xiiInt32          m_iTableRow;
    xiiUInt16         m_uiSlotFlags;
    xiiKeyState::Enum m_KeyState;
    float             m_fValue;
    float             m_fDeadZone;

    SlotData()
    {
      m_iTableRow   = -1;
      m_uiSlotFlags = 0;
      m_KeyState    = xiiKeyState::Up;
      m_fValue      = 0;
      m_fDeadZone   = 0;
    }
  };

  xiiMap<xiiString, SlotData> m_InputSlots;

  struct ActionData
  {
    xiiInt32          m_iTableRow;
    xiiKeyState::Enum m_KeyState;
    float             m_fValue;
    bool              m_bUseTimeScaling;

    xiiString m_sTrigger[xiiInputActionConfig::MaxInputSlotAlternatives];
    float     m_fTriggerScaling[xiiInputActionConfig::MaxInputSlotAlternatives];

    ActionData()
    {
      m_iTableRow       = -1;
      m_KeyState        = xiiKeyState::Up;
      m_fValue          = 0;
      m_bUseTimeScaling = false;
    }
  };

  xiiMap<xiiString, ActionData> m_InputActions;
};
