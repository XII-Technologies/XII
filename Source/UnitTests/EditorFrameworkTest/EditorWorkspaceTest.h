/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/StringView.h>
#include <Foundation/Types/Uuid.h>

class xiiSceneDocument;
class xiiQtEngineDocumentWindow;
class xiiQtEngineViewWidget;

void TestEditorWorkspace(xiiSceneDocument& scene, xiiQtEngineDocumentWindow& window, xiiQtEngineViewWidget& view, const xiiUuid& selectedObject, xiiStringView sOutputDirectory);
