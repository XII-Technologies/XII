#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Profiling/ProfilingUtils.h>

xiiResult xiiProfilingUtils::SaveProfilingCapture(xiiStringView sCapturePath)
{
  xiiFileWriter fileWriter;
  if (fileWriter.Open(sCapturePath) == XII_SUCCESS)
  {
    xiiProfilingSystem::ProfilingData profilingData;
    xiiProfilingSystem::Capture(profilingData);
    // Set sort index to -1 so that the editor is always on top when opening the trace.
    profilingData.m_uiProcessSortIndex = -1;
    if (profilingData.Write(fileWriter).Failed())
    {
      xiiLog::Error("Failed to write profiling capture: {0}.", sCapturePath);
      return XII_FAILURE;
    }

    xiiLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
  }
  else
  {
    xiiLog::Error("Could not write profiling capture to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiProfilingUtils::MergeProfilingCaptures(xiiStringView sCapturePath1, xiiStringView sCapturePath2, xiiStringView sMergedCapturePath)
{
  xiiString sFirstProfilingJson;
  {
    xiiFileReader reader;
    if (reader.Open(sCapturePath1).Failed())
    {
      xiiLog::Error("Failed to read first profiling capture to be merged: {}.", sCapturePath1);
      return XII_FAILURE;
    }
    sFirstProfilingJson.ReadAll(reader);
  }
  xiiString sSecondProfilingJson;
  {
    xiiFileReader reader;
    if (reader.Open(sCapturePath2).Failed())
    {
      xiiLog::Error("Failed to read second profiling capture to be merged: {}.", sCapturePath2);
      return XII_FAILURE;
    }
    sSecondProfilingJson.ReadAll(reader);
  }

  xiiStringBuilder sMergedProfilingJson;
  {
    // Just glue the array together
    sMergedProfilingJson.Reserve(sFirstProfilingJson.GetElementCount() + 1 + sSecondProfilingJson.GetElementCount());
    const char* szEndArray = sFirstProfilingJson.FindLastSubString("]");
    sMergedProfilingJson.Append(xiiStringView(sFirstProfilingJson.GetData(), static_cast<xiiUInt32>(szEndArray - sFirstProfilingJson.GetData())));
    sMergedProfilingJson.Append(",");
    const char* szStartArray = sSecondProfilingJson.FindSubString("[") + 1;
    sMergedProfilingJson.Append(xiiStringView(szStartArray, static_cast<xiiUInt32>(sSecondProfilingJson.GetElementCount() - (szStartArray - sSecondProfilingJson.GetData()))));
  }

  xiiFileWriter fileWriter;
  if (fileWriter.Open(sMergedCapturePath).Failed() || fileWriter.WriteBytes(sMergedProfilingJson.GetData(), sMergedProfilingJson.GetElementCount()).Failed())
  {
    xiiLog::Error("Failed to write merged profiling capture: {}.", sMergedCapturePath);
    return XII_FAILURE;
  }
  xiiLog::Info("Merged profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
  return XII_SUCCESS;
}
