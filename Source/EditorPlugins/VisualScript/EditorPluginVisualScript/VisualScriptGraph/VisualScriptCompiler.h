#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>

class xiiVisualScriptCompiler
{
public:
  xiiVisualScriptCompiler();
  ~xiiVisualScriptCompiler();

  xiiResult AddFunction(xiiStringView sName, xiiVisualScriptNodeDescription::Type::Enum type, const xiiDocumentObject* pEntryObject);

  xiiResult Compile(xiiStringView sDebugAstOutputPath = xiiStringView());

  struct CompiledFunction
  {
    xiiString m_sName;
    xiiEnum<xiiVisualScriptNodeDescription::Type> m_Type;
    xiiDynamicArray<xiiVisualScriptNodeDescription> m_NodeDescriptions;
  };

  struct CompiledModule
  {
    CompiledModule();

    xiiResult Serialize(xiiStreamWriter& inout_stream, xiiStringView sBaseClassName, xiiStringView sScriptClassName) const;

    xiiHybridArray<CompiledFunction, 16> m_Functions;

    xiiVisualScriptDataDescription m_VariableDataDesc;
    xiiVisualScriptDataDescription m_ConstantDataDesc;
    xiiVisualScriptDataStorage m_ConstantDataStorage;
  };

  const CompiledModule& GetCompiledModule() const { return m_Module; }

private:
  struct AstNode;

  struct DataInput
  {
    XII_DECLARE_POD_TYPE();

    AstNode* m_pSourceNode = nullptr;
    xiiUInt32 m_uiId = 0;
    xiiUInt8 m_uiSourcePinIndex = 0;
    xiiUInt8 m_uiTargetPinIndex = 0;
    xiiEnum<xiiVisualScriptDataType> m_DataType;
    xiiUInt8 m_uiArrayIndex = 0;
  };

  struct DataOutput
  {
    xiiSmallArray<AstNode*, 3> m_TargetNodes;
    xiiUInt32 m_uiId = 0;
    xiiUInt8 m_uiSourcePinIndex = 0;
    xiiEnum<xiiVisualScriptDataType> m_DataType;
  };

  struct AstNode
  {
    xiiEnum<xiiVisualScriptNodeDescription::Type> m_Type;
    xiiEnum<xiiVisualScriptDataType> m_DeductedDataType;
    bool m_bImplicitExecution = false;
    const xiiDocumentObject* m_pObject = nullptr;
    xiiSmallArray<AstNode*, 8> m_Next;
    xiiSmallArray<DataInput, 4> m_Inputs;
    xiiSmallArray<DataOutput, 4> m_Outputs;
  };

  XII_ALWAYS_INLINE static xiiStringView GetNiceTypeName(const xiiDocumentObject* pObject)
  {
    return xiiVisualScriptNodeManager::GetNiceTypeName(pObject);
  }

  XII_ALWAYS_INLINE xiiVisualScriptDataType::Enum GetDeductedType(const xiiDocumentObject* pObject) const
  {
    return m_pManager->GetDeductedType(pObject);
  }

  xiiUInt32 GetPinId(const xiiVisualScriptPin* pPin);
  DataOutput& GetDataOutput(const DataInput& dataInput);

  AstNode* BuildAST(const xiiDocumentObject* pEntryNode);
  xiiResult InsertMakeArrayForDynamicPin(AstNode* pNode, const xiiVisualScriptNodeRegistry::PinDesc& pinDesc, xiiPin::Type pinType);
  xiiResult InsertTypeConversions(AstNode* pEntryAstNode);
  xiiResult BuildDataStack(AstNode* pEntryAstNode, xiiDynamicArray<AstNode*>& out_Stack);
  xiiResult BuildDataExecutions(AstNode* pEntryAstNode);
  xiiResult FillDataOutputConnections(AstNode* pEntryAstNode);
  xiiResult CollectData(AstNode* pEntryAstNode);
  xiiResult BuildNodeDescriptions(AstNode* pEntryAstNode, xiiDynamicArray<xiiVisualScriptNodeDescription>& out_NodeDescriptions);

  struct ConnectionType
  {
    enum Enum
    {
      Execution = XII_BIT(0),
      Data = XII_BIT(1),

      All = Execution | Data,
    };
  };

  struct Connection
  {
    AstNode* m_pPrev = nullptr;
    AstNode* m_pCurrent = nullptr;
    ConnectionType::Enum m_Type = ConnectionType::Execution;
    xiiUInt32 m_uiPrevPinIndex = 0;
  };

  struct ConnectionHasher
  {
    static xiiUInt32 Hash(const Connection& c);
    static bool Equal(const Connection& a, const Connection& b);
  };

  enum class VisitorResult
  {
    Continue,
    Stop,
    Error,
  };

  using AstNodeVisitorFunc = xiiDelegate<VisitorResult(const Connection& connection)>;
  xiiResult TraverseAst(AstNode* pEntryAstNode, xiiUInt32 uiConnectionTypes, AstNodeVisitorFunc func);

  xiiResult FinalizeDataOffsets();
  xiiResult FinalizeConstantData();

  void DumpAST(AstNode* pEntryAstNode, xiiStringView sOutputPath, xiiStringView sFunctionName, xiiStringView sSuffix);
  void DumpGraph(xiiArrayPtr<const xiiVisualScriptNodeDescription> nodeDescriptions, xiiStringView sOutputPath, xiiStringView sFunctionName, xiiStringView sSuffix);

  const xiiVisualScriptNodeManager* m_pManager = nullptr;

  xiiDeque<AstNode> m_AstNodes;
  xiiHashTable<const xiiDocumentObject*, AstNode*> m_ObjectToAstNode;
  xiiHybridArray<AstNode*, 8> m_EntryAstNodes;

  xiiHashSet<Connection, ConnectionHasher> m_ReportedConnections;

  xiiHashTable<const xiiVisualScriptPin*, xiiUInt32> m_PinToId;
  xiiUInt32 m_uiNextPinId = 0;

  using DataOffset = xiiVisualScriptNodeDescription::DataOffset;

  struct DataDesc
  {
    XII_DECLARE_POD_TYPE();

    DataOffset m_DataOffset;
    xiiUInt32 m_uiUsageCounter = 0;
  };

  xiiHashTable<xiiUInt32, DataDesc> m_PinIdToDataDesc;

  xiiHashTable<xiiVariant, xiiUInt32> m_ConstantDataToIndex;

  CompiledModule m_Module;
};
