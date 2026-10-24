/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <ModelImporter/ModelImporterDLL.h>

class xiiStringBuilder;

struct XII_MODELIMPORTER_DLL xiiModelAnimationImportOptions
{
  xiiString m_sClipName;            ///< Empty selects the first source clip. Unnamed clips receive stable Animation_<index> labels.
  float     m_fSampleRate  = 30.0f; ///< Frames per second for trimming and cubic curve baking, independent of source tick units.
  xiiUInt32 m_uiFirstFrame = 0;
  xiiUInt32 m_uiNumFrames  = 0;     ///< Zero imports all remaining frames; otherwise includes this many sample frames.
  bool      m_bAdditive    = false; ///< Store deltas relative to the first imported pose, including identity for unanimated joints.
  bool      m_bLooping     = true;  ///< Runtime playback wraps at the clip duration; Editor scrubbing remains clamped.
};

struct XII_MODELIMPORTER_DLL xiiModelAnimationImport
{
  xiiAnimationClipResourceDescriptor m_Clip;
  xiiDynamicArray<xiiString>         m_AvailableClips;

  /// Discover all source clip labels without baking any clip. An animation-free model
  /// returns an empty list. Duplicate labels receive stable source-index suffixes.
  /// Failure clears out_names.
  static xiiResult ReadClipNames(xiiStringView sSourceFile, xiiDynamicArray<xiiString>& out_names, xiiStringBuilder* out_pError = nullptr);

  /// Read named joint transform animation through the existing Assimp dependency.
  /// Native linear keys retain source key times and trim boundaries. STEP boundaries
  /// use consecutive representable float times; cubic Hermite curves are baked at
  /// the selected sample rate and retain authored key times. Rotations are normalized.
  /// DEFAULT boundaries use rest transforms except glTF, which clamps as required by its format.
  /// CONSTANT boundaries clamp; per-channel LINEAR extrapolation and REPEAT fail explicitly.
  /// Joint names resolve against the runtime skeleton; source-local joint indices are not persisted.
  /// Failure clears the clip. Discovered clip labels remain available on selection/conversion failure.
  /// Morph/vertex animation, ambiguous joint names, invalid TRS/keys and excessive
  /// output (over one million samples per track or sixteen million keys per clip) fail explicitly.
  static xiiResult ReadFile(xiiStringView sSourceFile, const xiiModelAnimationImportOptions& options, xiiModelAnimationImport& out_import, xiiStringBuilder* out_pError = nullptr);
};
