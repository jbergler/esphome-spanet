// ──────────────────────────────────────────────────────────────
// Architecture and API contract
// ──────────────────────────────────────────────────────────────
//
// Three layers between raw UART bytes and north-facing state:
//
//   UartRxBuffer       Byte framing. Emits one line per \n, stripped
//                      of the trailing newline. No protocol knowledge.
//
//   SpaNetParser       Stateless. Classifies a single line and parses
//                      one register line into a typed register variant.
//                      Holds no state between calls.
//
//   RegisterStore      Stateful accumulator of latest typed registers.
//                      Updated one parsed register line at a time.
//                      Exposes normalized north-facing State.
//
//                      Example: date/time register fields are normalized
//                      into a single Unix epoch value.
//
// A complete SpaNET RF response spans multiple UART lines — one per
// register (R2, R3 … RG). The store handles this naturally: each parsed line
// updates only the entry for its own label. Registers not yet received remain
// absent from the store and do not block access to registers that have already
// arrived.
//
// ──────────────────────────────────────────────────────────────
// Caller responsibilities
// ──────────────────────────────────────────────────────────────
//
//   1. Call classify_message on each line from UartRxBuffer.
//
//   2. For kStateUpdate lines, call RegisterStore::update(line).
//      No multi-line assembly step is needed.
//
//   3. Route kAck lines to command-ack handling, not to the store.
//
//   4. Publish/consume State snapshots when either:
//      - UartRxBuffer is empty, or
//      - no new UART data has arrived for >= 100 ms.
//
//      This provides stable state snapshots while preserving partial-update
//      behavior in the register store.
