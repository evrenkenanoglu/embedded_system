# OTA project — reorganized structure

Every file from the original project is preserved below with its content unchanged.
Nothing was deleted or rewritten — files were grouped by **what they're for** (docs vs.
diagrams) and, within diagrams, by **level of abstraction** (concept → server/client
detail → real code → single end-to-end view → temporal sequence → hardware layer).
The old flat/scattered layout mixed all six levels together under similar names, which
is what made it feel duplicated even though most files are not byte-for-byte identical.

## Folder guide

| Folder                                 | What's in it                                                                                                                                                                                       | Old location                                                |
| -------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------- |
| `00_Documentation/`                    | Prose docs: system architecture, advanced application-level architecture, terminology glossary                                                                                                     | project root                                                |
| `01_Diagrams_Overview/`                | Six simplified, concept-level `.mmd` flowcharts (light theme) — good for explaining *why* each stage exists                                                                                        | `Diagram_Again/`                                            |
| `02_Diagrams_Detailed/`                | Six `.mmd` flowcharts with real field names (HSVN, canary %, mbedTLS calls) but not full code paths (dark theme)                                                                                   | root-level `01--…mmd` … `06--…mmd`                          |
| `03_Diagrams_Implementation/`          | Three `.mmd` flowcharts that mirror actual source (function names like `esp_delta_ota_feed_patch`, `vTaskDelete`, exact header names)                                                              | `Code_Diagram/`                                             |
| `04_Diagrams_Conceptual_Consolidated/` | Two `.mmd` files that draw the *entire* pipeline as one diagram instead of splitting it into six                                                                                                   | `Security/`                                                 |
| `05_Diagrams_Sequence/`                | One `.mmd` sequence diagram (actor-based, time-ordered) covering the same flow                                                                                                                     | root-level `OTA_Dynamic_Dual_Phase_Signed_OTA_Sequence.mmd` |
| `06_Diagrams_Hardware_Root_of_Trust/`  | One `.mmd` diagram of the silicon-level secure boot chain (eFuses, Secure Boot V2, AES flash encryption) — this is the one diagram with genuinely unique subject matter, not covered anywhere else | root-level `OTA_Crypto_Chain_of_Trust.mmd`                  |

So instead of one folder with 6 diagrams (`Diagram_Again`) and another six loose files
at the root covering the *same six topics* at a different detail level, plus a third
`Code_Diagram` folder covering three of those same topics again at a *third* detail
level, plus a `Security` folder covering the *whole thing* in one or two mega-diagrams
— you now have one place per abstraction level, and within each level the six pipeline
stages are numbered and named consistently (`01_trust_delegation…` through
`06_telemetry_rollback…`).

## One genuine duplicate found (flagged, not silently fixed)

`02_Diagrams_Detailed/05_client_pki_chain_verification__DUPLICATE_OF_03_SEE_README.mmd`
(originally `05--Diagram--Client-Side PKI Chain Verification & Signature Check.mmd`)

Its **title** promises a certificate-chain / ECDSA signature-check diagram. Its
**content**, however, is byte-for-byte identical to
`02_Diagrams_Detailed/03_handshake_canary_hsvn_decision.mmd` (the handshake/canary/HSVN
decision tree) — down to every node ID and class assignment. This looks like a
copy-paste or save error in the original project rather than an intentional duplicate.

I did not invent replacement content for it, since I have no source for what the real
"Client-Side PKI Chain Verification" diagram at this detail level was supposed to
contain — that would be fabricating information rather than removing redundancy. I kept
the file exactly as it existed (nothing lost) and renamed it so the mismatch is visible
at a glance. The diagram you actually want for that topic already exists one level down,
in more depth, at `03_Diagrams_Implementation/esp32_client_state_machine.mmd`
(Phase 3: Cryptographic Integrity Verification) — you may want to regenerate a
standalone version of that section, or just point to it directly.

## Two "Security" files — kept both, not merged

`Conceptual_Diagram copy.mmd` → `conceptual_end_to_end_v1_full_6stage.mmd` (all 6 stages)
`Conceptual_Diagram.mmd` → `conceptual_end_to_end_v2_draft_3stage.mmd` (stages 1–3 only)

These aren't identical: the 3-stage version models the certificate/signature payload
as a single `Payload` node with dashed inbound edges from the signing stage, which the
6-stage version doesn't do. Since the wiring differs, not just the scope, both are kept
as distinct artifacts (v2 reads as an earlier draft of v1's first half) rather than
discarding one.

## Nothing else changed

Every other file's Mermaid/Markdown content is copied verbatim — only the filename
casing/spacing was normalized for consistency (e.g. `03--Diagram--Dynamic Handshake,
Canary, and HSVN Decision Flow.mmd` → `03_handshake_canary_hsvn_decision.mmd`) and files
were sorted into the folders above.
