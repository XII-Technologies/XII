#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QWidget>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

class QGridLayout;
class xiiDocument;
class xiiQtManipulatorLabel;
struct xiiManipulatorManagerEvent;
class xiiObjectAccessorBase;

class XII_GUIFOUNDATION_DLL xiiQtTypeWidget : public QWidget
{
  Q_OBJECT
public:
  xiiQtTypeWidget(QWidget* pParent, xiiQtPropertyGridWidget* pGrid, xiiObjectAccessorBase* pObjectAccessor, const xiiRTTI* pType, const char* szIncludeProperties, const char* szExcludeProperties);
  ~xiiQtTypeWidget();
  void                                           SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items);
  const xiiHybridArray<xiiPropertySelection, 8>& GetSelection() const { return m_Items; }
  const xiiRTTI*                                 GetType() const { return m_pType; }
  void                                           PrepareToDie();

private:
  struct PropertyGroup
  {
    PropertyGroup(const xiiGroupAttribute* pAttr, float& ref_fOrder)
    {
      if (pAttr)
      {
        m_sGroup    = pAttr->GetGroup();
        m_sIconName = pAttr->GetIconName();
        m_fOrder    = pAttr->GetOrder();
        if (m_fOrder == -1.0f)
        {
          ref_fOrder += 1.0f;
          m_fOrder = ref_fOrder;
        }
      }
      else
      {
        ref_fOrder += 1.0f;
        m_fOrder = ref_fOrder;
      }
    }

    void MergeGroup(const xiiGroupAttribute* pAttr)
    {
      if (pAttr)
      {
        m_sGroup    = pAttr->GetGroup();
        m_sIconName = pAttr->GetIconName();
        if (pAttr->GetOrder() != -1.0f)
        {
          m_fOrder = pAttr->GetOrder();
        }
      }
    }

    bool operator==(const PropertyGroup& rhs) { return m_sGroup == rhs.m_sGroup; }
    bool operator<(const PropertyGroup& rhs) { return m_fOrder < rhs.m_fOrder; }

    xiiString                                     m_sGroup;
    xiiString                                     m_sIconName;
    float                                         m_fOrder = -1.0f;
    xiiHybridArray<const xiiAbstractProperty*, 8> m_Properties;
  };

  void BuildUI(const xiiRTTI* pType, const char* szIncludeProperties, const char* szExcludeProperties);
  void BuildUI(const xiiRTTI* pType, const xiiMap<xiiString, const xiiManipulatorAttribute*>& manipulatorMap, const xiiDynamicArray<xiiUniquePtr<PropertyGroup>>& groups, const char* szIncludeProperties, const char* szExcludeProperties);

  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const xiiCommandHistoryEvent& e);
  void ManipulatorManagerEventHandler(const xiiManipulatorManagerEvent& e);

  void UpdateProperty(const xiiDocumentObject* pObject, const xiiString& sProperty);
  void FlushQueuedChanges();
  void UpdatePropertyMetaState();

protected:
  virtual void showEvent(QShowEvent* event) override;

private:
  bool                                    m_bUndead         = false;
  xiiQtPropertyGridWidget*                m_pGrid           = nullptr;
  xiiObjectAccessorBase*                  m_pObjectAccessor = nullptr;
  const xiiRTTI*                          m_pType           = nullptr;
  xiiHybridArray<xiiPropertySelection, 8> m_Items;

  struct PropertyWidgetData
  {
    xiiQtPropertyWidget*   m_pWidget;
    xiiQtManipulatorLabel* m_pLabel;
    xiiString              m_sOriginalLabelText;
  };

  QGridLayout*                          m_pLayout;
  xiiMap<xiiString, PropertyWidgetData> m_PropertyWidgets;
  xiiHybridArray<xiiString, 1>          m_QueuedChanges;
  QPalette                              m_Pal;
};
