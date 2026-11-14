/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Reflection/Implementation/PropertyAttributes.h>

/// Editor adapters operate exclusively on reflected authored properties.
class XII_JOLTPLUGIN_DLL xiiJoltShapeManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltShapeManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiJoltShapeManipulatorAttribute(xiiStringView sSize = {}, xiiStringView sHeight = {}) :
    xiiManipulatorAttribute(sSize, sHeight, "LocalPosition", "LocalRotation") {}
};

class XII_JOLTPLUGIN_DLL xiiJoltJointManipulatorAttribute : public xiiManipulatorAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltJointManipulatorAttribute, xiiManipulatorAttribute);

public:
  xiiJoltJointManipulatorAttribute(xiiStringView sFrame = {}, xiiStringView sActor = {}) : xiiManipulatorAttribute(sFrame, sActor) {}
};

class XII_JOLTPLUGIN_DLL xiiJoltVisualizerAttribute : public xiiVisualizerAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltVisualizerAttribute, xiiVisualizerAttribute);

public:
  xiiJoltVisualizerAttribute() : xiiVisualizerAttribute({}) {}
};
