/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngineTest/GameEngineTestPCH.h>

#include "StateMachineTest.h"
#include <GameEngine/Components/StateMachine/StateMachineBuiltins.h>

namespace
{
  class TestState : public xiiStateMachineState
  {
    XII_ADD_DYNAMIC_REFLECTION(TestState, xiiStateMachineState);

  public:
    TestState(xiiStringView sName = xiiStringView()) :
      xiiStateMachineState(sName)
    {
    }

    virtual void OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_Counter.m_uiEnterCounter++;

      m_CounterTable[&ref_instance] = pData->m_Counter;
    }

    virtual void OnExit(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pToState) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_Counter.m_uiExitCounter++;

      m_CounterTable[&ref_instance] = pData->m_Counter;
    }

    virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) override
    {
      out_desc.FillFromType<InstanceData>();
      return true;
    }

    struct Counter
    {
      xiiUInt32 m_uiEnterCounter = 0;
      xiiUInt32 m_uiExitCounter  = 0;
    };

    mutable xiiHashTable<xiiStateMachineInstance*, Counter> m_CounterTable;

    struct InstanceData
    {
      InstanceData() { s_uiConstructionCounter++; }
      ~InstanceData() { s_uiDestructionCounter++; }

      Counter m_Counter;

      static xiiUInt32 s_uiConstructionCounter;
      static xiiUInt32 s_uiDestructionCounter;
    };
  };

  xiiUInt32 TestState::InstanceData::s_uiConstructionCounter = 0;
  xiiUInt32 TestState::InstanceData::s_uiDestructionCounter  = 0;

  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(TestState, 1, xiiRTTIDefaultAllocator<TestState>)
  XII_END_DYNAMIC_REFLECTED_TYPE;

  class TestTransition : public xiiStateMachineTransition
  {
  public:
    bool IsConditionMet(xiiStateMachineInstance& ref_instance, void* pInstanceData) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_uiConditionCounter++;

      return pData->m_uiConditionCounter > 1;
    }

    bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) override
    {
      out_desc.FillFromType<InstanceData>();
      return true;
    }

    struct InstanceData
    {
      InstanceData() { s_uiConstructionCounter++; }
      ~InstanceData() { s_uiDestructionCounter++; }

      xiiUInt32 m_uiConditionCounter;

      static xiiUInt32 s_uiConstructionCounter;
      static xiiUInt32 s_uiDestructionCounter;
    };
  };

  xiiUInt32 TestTransition::InstanceData::s_uiConstructionCounter = 0;
  xiiUInt32 TestTransition::InstanceData::s_uiDestructionCounter  = 0;

  static void ResetCounter()
  {
    TestState::InstanceData::s_uiConstructionCounter      = 0;
    TestState::InstanceData::s_uiDestructionCounter       = 0;
    TestTransition::InstanceData::s_uiConstructionCounter = 0;
    TestTransition::InstanceData::s_uiDestructionCounter  = 0;
  }

  static xiiTime s_TimeStep = xiiTime::MakeFromMilliseconds(10);

} // namespace

void xiiGameEngineTestStateMachine::RunBuiltinsTest()
{
  xiiReflectedClass fakeOwner;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simple States")
  {
    ResetCounter();

    xiiSharedPtr<xiiStateMachineDescription> pDesc = XII_DEFAULT_NEW(xiiStateMachineDescription);

    auto pStateA = XII_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = XII_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pTransition = XII_DEFAULT_NEW(TestTransition);
    pDesc->AddTransition(1, 0, pTransition);

    xiiStateMachineInstance* pInstance = nullptr;
    {
      xiiStateMachineInstance sm(fakeOwner, pDesc);
      XII_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 2);
      XII_TEST_INT(TestTransition::InstanceData::s_uiConstructionCounter, 1);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 0);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      xiiHashedString sStateName; // intentionally left empty to go to fallback state (state with index 0 -> state "A")
      XII_TEST_BOOL(sm.SetStateOrFallback(sStateName).Succeeded());
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      XII_TEST_BOOL(sm.SetState(pStateB).Succeeded());
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      sStateName.Assign("C");
      XII_TEST_BOOL(sm.SetState(sStateName).Failed());

      // no transition yet
      sm.Update(s_TimeStep);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // go back to "A"
      sm.Update(s_TimeStep);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 2);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 1);

      pInstance = &sm; // will be dead after this line but we only need the pointer
    }

    XII_TEST_INT(TestState::InstanceData::s_uiDestructionCounter, 2);
    XII_TEST_INT(TestTransition::InstanceData::s_uiDestructionCounter, 1);
    XII_TEST_INT(pStateA->m_CounterTable[pInstance].m_uiEnterCounter, 2);
    XII_TEST_INT(pStateA->m_CounterTable[pInstance].m_uiExitCounter, 2);
    XII_TEST_INT(pStateB->m_CounterTable[pInstance].m_uiEnterCounter, 1);
    XII_TEST_INT(pStateB->m_CounterTable[pInstance].m_uiExitCounter, 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Blackboard Transition")
  {
    ResetCounter();

    xiiSharedPtr<xiiStateMachineDescription> pDesc = XII_DEFAULT_NEW(xiiStateMachineDescription);

    auto pStateA = XII_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = XII_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pStateC = XII_DEFAULT_NEW(TestState, "C");
    pDesc->AddState(pStateC);

    xiiHashedString sTestVal  = xiiMakeHashedString("TestVal");
    xiiHashedString sTestVal2 = xiiMakeHashedString("TestVal2");

    {
      auto  pTransition       = XII_DEFAULT_NEW(xiiStateMachineTransition_BlackboardConditions);
      auto& cond              = pTransition->m_Conditions.ExpandAndGetRef();
      cond.m_sEntryName       = sTestVal;
      cond.m_fComparisonValue = 2;
      cond.m_Operator         = xiiComparisonOperator::Greater;

      auto& cond2              = pTransition->m_Conditions.ExpandAndGetRef();
      cond2.m_sEntryName       = sTestVal2;
      cond2.m_fComparisonValue = 10;
      cond2.m_Operator         = xiiComparisonOperator::Equal;

      pDesc->AddTransition(0, 1, pTransition);
    }

    {
      auto pTransition        = XII_DEFAULT_NEW(xiiStateMachineTransition_BlackboardConditions);
      pTransition->m_Operator = xiiStateMachineLogicOperator::Or;

      auto& cond              = pTransition->m_Conditions.ExpandAndGetRef();
      cond.m_sEntryName       = sTestVal;
      cond.m_fComparisonValue = 3;
      cond.m_Operator         = xiiComparisonOperator::Greater;

      auto& cond2              = pTransition->m_Conditions.ExpandAndGetRef();
      cond2.m_sEntryName       = sTestVal2;
      cond2.m_fComparisonValue = 20;
      cond2.m_Operator         = xiiComparisonOperator::Equal;

      pDesc->AddTransition(1, 2, pTransition);
    }

    {
      xiiSharedPtr<xiiBlackboard> pBlackboard = xiiBlackboard::Create();
      pBlackboard->SetEntryValue(sTestVal, 2);
      pBlackboard->SetEntryValue(sTestVal2, 0);

      xiiStateMachineInstance sm(fakeOwner, pDesc);
      sm.SetBlackboard(pBlackboard);
      XII_TEST_BOOL(sm.SetState(pStateA).Succeeded());

      // no transition yet since only part of the conditions is true
      pBlackboard->SetEntryValue(sTestVal, 3);
      sm.Update(s_TimeStep);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
      XII_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 0);
      XII_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);

      // transition to B
      pBlackboard->SetEntryValue(sTestVal2, 10);
      sm.Update(s_TimeStep);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
      XII_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 0);
      XII_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);

      // transition to C, only part of the condition needed because of 'OR' operator
      pBlackboard->SetEntryValue(sTestVal2, 20);
      sm.Update(s_TimeStep);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 1);
      XII_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Timeout Transition")
  {
    ResetCounter();

    xiiSharedPtr<xiiStateMachineDescription> pDesc = XII_DEFAULT_NEW(xiiStateMachineDescription);

    auto pStateA = XII_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = XII_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pTransition       = XII_DEFAULT_NEW(xiiStateMachineTransition_Timeout);
    pTransition->m_Timeout = xiiTime::MakeFromMilliseconds(5);
    pDesc->AddTransition(0, 1, pTransition);

    {
      xiiStateMachineInstance sm(fakeOwner, pDesc);
      XII_TEST_BOOL(sm.SetState(pStateA).Succeeded());

      sm.Update(s_TimeStep);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      sm.Update(s_TimeStep);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compounds")
  {
    ResetCounter();

    xiiSharedPtr<xiiStateMachineDescription> pDesc = XII_DEFAULT_NEW(xiiStateMachineDescription);

    auto pCompoundState = XII_DEFAULT_NEW(xiiStateMachineState_Compound, "A");
    {
      auto pAllocator = xiiGetStaticRTTI<TestState>()->GetAllocator();
      pCompoundState->m_SubStates.PushBack(pAllocator->Allocate<TestState>());
      pCompoundState->m_SubStates.PushBack(pAllocator->Allocate<TestState>());
    }
    pDesc->AddState(pCompoundState);

    auto pStateB = XII_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    xiiHashedString sTestVal = xiiMakeHashedString("TestVal");

    {
      auto pCompoundTransition = XII_DEFAULT_NEW(xiiStateMachineTransition_Compound);

      {
        auto pAllocator     = xiiGetStaticRTTI<xiiStateMachineTransition_BlackboardConditions>()->GetAllocator();
        auto pSubTransition = pAllocator->Allocate<xiiStateMachineTransition_BlackboardConditions>();

        auto& cond              = pSubTransition->m_Conditions.ExpandAndGetRef();
        cond.m_sEntryName       = sTestVal;
        cond.m_fComparisonValue = 2;
        cond.m_Operator         = xiiComparisonOperator::Greater;

        pCompoundTransition->m_SubTransitions.PushBack(pSubTransition);
      }

      {
        auto pAllocator           = xiiGetStaticRTTI<xiiStateMachineTransition_Timeout>()->GetAllocator();
        auto pSubTransition       = pAllocator->Allocate<xiiStateMachineTransition_Timeout>();
        pSubTransition->m_Timeout = xiiTime::MakeFromMilliseconds(5);

        pCompoundTransition->m_SubTransitions.PushBack(pSubTransition);
      }

      pDesc->AddTransition(0, 1, pCompoundTransition);
    }

    {
      xiiSharedPtr<xiiBlackboard> pBlackboard = xiiBlackboard::Create();
      pBlackboard->SetEntryValue(sTestVal, 2);

      xiiStateMachineInstance sm(fakeOwner, pDesc);
      sm.SetBlackboard(pBlackboard);
      XII_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 1); // Compound instance data not constructed yet

      XII_TEST_BOOL(sm.SetState(pCompoundState).Succeeded());
      XII_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 3);
      XII_TEST_INT(xiiStaticCast<TestState*>(pCompoundState->m_SubStates[0])->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(xiiStaticCast<TestState*>(pCompoundState->m_SubStates[1])->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // no transition yet because timeout is not reached yet
      pBlackboard->SetEntryValue(sTestVal, 3);
      sm.Update(s_TimeStep);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // all conditions met, transition to B
      sm.Update(s_TimeStep);
      XII_TEST_INT(xiiStaticCast<TestState*>(pCompoundState->m_SubStates[0])->m_CounterTable[&sm].m_uiExitCounter, 1);
      XII_TEST_INT(xiiStaticCast<TestState*>(pCompoundState->m_SubStates[1])->m_CounterTable[&sm].m_uiExitCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      XII_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
    }

    XII_TEST_INT(TestState::InstanceData::s_uiDestructionCounter, 3);
  }
}
