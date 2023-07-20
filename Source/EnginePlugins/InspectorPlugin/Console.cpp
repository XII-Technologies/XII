#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/Console/Console.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>

static void TelemetryMessage(void* pPassThrough)
{
  xiiTelemetryMessage Msg;
  xiiStringBuilder    input;

  while (xiiTelemetry::RetrieveMessage('CMD', Msg) == XII_SUCCESS)
  {
    if (Msg.GetMessageID() == 'EXEC' || Msg.GetMessageID() == 'COMP')
    {
      Msg.GetReader() >> input;

      if (xiiConsole::GetMainConsole())
      {
        if (auto pInt = xiiConsole::GetMainConsole()->GetCommandInterpreter())
        {
          xiiCommandInterpreterState s;
          s.m_sInput = input;

          xiiStringBuilder encoded;

          if (Msg.GetMessageID() == 'EXEC')
          {
            pInt->Interpret(s);
          }
          else
          {
            pInt->AutoComplete(s);
            encoded.AppendFormat(";;00||<{}", s.m_sInput);
          }

          for (const auto& l : s.m_sOutput)
          {
            encoded.AppendFormat(";;{}||{}", xiiArgI((xiiInt32)l.m_Type, 2, true), l.m_sText);
          }

          xiiTelemetryMessage msg;
          msg.SetMessageID('CMD', 'RES');
          msg.GetWriter() << encoded;
          xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
        }
      }
    }
  }
}

void AddConsoleEventHandler()
{
  xiiTelemetry::AcceptMessagesForSystem('CMD', true, TelemetryMessage, nullptr);
}

void RemoveConsoleEventHandler()
{
  xiiTelemetry::AcceptMessagesForSystem('CMD', false);
}
