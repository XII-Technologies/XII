/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>

class xiiOpenDdlWriter;
class xiiOpenDdlReaderElement;

class XII_GAMEENGINE_DLL xiiGameAppInputConfig
{
public:
  constexpr static xiiUInt32 MaxInputSlotAlternatives = 3;

  static constexpr const xiiStringView s_sConfigFile = ":project/RuntimeConfigs/InputConfig.ddl"_xiisv;

  xiiGameAppInputConfig();

  void Apply() const;
  void WriteToDDL(xiiOpenDdlWriter& ref_writer) const;
  void ReadFromDDL(const xiiOpenDdlReaderElement* pAction);

  static void ApplyAll(const xiiArrayPtr<xiiGameAppInputConfig>& actions);
  static void WriteToDDL(xiiStreamWriter& inout_stream, const xiiArrayPtr<xiiGameAppInputConfig>& actions);
  static void ReadFromDDL(xiiStreamReader& inout_stream, xiiHybridArray<xiiGameAppInputConfig, 32>& out_actions);

  xiiString m_sInputSet;
  xiiString m_sInputAction;

  xiiString m_sInputSlotTrigger[MaxInputSlotAlternatives];

  float m_fInputSlotScale[MaxInputSlotAlternatives];

  bool m_bApplyTimeScaling = true;
};
