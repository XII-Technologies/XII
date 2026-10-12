/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Animation/ExposedBone.h>

class xiiDocumentObject;
class xiiObjectAccessorBase;

/// Resolves skeleton asset metadata and sparse native pose overrides into runtime-coordinate bones.
/// The optional <root-transform> metadata is baked into default root joints exactly once and is not a joint.
/// Overrides are absolute native local transforms. Unknown joint names are retained by the component but ignored here.
/// Invalid metadata, hierarchy, transforms or root matrices that cannot be represented as TRS fail and clear the output.
XII_EDITORFRAMEWORK_DLL xiiResult xiiResolveSkeletonPoseForAuthoring(xiiArrayPtr<const xiiExposedBone>      metadata,
                                                                     const xiiMap<xiiString, xiiTransform>& overrides, xiiDynamicArray<xiiExposedBone>& out_bones);

/// Writes native bone overrides through the command accessor. The caller owns the transaction and must cancel it on failure.
/// Requires a transform map identified by xiiBoneManipulatorAttribute; validates all incoming transforms before writing.
XII_EDITORFRAMEWORK_DLL xiiStatus xiiApplySkeletonPoseForAuthoring(xiiObjectAccessorBase& accessor, const xiiDocumentObject* pComponent,
                                                                   const xiiMap<xiiString, xiiTransform>& transforms);
