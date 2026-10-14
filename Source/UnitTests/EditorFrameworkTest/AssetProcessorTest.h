/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/StringView.h>

class xiiQtEditorApp;
class xiiEditorProcessCommunicationChannel;
class xiiProcessAssetMsg;
struct xiiTransformStatus;
struct xiiLogEntry;
void TestAssetProcessor(xiiQtEditorApp& editor, xiiStringView projectDirectory);
bool SendAssetProcessorRequest(xiiEditorProcessCommunicationChannel& processor, xiiProcessAssetMsg& request, xiiTransformStatus& out_response, xiiDynamicArray<xiiLogEntry>& out_logs);
void TestAssetProcessorProfileReload(xiiQtEditorApp& editor, xiiStringView projectDirectory, xiiEditorProcessCommunicationChannel& processor);
void TestAssetProcessorCommandLine(xiiStringView projectDirectory);
