#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Types/RangeView.h>
#include <GameEngine/StateMachine/StateMachine.h>

/// \brief A state machine state implementation that can be scripted using e.g. visual scripting.
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

  virtual bool GetInstanceDataDesc(xiiStateMachineInstanceDataDesc& out_desc) override;

  void        SetScriptClassFile(const char* szFile); // [ property ]
  const char* GetScriptClassFile() const;             // [ property ]

  // Exposed Parameters
  const xiiRangeView<const char*, xiiUInt32> GetParameters() const;
  void                                       SetParameter(const char* szKey, const xiiVariant& value);
  void                                       RemoveParameter(const char* szKey);
  bool                                       GetParameter(const char* szKey, xiiVariant& out_value) const;

private:
  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;

  xiiString m_sScriptClassFile;
};
