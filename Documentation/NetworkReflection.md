# Reflection networking

Foundation layers typed messaging and reflected RPC over [transport sessions](NetworkTransportSessions.md). `xiiNetworkTypeRegistry` discovers value schemas from XII RTTI. `xiiNetworkSerializer` operates independently of a host and can decode into an existing application value. `xiiNetworkReflectionEndpoint` adds session agreement, explicit incoming handlers, and asynchronous RPC. The same Foundation code runs on the Windows, Linux, and macOS session backends.

## Define and serialize a value

Use XII's regular reflection declarations and attach `xiiNetworkAttribute` to the properties to transmit. A type with at least one such property is discovered automatically, including inherited properties. The following example is compiled as part of [NetworkSerializerTest.cpp](../Source/UnitTests/FoundationTest/Network/NetworkSerializerTest.cpp).

```cpp
struct NetworkExampleNested
{
  xiiInt32  m_iNumber = 0;
  xiiString m_sLocal  = "constructor";
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkExampleNested);

XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkExampleNested, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkExampleNested>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Number", m_iNumber)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Local", m_sLocal),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
```

```cpp
xiiNetworkDataError RoundTrip(xiiNetworkTypeRegistry& registry,
                            const NetworkExampleNested& source, NetworkExampleNested& target)
{
  xiiNetworkSerializationContext context;
  xiiArrayPtr<const xiiUInt8> bytes;
  auto error = xiiNetworkSerializer::Serialize(registry, source, context, bytes);
  if (error.Failed())
    return error;
  return xiiNetworkSerializer::Deserialize(registry, target, bytes, context);
}
```

`m_sLocal` retains its existing value, including a constructor default on a newly allocated message. The output span borrows the context until its next serialization or destruction. Use a separate context for each concurrent operation. Both operations return `xiiNetworkDataError`. Inspect its code, type ID, field ID, offset, and any associated transport error where available. A failed decode leaves an existing destination unchanged.

Supported values include booleans, integers, reflected enums and flags, finite floating-point values, UTF-8 strings, byte buffers, vectors, quaternions, matrices, transforms, colors, angles, `xiiTime`, `xiiTimestamp`, UUIDs, and bounded variants. Reflected arrays and string-keyed maps support the same value types, including nested network structures. Dynamic variants contain a closed set of value alternatives, including variant arrays and dictionaries. Native pointers, arbitrary polymorphic variant objects, and borrowed strings are rejected. Decoding does not intern untrusted strings. Writable reflected properties must implement ordinary value semantics. A setter must accept every value admitted by its field policy.

Use `xiiNetworkAttribute` to set field size limits, an optional field ID or codec ID, and quantization. Scalar and vector quantization maps the declared finite range to the nearest of `2^bits` levels. Before rounding to the destination floating-point type, its maximum absolute rounding error is `(maximum - minimum) / (2 * (2^bits - 1))`. Quantized quaternions require 8–24 bits and the range `[-1, 1]`. The encoder validates unit length and the decoder checks the quantization tolerance before normalizing. Quantization is part of wire compatibility.

`RegisterCodec` installs a trusted, owned `xiiNetworkCustomCodec` with a stable wire ID, version, and expected RTTI. Its decoder must reserve allocation budget before allocating and return the declared value type. The engine supplies a bounded reader and checks full consumption and output type. Application code inside a codec, native constructor, or setter is responsible for its own allocations and side effects.

## Wire format and evolution

Fixed-width wire integers use big-endian byte order. The value format uses canonical unsigned LEB128 with zigzag for signed integers, explicit component encoding for math values, and length-prefixed UTF-8 strings and byte buffers. Map keys are ordered by byte value. Native object layouts and addresses never appear in the format.

A root value starts with `XNS1`, one byte for encoding, the 64-bit type ID, the 32-bit RTTI version, and the 64-bit schema fingerprint. Tagged objects contain ascending field IDs followed by a 64-bit field signature, a 32-bit value length, and the value. A zero field ID ends an object. Unknown fields can be skipped. Known fields must match their wire signatures and required fields must be present. Optional fields missing from input preserve the destination value. Compact objects write values in schema field order and require exact recursive schema agreement when used on a session.

Default type and field IDs use FNV-1a over the exact reflected UTF-8 name. IDs are stable across platforms and declaration reordering. Preserve an explicit `xiiNetworkIdentityAttribute` ID when renaming a type and an explicit field ID when renaming a field. ID reuse for a different field meaning breaks compatibility. Tagged compatibility is directional. A receiver can add an optional field and ignore a removed field, but it rejects a missing required field, changed wire shape, or a sender policy that exceeds a receiver's limits. Nested descriptions obey the same rules. Type versions participate in compact agreement. A version increment alone does not establish compatibility.

`EnableProtocol` reserves session feature bit 63. Typed control frames use `XNR1` and a one-byte frame kind on ordinary session channels. Offers and acknowledgments carry bounded complete recursive descriptors on a reliable channel. They permit tagged transmission when the receiver can decode the sender's schema. An exact descriptor enables compact transmission. `GetSchemaAgreement` reports whether the current local schema may be sent, and whether compact is available. The receiver reports schema rejection through `ProcessEvent`, and the sender reports `NotNegotiated` until it receives a compatible acknowledgment. Cache entries are scoped to the session. Reconnects and plugin changes require a fresh agreement.

## Typed messages

After configuring both hosts with `EnableProtocol`, start the hosts and endpoints. Route `Connected`, `Message`, `ChannelClosed`, and `Closed` events in host publication order through `ProcessEvent`. On a host carrying other application messages, reserve dedicated channels for reflection traffic and route message events accordingly. Continue draining host events, including `MessageSent`. The optional submission handle and user data from `SendMessage` let the application identify its transport completion. Transport completion records release the payload and do not acknowledge handler execution.

Derive from `xiiNetworkTypedMessageHandler<T>`, implement `Authorize` and `OnMessage`, and register an owned handler through `RegisterMessageHandler`. `Authorize` receives the original immutable session record and channel, including the session's authenticated identity and channel identity. It runs before constructing the value. Once the sender has called `OfferSchema` and processed the acknowledgment, use `SendMessage(channel, value, encoding)`. On the receiver, `Dispatch(executor)` runs callbacks bound to that executor. The message reference is borrowed for the callback only. The queued host lease remains owned by the endpoint through dispatch. A removed handler's queued work is rejected and an active callback retains its owner until it returns. Choose the delivery mode when opening the session channel. QUIC unreliable channels may carry typed messages after a reliable schema exchange for that session.

## Reflected RPC

The [loopback example](../Source/UnitTests/FoundationTest/Network/NetworkReflectionEndpointTest.cpp) builds and runs on the Foundation test target. `NetworkRpcExample` declares `Add`, `Echo`, `Nested`, `Timestamp`, `Notify` and `Denied` using `XII_FUNCTION_PROPERTY(...)->AddAttributes(new xiiNetworkRpcAttribute(id))`. The unannotated `Hidden` function cannot be called. `ExampleService` owns its reflected target and checks the original session, service, and method before authorizing a call. Production policies should select the permitted authenticated identity, session and service or method from `xiiNetworkDispatchContext`.

The caller uses `DeclareRemoteService` with the local reflected signature. The receiver uses `RegisterService` with its owned target and an executor ID. Both peers use the same service ID and method IDs. Request frames carry the service, method, correlation ID, deadline budget, invocation flags, a bounded method descriptor, and encoded arguments. The descriptor includes argument and return schemas. The receiver checks it before decoding arguments or invoking the function. Return values use typed serialization and errors expose only a protocol code. Unsupported signatures such as native pointers, mutable references, output arguments, and constructors fail registration. Borrowed typed pointers may provide **local** reflected structure inputs to `CallRpc`. The call encodes them synchronously. Timestamps can use an `xiiTimestamp` typed pointer or signed microseconds in the variant API.

```cpp
xiiNetworkDataError SubmitAdd(xiiNetworkReflectionEndpoint& caller, xiiNetworkChannel channel,
                              xiiNetworkRpcRequest& request)
{
  const xiiVariant arguments[] = {xiiInt32(19), xiiInt32(23)};
  return caller.CallRpc(channel, 7, 1, arguments, request);
}

xiiNetworkDataError ReadAddResult(xiiNetworkReflectionEndpoint& caller,
                                 const xiiNetworkRpcResult& result, xiiInt32& sum)
{
  return caller.DecodeRpcResult(result, sum);
}
```

Pump `TryGetRpcResult` to retrieve correlation handles and encoded results, and `Update` to expire local deadlines. A successful correlated admission reserves one completion slot until its result is consumed. Responses, timeouts, cancellation, and disconnection compete for that slot. `CancelRpc` immediately completes locally and submits a best-effort remote cancellation. Once the receiver starts dispatching an invocation, cancellation cannot stop it or reverse side effects. Reliable channels are required, including for fire-and-forget. A finite per-session replay window accepts out-of-order request IDs within the window and rejects repeated or older IDs. Reconnect never retries calls automatically. Transport reliability does not establish exactly-once execution across sessions.

## Concurrency and maintenance

Call `ProcessEvent` from one event pump in publication order. Dispatch queues preserve admission order within each executor. Drive each executor from one caller-managed thread at a time. Different executors can run concurrently. Callbacks run outside endpoint locks and never on native I/O workers. Use the same executor or application synchronization for targets shared between registrations. Do not mutate the registry or unload plugins inside a callback. Keep the host and registry alive until endpoint destruction and quiesce endpoint callers before shutdown. Queued work is discarded on shutdown and admitted RPC calls complete with `Disconnected`.

Cached metadata uses registry read permits. Plugin changes drain readers before unloading code, invalidate handler and method registrations and schema agreements, and rebuild the registry. Re-register handlers and services and offer schemas again after the change. Portable schema snapshots can survive unload. Keep any caller-owned plugin handler, service, or decoded plugin value within the lifetime of its module. For manual RTTI changes, invalidate the registry before editing metadata and refresh after publication.

Apply endpoint limits to peers, registrations, schema cache bytes, queued packet bytes, pending and completed calls, and replay windows. RPC admission applies per-peer and per-service rate and concurrent request limits. Registry and endpoint statistics expose schema build time, serialization operations and failures, byte counts, pending work, rejected packets, authorization failures, and aggregate dispatch and execution time. Buffer limits apply before variable-size decoding. For large hot-path collections measure both runtime and allocation cost, and reuse serialization contexts where possible.

## Focused validation and measurements

Build `FoundationTest` with the `linux-clang-debug` preset and run `FoundationTest -noGui -run -close -noSave -all -filter NetworkReflection`. The suite includes portable serializer and RPC fixtures, tagged evolution, fixed-array and allocation bounds, two deterministic malformed-input corpora, metadata invalidation during dispatch, and loopback messaging and RPC over TCP, TLS, and QUIC. Portable fixtures validate common bytes across builders. Mixed-OS peers require a separate deployment test.

Enable `-networkReflectionBenchmark true` to reproduce the opt-in samples. The following single Linux x86-64 run used Clang 23.1.5 Debug on an Intel Core i7-12700H. Encode timings reuse warmed context storage. The allocation columns count changes reported by the XII default allocator. Accounted decode peak measures the conservative decode budget, which includes staging estimates. These numbers do not measure process-wide allocations or release performance.

| Workload | Tagged / compact bytes | Tagged / compact encode | Tagged / compact decode | Decode allocations/op | Tagged / compact scratch bytes | Accounted decode peak |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Single signed integer | 40 / 26 | 667 / 474 ns | 943 / 670 ns | 0 | 48 / 32 | 402 B |
| Small nested value | 282 / 72 | 4.57 / 1.82 µs | 31.35 / 26.92 µs | 9 | 288 / 80 | 5,436 B |
| Nested value with 256 KiB buffer | 264,336 / 262,334 | 37.03 / 12.94 µs | 777.54 / 735.57 µs | 272.02 | 393,456 / 393,296 | 658,978 B |

All three warmed encoders reported zero allocations per operation. The general reflection binary serializer wrote 78 bytes for the same single-field RTTI value, at 28.45 µs and 11 allocations per encode. Its persistence wire format serves a different purpose. Registry refresh averaged 422 µs for seven valid schemas among the discovered RTTI, and cached lookup averaged 62 ns. The custom integer codec round trip averaged 1.82 µs. Cached typed decode and dispatch averaged 33.86 µs, and cached RPC decode, invoke and response admission averaged 12.33 µs. Sequential TCP loopback RPC averaged 101.1 µs including application polling, excluding connection setup.
