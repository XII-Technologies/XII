/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/StringView.h>

class xiiQtEditorApp;

/// Verifies dependency metadata from a saved prefab and parameter overrides authored through commands.
void TestExposedAssetDependencies(xiiQtEditorApp& editor, xiiStringView projectDirectory);
