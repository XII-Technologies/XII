#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

xiiVisualScriptInstance::xiiVisualScriptInstance(xiiReflectedClass& inout_owner, xiiWorld* pWorld, const xiiSharedPtr<xiiVisualScriptDataStorage>& pConstantDataStorage, const xiiSharedPtr<const xiiVisualScriptDataDescription>& pInstanceDataDesc, const xiiSharedPtr<xiiVisualScriptInstanceDataMapping>& pInstanceDataMapping) :
  xiiScriptInstance(inout_owner, pWorld), m_pConstantDataStorage(pConstantDataStorage), m_pInstanceDataMapping(pInstanceDataMapping), m_InstanceDataStorage(pInstanceDataDesc)
{
  if (pInstanceDataDesc != nullptr)
  {
    m_InstanceDataStorage.AllocateStorage(xiiScriptAllocator::GetAllocator());

    for (auto& it : m_pInstanceDataMapping->m_Content)
    {
      auto& instanceData = it.Value();
      m_InstanceDataStorage.SetDataFromVariant(instanceData.m_DataOffset, instanceData.m_DefaultValue, 0);
    }
  }
}

void xiiVisualScriptInstance::SetInstanceVariable(const xiiHashedString& sName, const xiiVariant& value)
{
  if (m_pInstanceDataMapping == nullptr)
    return;

  xiiVisualScriptInstanceData* pInstanceData = nullptr;
  if (m_pInstanceDataMapping->m_Content.TryGetValue(sName, pInstanceData) == false)
    return;

  xiiResult            conversionStatus = XII_FAILURE;
  xiiVariantType::Enum targetType       = xiiVisualScriptDataType::GetVariantType(pInstanceData->m_DataOffset.GetType());

  xiiVariant convertedValue = value.ConvertTo(targetType, &conversionStatus);
  if (conversionStatus.Failed())
  {
    xiiLog::Error("Can't apply instance variable '{}' because the given value of type '{}' can't be converted the expected target type '{}'", sName, value.GetType(), targetType);
    return;
  }

  m_InstanceDataStorage.SetDataFromVariant(pInstanceData->m_DataOffset, convertedValue, 0);
}

xiiVariant xiiVisualScriptInstance::GetInstanceVariable(const xiiHashedString& sName)
{
  if (m_pInstanceDataMapping == nullptr)
    return xiiVariant();

  xiiVisualScriptInstanceData* pInstanceData = nullptr;
  if (m_pInstanceDataMapping->m_Content.TryGetValue(sName, pInstanceData) == false)
    return xiiVariant();

  return m_InstanceDataStorage.GetDataAsVariant(pInstanceData->m_DataOffset, nullptr, 0);
}
