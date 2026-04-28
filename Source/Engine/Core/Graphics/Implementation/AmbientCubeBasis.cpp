/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Graphics/AmbientCubeBasis.h>

xiiVec3 xiiAmbientCubeBasis::s_Dirs[NumDirs] = {xiiVec3(1.0f, 0.0f, 0.0f), xiiVec3(-1.0f, 0.0f, 0.0f), xiiVec3(0.0f, 1.0f, 0.0f),
                                                xiiVec3(0.0f, -1.0f, 0.0f), xiiVec3(0.0f, 0.0f, 1.0f), xiiVec3(0.0f, 0.0f, -1.0f)};

XII_STATICLINK_FILE(Core, Core_Graphics_Implementation_AmbientCubeBasis);
