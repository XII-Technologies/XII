#pragma once

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QIcon>

///
class XII_GUIFOUNDATION_DLL xiiNamedAction : public xiiAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNamedAction, xiiAction);

public:
  xiiNamedAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
    xiiAction(context), m_sName(szName), m_sIconPath(szIconPath)
  {
  }

  const char* GetName() const { return m_sName; }

  const char* GetAdditionalDisplayString() { return m_sAdditionalDisplayString; }
  void        SetAdditionalDisplayString(const char* szString, bool bTriggerUpdate = true)
  {
    m_sAdditionalDisplayString = szString;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  const char* GetIconPath() const { return m_sIconPath; }
  void        SetIconPath(const char* szIconPath) { m_sIconPath = szIconPath; }

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

  virtual void Execute(const xiiVariant& value) override{};
};

///
class XII_GUIFOUNDATION_DLL xiiMenuAction : public xiiNamedAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMenuAction, xiiNamedAction);

public:
  xiiMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
    xiiNamedAction(context, szName, szIconPath)
  {
  }

  virtual void Execute(const xiiVariant& value) override{};
};

///
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
      typedef xiiUInt8 StorageType;

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

  xiiDynamicMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
    xiiMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(xiiHybridArray<Item, 16>& out_Entries) = 0;
};

///
class XII_GUIFOUNDATION_DLL xiiDynamicActionAndMenuAction : public xiiDynamicMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicActionAndMenuAction, xiiDynamicMenuAction);

public:
  xiiDynamicActionAndMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath);

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

///
class XII_GUIFOUNDATION_DLL xiiEnumerationMenuAction : public xiiDynamicMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEnumerationMenuAction, xiiDynamicMenuAction);

public:
  xiiEnumerationMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath);
  void             InitEnumerationType(const xiiRTTI* pEnumerationType);
  virtual void     GetEntries(xiiHybridArray<xiiDynamicMenuAction::Item, 16>& out_Entries) override;
  virtual xiiInt64 GetValue() const = 0;

protected:
  const xiiRTTI* m_pEnumerationType;
};

///
class XII_GUIFOUNDATION_DLL xiiButtonAction : public xiiNamedAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiButtonAction, xiiNamedAction);

public:
  xiiButtonAction(const xiiActionContext& context, const char* szName, bool bCheckable, const char* szIconPath);

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


class XII_GUIFOUNDATION_DLL xiiSliderAction : public xiiNamedAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSliderAction, xiiNamedAction);

public:
  xiiSliderAction(const xiiActionContext& context, const char* szName);

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
  void     SetValue(xiiInt32 val, bool bTriggerUpdate = true);

protected:
  bool     m_bEnabled;
  bool     m_bVisible;
  xiiInt32 m_iMinValue;
  xiiInt32 m_iMaxValue;
  xiiInt32 m_iCurValue;
};
