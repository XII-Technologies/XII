/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

namespace
{
  // Extend the descriptor's existing reflected property representation. Owned
  // settings use value dictionaries; borrowed object pointers are rejected.
  struct PropertyWalk
  {
    xiiUInt32 m_uiValues = 0;

    bool Spend(xiiUInt32 count = 1)
    {
      if (count > 65536 - m_uiValues)
        return false;
      m_uiValues += count;
      return true;
    }

    static bool IsValue(const xiiAbstractProperty* property)
    {
      return xiiReflectionUtils::IsValueType(property) || property->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags);
    }

    static xiiStringView String(const xiiVariant& value)
    {
      return value.IsA<xiiString>() ? value.Get<xiiString>().GetView() : value.Get<xiiStringView>();
    }

    static xiiResult Convert(const xiiAbstractProperty* property, const xiiVariant& value, xiiVariant& out_value)
    {
      if (property->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
      {
        out_value = value;
        return XII_SUCCESS;
      }
      if (property->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
      {
        if (value.IsString())
        {
          xiiInt64 enumeration = 0;
          if (!xiiReflectionUtils::StringToEnumeration(property->GetSpecificType(), String(value), enumeration))
            return XII_FAILURE;
          out_value = enumeration;
          return XII_SUCCESS;
        }
        xiiResult result = XII_FAILURE;
        out_value        = value.ConvertTo(xiiVariantType::Int64, &result);
        return result;
      }
      if (property->GetSpecificType()->GetVariantType() == xiiVariantType::TypedObject)
      {
        if (value.GetReflectedType() != property->GetSpecificType())
          return XII_FAILURE;
        out_value = value;
        return XII_SUCCESS;
      }
      xiiResult result = XII_FAILURE;
      out_value        = value.ConvertTo(property->GetSpecificType()->GetVariantType(), &result);
      return result;
    }

    xiiResult ReadObject(const void* object, const xiiRTTI* type, xiiVariantDictionary& out_values, xiiUInt32 depth, bool pass = false)
    {
      if (depth >= 32 || !Spend())
        return XII_FAILURE;
      xiiHybridArray<const xiiAbstractProperty*, 32> properties;
      type->GetAllProperties(properties);
      for (const auto* property : properties)
      {
        if (property->GetFlags().IsSet(xiiPropertyFlags::ReadOnly) ||
            (pass && property->GetPropertyName() == "ParameterOverrides") ||
            property->GetCategory() == xiiPropertyCategory::Constant || property->GetCategory() == xiiPropertyCategory::Function)
          continue;
        xiiVariant value;
        XII_SUCCEED_OR_RETURN(ReadProperty(object, property, value, depth + 1));
        // Older assets have no authored pin overrides. Preserve their byte representation.
        if (pass && property->GetPropertyName() == "PinConfigurations" && value.IsA<xiiVariantArray>() && value.Get<xiiVariantArray>().IsEmpty())
          continue;
        out_values.Insert(property->GetPropertyName(), std::move(value));
      }
      return XII_SUCCESS;
    }

    xiiResult ReadOwned(const void* object, const xiiAbstractProperty* property, xiiVariant& out_value, xiiUInt32 depth)
    {
      if (!property->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
        return XII_FAILURE;
      if (!object)
      {
        out_value = {};
        return XII_SUCCESS;
      }
      const auto* type = property->GetSpecificType();
      if (type->IsDerivedFrom<xiiReflectedClass>())
        type = static_cast<const xiiReflectedClass*>(object)->GetDynamicRTTI();
      xiiVariantDictionary values;
      XII_SUCCEED_OR_RETURN(ReadObject(object, type, values, depth));
      if (type != property->GetSpecificType())
        values.Insert("$type", xiiString(type->GetTypeName()));
      out_value = std::move(values);
      return XII_SUCCESS;
    }

    xiiResult ReadProperty(const void* object, const xiiAbstractProperty* property, xiiVariant& out_value, xiiUInt32 depth)
    {
      if (!Spend())
        return XII_FAILURE;
      if (property->GetCategory() == xiiPropertyCategory::Member)
      {
        const auto* member = static_cast<const xiiAbstractMemberProperty*>(property);
        if (IsValue(property))
          out_value = xiiReflectionUtils::GetMemberPropertyValue(member, object);
        else if (property->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          void* child = nullptr;
          member->GetValuePtr(object, &child);
          return ReadOwned(child, property, out_value, depth);
        }
        else if (const void* child = member->GetPropertyPointer(object))
        {
          xiiVariantDictionary values;
          XII_SUCCEED_OR_RETURN(ReadObject(child, property->GetSpecificType(), values, depth));
          out_value = std::move(values);
        }
        else
          return XII_FAILURE;
        return XII_SUCCESS;
      }
      if (property->GetCategory() == xiiPropertyCategory::Array)
      {
        const auto* array = static_cast<const xiiAbstractArrayProperty*>(property);
        if (!Spend(array->GetCount(object)))
          return XII_FAILURE;
        xiiVariantArray values;
        for (xiiUInt32 i = 0; i < array->GetCount(object); ++i)
        {
          xiiVariant value;
          if (IsValue(property))
            value = xiiReflectionUtils::GetArrayPropertyValue(array, object, i);
          else if (property->GetFlags().IsSet(xiiPropertyFlags::Pointer))
          {
            void* child = nullptr;
            array->GetValue(object, i, &child);
            XII_SUCCEED_OR_RETURN(ReadOwned(child, property, value, depth));
          }
          else if (const void* child = array->GetValuePointer(const_cast<void*>(object), i))
          {
            xiiVariantDictionary fields;
            XII_SUCCEED_OR_RETURN(ReadObject(child, property->GetSpecificType(), fields, depth));
            value = std::move(fields);
          }
          else
            return XII_FAILURE;
          values.PushBack(std::move(value));
        }
        out_value = std::move(values);
        return XII_SUCCESS;
      }
      if (property->GetCategory() == xiiPropertyCategory::Map && IsValue(property))
      {
        const auto*                   map = static_cast<const xiiAbstractMapProperty*>(property);
        xiiHybridArray<xiiString, 16> keys;
        map->GetKeys(object, keys);
        if (!Spend(keys.GetCount()))
          return XII_FAILURE;
        xiiVariantDictionary values;
        for (const auto& key : keys)
          values.Insert(key, xiiReflectionUtils::GetMapPropertyValue(map, object, key));
        out_value = std::move(values);
        return XII_SUCCESS;
      }
      return XII_FAILURE;
    }

    xiiResult WriteObject(void* object, const xiiRTTI* type, const xiiVariantDictionary& values, xiiUInt32 depth, bool pass = false)
    {
      if (depth >= 32 || !Spend())
        return XII_FAILURE;
      for (auto it = values.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Key() == "$type" && !pass)
          continue;
        const auto* property = type->FindPropertyByName(it.Key());
        if (!property || property->GetFlags().IsSet(xiiPropertyFlags::ReadOnly) || (pass && it.Key() == "ParameterOverrides"))
          return XII_FAILURE;
        XII_SUCCEED_OR_RETURN(WriteProperty(object, property, it.Value(), depth + 1));
      }
      return XII_SUCCESS;
    }

    xiiResult CreateOwned(const xiiAbstractProperty* property, const xiiVariant& value, xiiTypedPointer& out_object, xiiUInt32 depth)
    {
      out_object = {};
      if (!property->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
        return XII_FAILURE;
      if (!value.IsValid())
        return XII_SUCCESS;
      if (!value.IsA<xiiVariantDictionary>())
        return XII_FAILURE;
      const auto& values = value.Get<xiiVariantDictionary>();
      const auto* type   = property->GetSpecificType();
      if (const auto* derived = values.GetValue("$type"))
      {
        if (!derived->IsString() || !type->IsDerivedFrom<xiiReflectedClass>())
          return XII_FAILURE;
        type = xiiRTTI::FindTypeByName(String(*derived));
        if (!type || !type->IsDerivedFrom(property->GetSpecificType()))
          return XII_FAILURE;
      }
      if (!type->GetAllocator()->CanAllocate())
        return XII_FAILURE;
      void* child = type->GetAllocator()->Allocate<void>();
      if (WriteObject(child, type, values, depth).Failed())
      {
        type->GetAllocator()->Deallocate(child);
        return XII_FAILURE;
      }
      out_object.m_pObject = child;
      out_object.m_pType   = type;
      return XII_SUCCESS;
    }

    xiiResult WriteProperty(void* object, const xiiAbstractProperty* property, const xiiVariant& value, xiiUInt32 depth)
    {
      if (!Spend())
        return XII_FAILURE;
      if (property->GetCategory() == xiiPropertyCategory::Member)
      {
        const auto* member = static_cast<const xiiAbstractMemberProperty*>(property);
        if (IsValue(property))
        {
          xiiVariant converted;
          XII_SUCCEED_OR_RETURN(Convert(property, value, converted));
          xiiReflectionUtils::SetMemberPropertyValue(member, object, converted);
          return XII_SUCCESS;
        }
        if (property->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          xiiTypedPointer replacement;
          XII_SUCCEED_OR_RETURN(CreateOwned(property, value, replacement, depth));
          void* previous = nullptr;
          member->GetValuePtr(object, &previous);
          member->SetValuePtr(object, &replacement.m_pObject);
          if (previous)
            xiiReflectionUtils::DeleteObject(previous, property);
          return XII_SUCCESS;
        }
        void* child = member->GetPropertyPointer(object);
        return child && value.IsA<xiiVariantDictionary>() ? WriteObject(child, property->GetSpecificType(), value.Get<xiiVariantDictionary>(), depth) : XII_FAILURE;
      }
      if (property->GetCategory() == xiiPropertyCategory::Array)
      {
        if (!value.IsA<xiiVariantArray>())
          return XII_FAILURE;
        const auto& values = value.Get<xiiVariantArray>();
        if (!Spend(values.GetCount()))
          return XII_FAILURE;
        const auto* array = static_cast<const xiiAbstractArrayProperty*>(property);
        if (IsValue(property))
        {
          xiiVariantArray converted;
          for (const auto& element : values)
          {
            xiiVariant entry;
            XII_SUCCEED_OR_RETURN(Convert(property, element, entry));
            converted.PushBack(std::move(entry));
          }
          array->SetCount(object, converted.GetCount());
          for (xiiUInt32 i = 0; i < converted.GetCount(); ++i)
            xiiReflectionUtils::SetArrayPropertyValue(array, object, i, converted[i]);
          return XII_SUCCESS;
        }
        if (property->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          xiiDynamicArray<xiiTypedPointer> replacements;
          XII_SCOPE_EXIT(for (const auto& entry : replacements) if (entry.m_pObject) entry.m_pType->GetAllocator()->Deallocate(entry.m_pObject));
          for (const auto& element : values)
          {
            xiiTypedPointer entry;
            XII_SUCCEED_OR_RETURN(CreateOwned(property, element, entry, depth));
            replacements.PushBack(entry);
          }
          for (xiiUInt32 i = array->GetCount(object); i > 0; --i)
          {
            void* previous = nullptr;
            array->GetValue(object, i - 1, &previous);
            array->Remove(object, i - 1);
            if (previous)
              xiiReflectionUtils::DeleteObject(previous, property);
          }
          array->SetCount(object, replacements.GetCount());
          for (xiiUInt32 i = 0; i < replacements.GetCount(); ++i)
          {
            array->SetValue(object, i, &replacements[i].m_pObject);
            replacements[i].m_pObject = nullptr;
          }
          return XII_SUCCESS;
        }
        array->SetCount(object, values.GetCount());
        for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
        {
          void* child = array->GetValuePointer(object, i);
          if (!child || !values[i].IsA<xiiVariantDictionary>())
            return XII_FAILURE;
          XII_SUCCEED_OR_RETURN(WriteObject(child, property->GetSpecificType(), values[i].Get<xiiVariantDictionary>(), depth));
        }
        return XII_SUCCESS;
      }
      if (property->GetCategory() == xiiPropertyCategory::Map && IsValue(property))
      {
        if (!value.IsA<xiiVariantDictionary>())
          return XII_FAILURE;
        const auto& values = value.Get<xiiVariantDictionary>();
        if (!Spend(values.GetCount()))
          return XII_FAILURE;
        xiiVariantDictionary converted;
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          xiiVariant entry;
          XII_SUCCEED_OR_RETURN(Convert(property, it.Value(), entry));
          converted.Insert(it.Key(), std::move(entry));
        }
        const auto* map = static_cast<const xiiAbstractMapProperty*>(property);
        map->Clear(object);
        for (auto it = converted.GetIterator(); it.IsValid(); ++it)
          xiiReflectionUtils::SetMapPropertyValue(map, object, it.Key(), it.Value());
        return XII_SUCCESS;
      }
      return XII_FAILURE;
    }
  };
} // namespace

xiiResult xiiRenderPipelinePass::Configure(const xiiVariantDictionary& properties)
{
  PropertyWalk walk;
  return walk.WriteObject(this, GetDynamicRTTI(), properties, 0, true);
}

xiiResult xiiRenderPipelinePass::GetProperties(xiiVariantDictionary& out_properties) const
{
  PropertyWalk         walk;
  xiiVariantDictionary properties;
  XII_SUCCEED_OR_RETURN(walk.ReadObject(this, GetDynamicRTTI(), properties, 0, true));
  out_properties = std::move(properties);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelinePassProperties);
