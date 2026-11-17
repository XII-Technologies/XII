/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/Console/Console.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Diagnostics/TelemetryService.h>

void AddConsoleEventHandler()
{
  xiiApplication::GetApplicationInstance()->CreateTelemetryService().SetConsoleHandler([](xiiStringView input, bool complete) {
    xiiTelemetryCommandResponse response;
    auto*                       console     = xiiConsole::GetMainConsole();
    auto*                       interpreter = console ? console->GetCommandInterpreter().Borrow() : nullptr;
    if (!interpreter)
    {
      response.m_uiError = static_cast<xiiUInt8>(xiiNetworkDataErrorCode::UnknownRpc);
      return response;
    }
    xiiCommandInterpreterState state;
    state.m_sInput = input;
    xiiStringBuilder output;
    if (complete)
    {
      interpreter->AutoComplete(state);
      output.AppendFormat(";;00||<{}", state.m_sInput);
    }
    else
      interpreter->Interpret(state);
    for (const auto& line : state.m_sOutput)
    {
      if (line.m_sText.GetElementCount() > 32768 || output.GetElementCount() + line.m_sText.GetElementCount() > 60000)
      {
        response.m_uiError = static_cast<xiiUInt8>(xiiNetworkDataErrorCode::SizeLimit);
        break;
      }
      output.AppendFormat(";;{}||{}", xiiArgI(static_cast<xiiInt32>(line.m_Type), 2, true), line.m_sText);
    }
    response.m_sOutput = output;
    return response;
  });
}

void RemoveConsoleEventHandler()
{
  xiiApplication::GetApplicationInstance()->GetTelemetryService()->SetConsoleHandler({});
}
