/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/StringView.h>

class xiiQtEditorApp;

/// Exercises production scene documents and IPC; native Qt platforms additionally exercise the viewport.
void TestSceneWindow(xiiQtEditorApp& editor, xiiStringView sProjectDirectory);
