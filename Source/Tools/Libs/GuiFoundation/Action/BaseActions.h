/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/Action/Action.h>
#include <QIcon>

///
class XII_GUIFOUNDATION_DLL xiiNamedAction : public xiiAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNamedAction, xiiAction);

public:
  xiiNamedAction(const xiiActionContext& context, xiiStringView sName, xiiStringView sIconPath) :
    xiiAction(context), m_sName(sName), m_sIconPath(sIconPath)
  {
  }

  xiiStringView GetName() const { return m_sName; }

  xiiStringView GetAdditionalDisplayString() { return m_sAdditionalDisplayString; }
  void          SetAdditionalDisplayString(xiiStringView sString, bool bTriggerUpdate = true)
  {
    m_sAdditionalDisplayString = sString;

    if (bTriggerUpdate)
      TriggerUpdate();
  }

  xiiStringView GetIconPath() const { return m_sIconPath; }
  void          SetIconPath(xiiStringView sIconPath) { m_sIconPath = sIconPath; }

protected:
  xiiString m_sName;
  xiiString m_sAdditionalDisplayString; // to add some context to the current action
  xiiString m_sIconPath;
};

///
class XII_GUIFOUNDATION_DLL xiiCategoryAction : public xiiAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCategoryAction, xiiAction);

public:
  xiiCategoryAction(const xiiActionContext& context) :
    xiiAction(context)
  {
  }

  virtual void Execute(const xiiVariant& value) override {};
};

/// An action that represents a sub-menu. Can be within a menu bar, or the menu of a tool button).
///
/// This class can be used directly, but then every menu entry has to be mapped individually into the menu.
/// It is often more convenient to use derived types which already set up the content of the menu.
class XII_GUIFOUNDATION_DLL xiiMenuAction : public xiiNamedAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMenuAction, xiiNamedAction);

public:
  xiiMenuAction(const xiiActionContext& context, xiiStringView sName, xiiStringView sIconPath) :
    xiiNamedAction(context, sName, sIconPath)
  {
  }

  virtual void Execute(const xiiVariant& value) override {};
};

/// A menu action whose content is determined when opening the menu.
///
/// Every time this menu gets opened, GetEntries() is executed,
/// with the state of the previous menu items.
/// It can then return the same result, or adjust the entries (update check marks or show entirely different entries).
///
/// Derive from this, to create your own dynamic menu.
/// Or use something like xiiEnumerationMenuAction to get a menu for an enum type.
class XII_GUIFOUNDATION_DLL xiiDynamicMenuAction : public xiiMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicMenuAction, xiiMenuAction);

public:
  struct Item
  {
    enum class CheckMark
    {
      NotCheckable,
      Unchecked,
      Checked
    };

    struct ItemFlags
    {
      using StorageType = xiiUInt8;

      enum Enum
      {
        Default   = 0,
        Separator = XII_BIT(0),
      };
      struct Bits
      {
        StorageType Separator : 1;
      };
    };

    Item() { m_CheckState = CheckMark::NotCheckable; }

    xiiString              m_sDisplay;
    QIcon                  m_Icon;
    CheckMark              m_CheckState;
    xiiBitflags<ItemFlags> m_ItemFlags;
    xiiVariant             m_UserValue;
  };

  xiiDynamicMenuAction(const xiiActionContext& context, xiiStringView sName, xiiStringView sIconPath) :
    xiiMenuAction(context, sName, sIconPath)
  {
  }
  virtual void GetEntries(xiiDynamicArray<Item>& out_entries) = 0;
};

/// An action that is displayed as a tool button that is clickable but also has a sub-menu that can be opened for selecting a different action.
class XII_GUIFOUNDATION_DLL xiiDynamicActionAndMenuAction : public xiiDynamicMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicActionAndMenuAction, xiiDynamicMenuAction);

public:
  xiiDynamicActionAndMenuAction(const xiiActionContext& context, xiiStringView sName, xiiStringView sIconPath);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true)
  {
    m_bEnabled = bEnable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true)
  {
    m_bVisible = bVisible;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

protected:
  bool m_bEnabled;
  bool m_bVisible;
};

/// A menu that lists all values of an enum type.
class XII_GUIFOUNDATION_DLL xiiEnumerationMenuAction : public xiiDynamicMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEnumerationMenuAction, xiiDynamicMenuAction);

public:
  xiiEnumerationMenuAction(const xiiActionContext& context, xiiStringView sName, xiiStringView sIconPath);
  void             InitEnumerationType(const xiiRTTI* pEnumerationType);
  virtual void     GetEntries(xiiDynamicArray<Item>& out_entries) override;
  virtual xiiInt64 GetValue() const = 0;

protected:
  const xiiRTTI* m_pEnumerationType;
};

/// The standard button action.
class XII_GUIFOUNDATION_DLL xiiButtonAction : public xiiNamedAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiButtonAction, xiiNamedAction);

public:
  xiiButtonAction(const xiiActionContext& context, xiiStringView sName, bool bCheckable, xiiStringView sIconPath);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true)
  {
    m_bEnabled = bEnable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsCheckable() const { return m_bCheckable; }
  void SetCheckable(bool bCheckable, bool bTriggerUpdate = true)
  {
    m_bCheckable = bCheckable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsChecked() const { return m_bChecked; }
  void SetChecked(bool bChecked, bool bTriggerUpdate = true)
  {
    m_bChecked = bChecked;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true)
  {
    m_bVisible = bVisible;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

protected:
  bool m_bCheckable;
  bool m_bChecked;
  bool m_bEnabled;
  bool m_bVisible;
};

/// An action that represents an integer value within a fixed range, and gets displayed as a slider.
class XII_GUIFOUNDATION_DLL xiiSliderAction : public xiiNamedAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSliderAction, xiiNamedAction);

public:
  xiiSliderAction(const xiiActionContext& context, xiiStringView sName);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true)
  {
    m_bEnabled = bEnable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true)
  {
    m_bVisible = bVisible;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  void GetRange(xiiInt32& out_iMin, xiiInt32& out_iMax) const
  {
    out_iMin = m_iMinValue;
    out_iMax = m_iMaxValue;
  }

  void SetRange(xiiInt32 iMin, xiiInt32 iMax, bool bTriggerUpdate = true);

  xiiInt32 GetValue() const { return m_iCurValue; }
  void     SetValue(xiiInt32 iVal, bool bTriggerUpdate = true);

protected:
  bool     m_bEnabled;
  bool     m_bVisible;
  xiiInt32 m_iMinValue;
  xiiInt32 m_iMaxValue;
  xiiInt32 m_iCurValue;
};
