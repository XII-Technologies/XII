#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

bool xiiIReflectedTypeAccessor::GetValues(xiiStringView sProperty, xiiDynamicArray<xiiVariant>& out_values) const
{
  xiiHybridArray<xiiVariant, 16> keys;
  if (!GetKeys(sProperty, keys))
    return false;

  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (xiiVariant key : keys)
  {
    out_values.PushBack(GetValue(sProperty, key));
  }
  return true;
}
