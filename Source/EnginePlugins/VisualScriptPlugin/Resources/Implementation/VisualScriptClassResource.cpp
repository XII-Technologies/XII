/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <VisualScriptPlugin/Resources/VisualScriptClassResource.h>
#include <VisualScriptPlugin/Runtime/VisualScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptFunctionProperty.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptClassResource, 1, xiiRTTIDefaultAllocator<xiiVisualScriptClassResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiVisualScriptClassResource);

XII_BEGIN_SUBSYSTEM_DECLARATION(VisualScript, VisualScriptResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiResourceManager::RegisterResourceForAssetType("VisualScriptClass", xiiGetStaticRTTI<xiiVisualScriptClassResource>());
    xiiResourceManager::RegisterResourceOverrideType(xiiGetStaticRTTI<xiiVisualScriptClassResource>(), [](const xiiStringBuilder& sResourceID) -> bool  {
      return sResourceID.HasExtension(".xiiBinVisualScriptClass");
    });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::UnregisterResourceOverrideType(xiiGetStaticRTTI<xiiVisualScriptClassResource>());
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiVisualScriptClassResource::xiiVisualScriptClassResource()  = default;
xiiVisualScriptClassResource::~xiiVisualScriptClassResource() = default;

xiiResourceLoadDescription xiiVisualScriptClassResource::UnloadData(Unload WhatToUnload)
{
  DeleteScriptType();
  DeleteAllScriptCoroutineTypes();

  xiiResourceLoadDescription ld;
  ld.m_State                      = xiiResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable    = 0;

  return ld;
}

xiiResourceLoadDescription xiiVisualScriptClassResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDescription ld;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable    = 0;
  ld.m_State                      = xiiResourceState::LoadedResourceMissing;

  if (pStream == nullptr)
  {
    return ld;
  }

  // the standard file reader writes the absolute file path into the stream
  xiiString sAbsFilePath;
  (*pStream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  xiiString                         sScriptClassName;
  const xiiRTTI*                    pBaseClassType = nullptr;
  xiiScriptRTTI::FunctionList       functions;
  xiiScriptRTTI::MessageHandlerList messageHandlers;
  {
    xiiStringDeduplicationReadContext stringDedup(*pStream);

    xiiChunkStreamReader chunk(*pStream);
    chunk.SetEndChunkFileMode(xiiChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "Header")
      {
        xiiString sBaseClassName;
        chunk >> sBaseClassName;
        chunk >> sScriptClassName;
        pBaseClassType = xiiRTTI::FindTypeByName(sBaseClassName);
        if (pBaseClassType == nullptr)
        {
          xiiLog::Error("Invalid base class '{}' for Visual Script Class '{}'", sBaseClassName, sScriptClassName);
          return ld;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "ConstantData")
      {
        xiiSharedPtr<xiiVisualScriptDataDescription> pConstantDataDesc = XII_SCRIPT_NEW(xiiVisualScriptDataDescription);
        if (pConstantDataDesc->Deserialize(chunk).Failed())
        {
          return ld;
        }

        xiiSharedPtr<xiiVisualScriptDataStorage> pConstantDataStorage = XII_SCRIPT_NEW(xiiVisualScriptDataStorage, pConstantDataDesc);
        if (pConstantDataStorage->Deserialize(chunk, xiiScriptAllocator::GetAllocator()).Succeeded())
        {
          m_pConstantDataStorage = pConstantDataStorage;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "InstanceData")
      {
        xiiSharedPtr<xiiVisualScriptDataDescription> pInstanceDataDesc = XII_SCRIPT_NEW(xiiVisualScriptDataDescription);
        if (pInstanceDataDesc->Deserialize(chunk).Succeeded())
        {
          m_pInstanceDataDesc = pInstanceDataDesc;
        }

        xiiSharedPtr<xiiVisualScriptInstanceDataMapping> pInstanceDataMapping = XII_SCRIPT_NEW(xiiVisualScriptInstanceDataMapping);
        if (chunk.ReadHashTable(pInstanceDataMapping->m_Content).Succeeded())
        {
          m_pInstanceDataMapping = pInstanceDataMapping;

          // calculate byte offsets from indices
          for (auto& it : m_pInstanceDataMapping->m_Content)
          {
            auto& dataOffset = it.Value().m_DataOffset;
            dataOffset       = m_pInstanceDataDesc->GetOffset(dataOffset.GetType(), dataOffset.m_uiByteOffset, dataOffset.GetSource());
          }
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "FunctionGraphs")
      {
        xiiUInt32 uiNumFunctions;
        chunk >> uiNumFunctions;

        if (m_pInstanceDataDesc == nullptr || m_pConstantDataStorage == nullptr)
        {
          xiiLog::Error("Old visual script, needs re-export");
          return ld;
        }

        for (xiiUInt32 i = 0; i < uiNumFunctions; ++i)
        {
          xiiString                                     sFunctionName;
          xiiEnum<xiiVisualScriptNodeDescription::Type> functionType;
          xiiEnum<xiiScriptCoroutineCreationMode>       coroutineCreationMode;
          chunk >> sFunctionName;
          chunk >> functionType;
          chunk >> coroutineCreationMode;

          xiiUniquePtr<xiiVisualScriptGraphDescription> pDesc = XII_SCRIPT_NEW(xiiVisualScriptGraphDescription);
          if (pDesc->Deserialize(chunk, *m_pInstanceDataDesc, m_pConstantDataStorage->GetDesc()).Failed())
          {
            xiiLog::Error("Invalid visual script desc");
            return ld;
          }

          if (functionType == xiiVisualScriptNodeDescription::Type::EntryCall)
          {
            xiiUniquePtr<xiiVisualScriptFunctionProperty> pFunctionProperty = XII_SCRIPT_NEW(xiiVisualScriptFunctionProperty, sFunctionName, std::move(pDesc));
            functions.PushBack(std::move(pFunctionProperty));
          }
          else if (functionType == xiiVisualScriptNodeDescription::Type::EntryCall_Coroutine)
          {
            xiiUniquePtr<xiiVisualScriptCoroutineAllocator>  pCoroutineAllocator = XII_SCRIPT_NEW(xiiVisualScriptCoroutineAllocator, std::move(pDesc));
            auto                                             pCoroutineType      = CreateScriptCoroutineType(sScriptClassName, sFunctionName, std::move(pCoroutineAllocator));
            xiiUniquePtr<xiiScriptCoroutineFunctionProperty> pFunctionProperty   = XII_SCRIPT_NEW(xiiScriptCoroutineFunctionProperty, sFunctionName, pCoroutineType, coroutineCreationMode);
            functions.PushBack(std::move(pFunctionProperty));
          }
          else if (functionType == xiiVisualScriptNodeDescription::Type::MessageHandler)
          {
            auto                                        desc            = pDesc->GetMessageDesc();
            xiiUniquePtr<xiiVisualScriptMessageHandler> pMessageHandler = XII_SCRIPT_NEW(xiiVisualScriptMessageHandler, desc, std::move(pDesc));
            messageHandlers.PushBack(std::move(pMessageHandler));
          }
          else if (functionType == xiiVisualScriptNodeDescription::Type::MessageHandler_Coroutine)
          {
            auto                                            desc                = pDesc->GetMessageDesc();
            xiiUniquePtr<xiiVisualScriptCoroutineAllocator> pCoroutineAllocator = XII_SCRIPT_NEW(xiiVisualScriptCoroutineAllocator, std::move(pDesc));
            auto                                            pCoroutineType      = CreateScriptCoroutineType(sScriptClassName, sFunctionName, std::move(pCoroutineAllocator));
            xiiUniquePtr<xiiScriptCoroutineMessageHandler>  pMessageHandler     = XII_SCRIPT_NEW(xiiScriptCoroutineMessageHandler, sFunctionName, desc, pCoroutineType, coroutineCreationMode);
            messageHandlers.PushBack(std::move(pMessageHandler));
          }
          else
          {
            xiiLog::Error("Invalid event handler type '{}' for event handler '{}'", xiiVisualScriptNodeDescription::Type::GetName(functionType), sFunctionName);
            return ld;
          }
        }
      }

      chunk.NextChunk();
    }
  }

  CreateScriptType(sScriptClassName, pBaseClassType, std::move(functions), std::move(messageHandlers));

  ld.m_State = xiiResourceState::Loaded;
  return ld;
}

void xiiVisualScriptClassResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (xiiUInt32)sizeof(xiiVisualScriptClassResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

xiiUniquePtr<xiiScriptInstance> xiiVisualScriptClassResource::Instantiate(xiiReflectedClass& inout_owner, xiiWorld* pWorld) const
{
  return XII_SCRIPT_NEW(xiiVisualScriptInstance, inout_owner, pWorld, m_pConstantDataStorage, m_pInstanceDataDesc, m_pInstanceDataMapping);
}

XII_STATICLINK_FILE(VisualScriptPlugin, VisualScriptPlugin_Resources_VisualScriptClassResource);
