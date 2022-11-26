#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <RmlUiPlugin/RmlUiDataBinding.h>

class xiiBlackboard;

namespace xiiRmlUiInternal
{
  class BlackboardDataBinding final : public xiiRmlUiDataBinding
  {
  public:
    BlackboardDataBinding(const xiiSharedPtr<xiiBlackboard>& pBlackboard);
    ~BlackboardDataBinding();

    virtual xiiResult Initialize(Rml::Context& context) override;
    virtual void      Deinitialize(Rml::Context& context) override;
    virtual void      Update() override;

  private:
    xiiSharedPtr<xiiBlackboard> m_pBlackboard;

    Rml::DataModelHandle m_hDataModel;

    struct EntryWrapper
    {
      EntryWrapper(xiiBlackboard& blackboard, const xiiHashedString& sName, xiiUInt32 uiChangeCounter) :
        m_Blackboard(blackboard), m_sName(sName), m_uiChangeCounter(uiChangeCounter)
      {
      }

      void SetValue(const Rml::Variant& value);
      void GetValue(Rml::Variant& out_Value) const;

      xiiBlackboard&  m_Blackboard;
      xiiHashedString m_sName;
      xiiUInt32       m_uiChangeCounter;
    };

    Rml::Vector<EntryWrapper> m_EntryWrappers;

    xiiUInt32 m_uiBlackboardChangeCounter      = 0;
    xiiUInt32 m_uiBlackboardEntryChangeCounter = 0;
  };
} // namespace xiiRmlUiInternal
