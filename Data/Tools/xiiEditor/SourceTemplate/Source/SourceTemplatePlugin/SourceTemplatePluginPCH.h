#pragma once

#include <Foundation/Basics.h>

#include <SourceTemplatePlugin/SourceTemplatePluginDLL.h>

#include <Core/Collection/CollectionResource.h>
#include <Core/Input/InputManager.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Uuid.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <Imgui/imgui.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <Utilities/DataStructures/GameGrid.h>
