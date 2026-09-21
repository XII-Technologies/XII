/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Types/RangeView.h>
#include <GameEngine/Components/StateMachine/StateMachine.h>

/// A state machine state implementation that can be scripted using e.g. visual scripting.
class XII_GAMEENGINE_DLL xiiStateMachineState_Script : public xiiStateMachineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineState_Script, xiiStateMachineState);

public:
  xiiStateMachineState_Script(xiiStringView sName = xiiStringView());
  ~xiiStateMachineState_Script();

  virtual void OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const override;
  virtual void OnExit(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pToState) const override;
  virtual void Update(xiiStateMachineInstance& ref_instance, void* pInstanceData, xiiTime deltaTime) const override;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) override;

  void          SetScriptClassFile(xiiStringView sFile); // [ property ]
  xiiStringView GetScriptClassFile() const;              // [ property ]

  // Exposed Parameters
  const xiiRangeView<xiiStringView, xiiUInt32> GetParameters() const;
  void                                         SetParameter(xiiStringView sKey, const xiiVariant& value);
  void                                         RemoveParameter(xiiStringView sKey);
  bool                                         GetParameter(xiiStringView sKey, xiiVariant& out_value) const;

private:
  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;

  xiiString m_sScriptClassFile;
};
