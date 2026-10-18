/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Foundation/Types/Variant.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>

/// Resolve reflected display overrides when configuration changes, not in execution callbacks.
XII_GRAPHICSCORE_DLL xiiResult xiiResolvePipelineDisplaySettings(const xiiVariantDictionary& parameters, xiiDisplayOutputSettings& inout_settings);
