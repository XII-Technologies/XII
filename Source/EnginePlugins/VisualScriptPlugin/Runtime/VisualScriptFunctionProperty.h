#pragma once

#include <VisualScriptPlugin/Runtime/VisualScript.h>

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptFunctionProperty : public xiiScriptFunctionProperty
{
public:
  xiiVisualScriptFunctionProperty(xiiStringView sName, const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc);
  ~xiiVisualScriptFunctionProperty();

  virtual xiiFunctionType::Enum GetFunctionType() const override { return xiiFunctionType::Member; }
  virtual const xiiRTTI* GetReturnType() const override { return nullptr; }
  virtual xiiBitflags<xiiPropertyFlags> GetReturnFlags() const override { return xiiPropertyFlags::Void; }
  virtual xiiUInt32 GetArgumentCount() const override { return 0; }
  virtual const xiiRTTI* GetArgumentType(xiiUInt32 uiParamIndex) const override { return nullptr; }
  virtual xiiBitflags<xiiPropertyFlags> GetArgumentFlags(xiiUInt32 uiParamIndex) const override { return xiiPropertyFlags::Void; }

  virtual void Execute(void* pInstance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& out_returnValue) const override;

private:
  xiiSharedPtr<const xiiVisualScriptGraphDescription> m_pDesc;
  mutable xiiVisualScriptDataStorage m_LocalDataStorage;
};

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptMessageHandler : public xiiScriptMessageHandler
{
public:
  xiiVisualScriptMessageHandler(const xiiScriptMessageDesc& desc, const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc);
  ~xiiVisualScriptMessageHandler();

  static void Dispatch(xiiAbstractMessageHandler* pSelf, void* pInstance, xiiMessage& ref_msg);

private:
  xiiSharedPtr<const xiiVisualScriptGraphDescription> m_pDesc;
  mutable xiiVisualScriptDataStorage m_LocalDataStorage;
};
