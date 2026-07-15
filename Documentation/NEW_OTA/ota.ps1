# 1. Define the consistent folder structure
$folders = @(
    "ota",
    "ota/diagrams",
    "ota/docs"
)

# 2. Define the consistent, de-duplicated files
$files = @(
    "ota/diagrams/01_trust_delegation_pki_setup.mmd",
    "ota/diagrams/02_server_code_signing_pipeline.mmd",
    "ota/diagrams/03_server_handshake_gateway_decision.mmd",
    "ota/diagrams/04_client_transport_flash_write.mmd",
    "ota/diagrams/05_client_cert_signature_validation.mmd",
    "ota/diagrams/06_telemetry_rollback_loop.mmd",
    "ota/diagrams/07_dynamic_dual_phase_sequence.mmd",
    "ota/diagrams/08_crypto_chain_of_trust.mmd",
    "ota/diagrams/09_conceptual_overview_lifecycle.mmd",
    "ota/docs/01_terminology_guide.md",
    "ota/docs/02_application_level_architecture.md",
    "ota/docs/03_framework_architecture.md"
)

# 3. Create folders and empty files
$folders | ForEach-Object { New-Item -ItemType Directory -Force -Path $_ }
$files | ForEach-Object { New-Item -ItemType File -Force -Path $_ }