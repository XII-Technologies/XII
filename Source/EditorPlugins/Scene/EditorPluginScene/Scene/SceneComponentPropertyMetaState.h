/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>

struct xiiPropertyMetaStateEvent;

XII_EDITORPLUGINSCENE_DLL void xiiCameraComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);
XII_EDITORPLUGINSCENE_DLL void xiiReflectionCaptureComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);
XII_EDITORPLUGINSCENE_DLL void xiiLightComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);
