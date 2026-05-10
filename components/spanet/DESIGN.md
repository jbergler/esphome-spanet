// ──────────────────────────────────────────────────────────────
// Architecture and API contract
// ──────────────────────────────────────────────────────────────
//
// Three layers between raw UART bytes and typed state:
//
//   UartRxBuffer       Byte framing. Emits one line per \n, stripped
//                      of the trailing newline. No protocol knowledge.
//
//   SpaNetParser       Stateless. Classifies a single line and parses
//                      one register line into its label and field list.
//                      Holds no state between calls.
//
//   RfRegisterStore    Stateful accumulator. Holds the latest known
//                      fields for every register label seen so far.
//                      Updated one line at a time. Exposes typed
//                      accessors that read from its accumulated data.
//
// A complete SpaNET RF response spans multiple UART lines — one per
// register (R2, R3 … RG). The store handles this naturally: each line
// updates only the entry for its own label. Registers not yet received
// remain absent from the store and do not block access to registers
// that have already arrived.
//
// ──────────────────────────────────────────────────────────────
// Caller responsibilities
// ──────────────────────────────────────────────────────────────
//
//   1. Call classify_message on each line from UartRxBuffer.
//
//   2. Feed kStateUpdate lines into RfRegisterStore::update().
//      No multi-line assembly step is needed.
//
//   3. Route kAck lines to command-ack handling, not to the store.
//
//   4. Read typed state from the store when needed (e.g. in update()).
//      The store always reflects the most recently seen value for each
//      register, regardless of whether the full RF response is complete.
