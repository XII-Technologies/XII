#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GameEngine/Configuration/InputConfig.h>

static_assert(xiiGameAppInputConfig::MaxInputSlotAlternatives == xiiInputActionConfig::MaxInputSlotAlternatives, "Max values should be kept in sync");

xiiGameAppInputConfig::xiiGameAppInputConfig()
{
  for (xiiUInt16 i = 0; i < MaxInputSlotAlternatives; ++i)
  {
    m_fInputSlotScale[i]   = 1.0f;
    m_sInputSlotTrigger[i] = xiiInputSlot_None;
  }
}

void xiiGameAppInputConfig::Apply() const
{
  xiiInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = m_bApplyTimeScaling;

  for (xiiUInt32 i = 0; i < MaxInputSlotAlternatives; ++i)
  {
    cfg.m_sInputSlotTrigger[i] = m_sInputSlotTrigger[i];
    cfg.m_fInputSlotScale[i]   = m_fInputSlotScale[i];
  }

  xiiInputManager::SetInputActionConfig(m_sInputSet, m_sInputAction, cfg, true);
}

void xiiGameAppInputConfig::WriteToDDL(xiiStreamWriter& inout_stream, const xiiArrayPtr<xiiGameAppInputConfig>& actions)
{
  xiiOpenDdlWriter writer;
  writer.SetCompactMode(false);
  writer.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
  writer.SetPrimitiveTypeStringMode(xiiOpenDdlWriter::TypeStringMode::Compliant);
  writer.SetOutputStream(&inout_stream);

  for (const xiiGameAppInputConfig& config : actions)
  {
    config.WriteToDDL(writer);
  }
}

void xiiGameAppInputConfig::WriteToDDL(xiiOpenDdlWriter& ref_writer) const
{
  ref_writer.BeginObject("InputAction");
  {
    xiiOpenDdlUtils::StoreString(ref_writer, m_sInputSet, "Set");
    xiiOpenDdlUtils::StoreString(ref_writer, m_sInputAction, "Action");
    xiiOpenDdlUtils::StoreBool(ref_writer, m_bApplyTimeScaling, "TimeScale");

    for (int i = 0; i < 3; ++i)
    {
      if (!m_sInputSlotTrigger[i].IsEmpty())
      {
        ref_writer.BeginObject("Slot");
        {
          xiiOpenDdlUtils::StoreString(ref_writer, m_sInputSlotTrigger[i], "Key");
          xiiOpenDdlUtils::StoreFloat(ref_writer, m_fInputSlotScale[i], "Scale");
        }
        ref_writer.EndObject();
      }
    }
  }
  ref_writer.EndObject();
}

void xiiGameAppInputConfig::ReadFromDDL(xiiStreamReader& inout_stream, xiiHybridArray<xiiGameAppInputConfig, 32>& out_actions)
{
  xiiOpenDdlReader reader;

  if (reader.ParseDocument(inout_stream, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
    return;

  const xiiOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const xiiOpenDdlReaderElement* pAction = pRoot->GetFirstChild(); pAction != nullptr; pAction = pAction->GetSibling())
  {
    if (!pAction->IsCustomType("InputAction"))
      continue;

    xiiGameAppInputConfig& cfg = out_actions.ExpandAndGetRef();

    cfg.ReadFromDDL(pAction);
  }
}

void xiiGameAppInputConfig::ReadFromDDL(const xiiOpenDdlReaderElement* pInput)
{
  const xiiOpenDdlReaderElement* pSet       = pInput->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Set");
  const xiiOpenDdlReaderElement* pAction    = pInput->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Action");
  const xiiOpenDdlReaderElement* pTimeScale = pInput->FindChildOfType(xiiOpenDdlPrimitiveType::Bool, "TimeScale");


  if (pSet)
    m_sInputSet = pSet->GetPrimitivesString()[0];

  if (pAction)
    m_sInputAction = pAction->GetPrimitivesString()[0];

  if (pTimeScale)
    m_bApplyTimeScaling = pTimeScale->GetPrimitivesBool()[0];

  xiiInt32 iSlot = 0;
  for (const xiiOpenDdlReaderElement* pSlot = pInput->GetFirstChild(); pSlot != nullptr; pSlot = pSlot->GetSibling())
  {
    if (!pSlot->IsCustomType("Slot"))
      continue;

    const xiiOpenDdlReaderElement* pKey   = pSlot->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Key");
    const xiiOpenDdlReaderElement* pScale = pSlot->FindChildOfType(xiiOpenDdlPrimitiveType::Float, "Scale");

    if (pKey)
      m_sInputSlotTrigger[iSlot] = pKey->GetPrimitivesString()[0];

    if (pScale)
      m_fInputSlotScale[iSlot] = pScale->GetPrimitivesFloat()[0];

    ++iSlot;

    if (iSlot >= MaxInputSlotAlternatives)
      break;
  }
}

void xiiGameAppInputConfig::ApplyAll(const xiiArrayPtr<xiiGameAppInputConfig>& actions)
{
  for (const xiiGameAppInputConfig& config : actions)
  {
    config.Apply();
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_InputConfig);
