// ──────────────────────────────────────────────────────────────
// Architecture and API contract
// ──────────────────────────────────────────────────────────────
//
// Four layers between raw UART bytes and north-facing state:
//
//   UartRxBuffer       Byte framing. Emits one line per \n, stripped
//                      of the trailing newline. No protocol knowledge.
//
//   SpaNetParser       Stateless. Classifies a single line and parses
//                      one register line into its label and field list.
//                      Holds no state between calls.
//
//   RfRegisterStore    Stateful raw accumulator. Holds the latest known
//                      raw fields for every register label seen so far.
//                      Updated one parsed register line at a time.
//
//   SpaNetState        North-facing API. Recomputes normalized state from
//                      the register store and hides controller register
//                      layout details from consumers.
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
//   2. For kStateUpdate lines, call parse_register_line() and feed the
//      parsed (label, fields) into RfRegisterStore::update_fields().
//      No multi-line assembly step is needed.
//
//   3. Route kAck lines to command-ack handling, not to the store.
//
//   4. Recompute SpaNetState from the store when either:
//      - UartRxBuffer is empty, or
//      - no new UART data has arrived for >= 100 ms.
//
//      This provides stable state snapshots while preserving partial-update
//      behavior in the raw store.
