/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

class xiiObjectAccessorBase;

/// Adds the initial thumbnail camera, native atmosphere, directional light and sphere to a scene.
/// Uses the document accessor's transaction/undo mechanism; failure cancels all changes.
/// Existing content is preserved. Call once when creating a new scene, outside another transaction.
XII_SHAREDPLUGINSCENE_DLL xiiStatus xiiCreateDefaultScene(xiiObjectAccessorBase& ref_accessor);

/// Authors a box using the shipped unit-box mesh and native mesh component, with its dimensions
/// represented by the object's local transform. Failure cancels the transaction and clears the
/// returned identity. The optional material overrides slot zero. Existing scene content is retained.
XII_SHAREDPLUGINSCENE_DLL xiiStatus xiiCreateSceneBox(xiiObjectAccessorBase& ref_accessor, const xiiBoundingBox& bounds,
                                                      const xiiUuid& material, xiiUuid& out_object);
