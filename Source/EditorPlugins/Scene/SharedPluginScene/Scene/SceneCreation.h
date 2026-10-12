/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/Status.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

class xiiObjectAccessorBase;

/// Adds the initial thumbnail camera, native atmosphere, directional light and sphere to a scene.
/// Uses the document accessor's transaction/undo mechanism; failure cancels all changes.
/// Existing content is preserved. Call once when creating a new scene, outside another transaction.
XII_SHAREDPLUGINSCENE_DLL xiiStatus xiiCreateDefaultScene(xiiObjectAccessorBase& ref_accessor);
