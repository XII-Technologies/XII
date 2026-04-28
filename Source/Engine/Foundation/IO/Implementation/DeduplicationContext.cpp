/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/DeduplicationReadContext.h>
#include <Foundation/IO/DeduplicationWriteContext.h>

XII_IMPLEMENT_SERIALIZATION_CONTEXT(xiiDeduplicationReadContext);

xiiDeduplicationReadContext::xiiDeduplicationReadContext()  = default;
xiiDeduplicationReadContext::~xiiDeduplicationReadContext() = default;

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_SERIALIZATION_CONTEXT(xiiDeduplicationWriteContext);

xiiDeduplicationWriteContext::xiiDeduplicationWriteContext()  = default;
xiiDeduplicationWriteContext::~xiiDeduplicationWriteContext() = default;

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_DeduplicationContext);
