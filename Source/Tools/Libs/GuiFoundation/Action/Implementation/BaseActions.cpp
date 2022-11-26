#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/BaseActions.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNamedAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCategoryAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMenuAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicMenuAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicActionAndMenuAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEnumerationMenuAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiButtonAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSliderAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDynamicActionAndMenuAction::xiiDynamicActionAndMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
  xiiDynamicMenuAction(context, szName, szIconPath)
{
  m_bEnabled = true;
  m_bVisible = true;
}

xiiEnumerationMenuAction::xiiEnumerationMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
  xiiDynamicMenuAction(context, szName, szIconPath)
{
  m_pEnumerationType = nullptr;
}

void xiiEnumerationMenuAction::InitEnumerationType(const xiiRTTI* pEnumerationType)
{
  m_pEnumerationType = pEnumerationType;
}

void xiiEnumerationMenuAction::GetEntries(xiiHybridArray<xiiDynamicMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();
  out_Entries.Reserve(m_pEnumerationType->GetProperties().GetCount() - 1);
  xiiInt64 iCurrentValue = xiiReflectionUtils::MakeEnumerationValid(m_pEnumerationType, GetValue());

  // sort entries by group / category
  // categories appear in the order in which they are used on the reflected properties
  // within each category, items are sorted by 'order'
  // all items that have the same 'order' are sorted alphabetically by display string

  xiiStringBuilder sCurGroup;
  float            fPrevOrder = -1;
  struct ItemWithOrder
  {
    float                      m_fOrder = -1;
    xiiDynamicMenuAction::Item m_Item;

    bool operator<(const ItemWithOrder& rhs) const
    {
      if (m_fOrder == rhs.m_fOrder)
      {
        return m_Item.m_sDisplay < rhs.m_Item.m_sDisplay;
      }

      return m_fOrder < rhs.m_fOrder;
    }
  };

  xiiHybridArray<ItemWithOrder, 16> unsortedItems;

  auto appendToOutput = [&]() {
    if (unsortedItems.IsEmpty())
      return;

    unsortedItems.Sort();

    if (!out_Entries.IsEmpty())
    {
      // add a separator between groups
      out_Entries.ExpandAndGetRef().m_ItemFlags.Add(xiiDynamicMenuAction::Item::ItemFlags::Separator);
    }

    for (const auto& sortedItem : unsortedItems)
    {
      out_Entries.PushBack(sortedItem.m_Item);
    }

    unsortedItems.Clear();
  };

  for (auto pProp : m_pEnumerationType->GetProperties().GetSubArray(1))
  {
    if (pProp->GetCategory() == xiiPropertyCategory::Constant)
    {
      if (const xiiGroupAttribute* pGroup = pProp->GetAttributeByType<xiiGroupAttribute>())
      {
        if (sCurGroup != pGroup->GetGroup())
        {
          sCurGroup = pGroup->GetGroup();

          appendToOutput();
        }

        fPrevOrder = pGroup->GetOrder();
      }

      ItemWithOrder& newItem = unsortedItems.ExpandAndGetRef();
      newItem.m_fOrder       = fPrevOrder;
      auto& item             = newItem.m_Item;

      {
        xiiInt64 iValue = static_cast<const xiiAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<xiiInt64>();

        item.m_sDisplay = xiiTranslate(pProp->GetPropertyName());

        item.m_UserValue = iValue;
        if (m_pEnumerationType->IsDerivedFrom<xiiEnumBase>())
        {
          item.m_CheckState =
            (iCurrentValue == iValue) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
        }
        else if (m_pEnumerationType->IsDerivedFrom<xiiBitflagsBase>())
        {
          item.m_CheckState =
            ((iCurrentValue & iValue) != 0) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
        }
      }
    }
  }

  appendToOutput();
}

xiiButtonAction::xiiButtonAction(const xiiActionContext& context, const char* szName, bool bCheckable, const char* szIconPath) :
  xiiNamedAction(context, szName, szIconPath)
{
  m_bCheckable = false;
  m_bChecked   = false;
  m_bEnabled   = true;
  m_bVisible   = true;
}


xiiSliderAction::xiiSliderAction(const xiiActionContext& context, const char* szName) :
  xiiNamedAction(context, szName, nullptr)
{
  m_bEnabled  = true;
  m_bVisible  = true;
  m_iMinValue = 0;
  m_iMaxValue = 100;
  m_iCurValue = 50;
}

void xiiSliderAction::SetRange(xiiInt32 iMin, xiiInt32 iMax, bool bTriggerUpdate /*= true*/)
{
  XII_ASSERT_DEBUG(iMin < iMax, "Invalid range");

  m_iMinValue = iMin;
  m_iMaxValue = iMax;

  if (bTriggerUpdate)
    TriggerUpdate();
}

void xiiSliderAction::SetValue(xiiInt32 val, bool bTriggerUpdate /*= true*/)
{
  m_iCurValue = xiiMath::Clamp(val, m_iMinValue, m_iMaxValue);
  if (bTriggerUpdate)
    TriggerUpdate();
}
