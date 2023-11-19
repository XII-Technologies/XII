#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

xiiVisualScriptInstance::xiiVisualScriptInstance(xiiReflectedClass& inout_owner, xiiWorld* pWorld, const xiiSharedPtr<xiiVisualScriptDataStorage>& pConstantDataStorage, const xiiSharedPtr<const xiiVisualScriptDataDescription>& pInstanceDataDesc, const xiiSharedPtr<xiiVisualScriptInstanceDataMapping>& pInstanceDataMapping)
  : xiiScriptInstance(inout_owner, pWorld)
  , m_pConstantDataStorage(pConstantDataStorage)
  , m_pInstanceDataMapping(pInstanceDataMapping)
{
  if (pInstanceDataDesc != nullptr)
  {
    m_pInstanceDataStorage = XII_SCRIPT_NEW(xiiVisualScriptDataStorage, pInstanceDataDesc);
    m_pInstanceDataStorage->AllocateStorage();

    for (auto& it : m_pInstanceDataMapping->m_Content)
    {
      auto& instanceData = it.Value();
      m_pInstanceDataStorage->SetDataFromVariant(instanceData.m_DataOffset, instanceData.m_DefaultValue, 0);
    }
  }
}

void xiiVisualScriptInstance::ApplyParameters(const xiiArrayMap<xiiHashedString, xiiVariant>& parameters)
{
  if (m_pInstanceDataMapping == nullptr)
    return;

  for (auto it : parameters)
  {
    xiiVisualScriptInstanceData* pInstanceData = nullptr;
    if (m_pInstanceDataMapping->m_Content.TryGetValue(it.key, pInstanceData))
    {
      xiiResult conversionStatus = XII_FAILURE;
      xiiVariantType::Enum targetType = xiiVisualScriptDataType::GetVariantType(pInstanceData->m_DataOffset.GetType());

      xiiVariant convertedValue = it.value.ConvertTo(targetType, &conversionStatus);
      if (conversionStatus.Failed())
      {
        xiiLog::Error("Can't apply script parameter '{}' because the given value of type '{}' can't be converted the expected target type '{}'", it.key, it.value.GetType(), targetType);
        continue;
      }

      m_pInstanceDataStorage->SetDataFromVariant(pInstanceData->m_DataOffset, convertedValue, 0);
    }
  }
}
