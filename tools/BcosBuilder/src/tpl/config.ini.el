; ============================================================================
; FISCO-BCOS Ethereum L1 EL-mode node configuration (config.ini)
; Run as a pure Ethereum execution-layer client: [ethereum] mode=el makes the
; node download blocks from RLPx bootnodes, verify them with the Ethereum
; block verifier (executor v2) and commit them locally. No FISCO gateway /
; PBFT / txpool network is started.
;
; Usage:
;   fisco-bcos -c config.ini -g config.genesis [--el] [--bootnodes bootnodes.json]
; ============================================================================

[service]
    ; AIR nodes run in-process (no tars framework needed). Keep without_tars_framework
    ; commented so the tars proxy file is not required.
    ; without_tars_framework = true
    ; tars_proxy_conf = conf/tars_proxy.ini
    rpc=chain0
    gateway=chain0

[ethereum]
    ; EL mode: none (default) = normal FISCO node; el = Ethereum L1 execution layer
    mode=el
    ; RLPx listen address/port (the Ethereum devp2p protocol, NOT the FISCO gateway)
    listen_ip=0.0.0.0
    listen_port=30303
    ; geth-style bootnodes file (a JSON array of enode:// strings); see bootnodes.json
    bootnodes_file=./bootnodes.json
    ; secp256k1 node identity (hex or PEM). Empty = derive deterministically.
    ; A stable key is strongly recommended so bootnodes can authenticate us.
    node_key_file=
    ; max blocks requested per batch
    max_batch_size=192

[chain]
    ; use SM crypto or not — EL mode is always plain secp256k1/keccak
    sm_crypto=false
    ; the group id (kept for FISCO compatibility; unused by the Ethereum stack)
    group_id=group0
    ; the chain id — must be the numeric Ethereum chain id (11155111 for Sepolia)
    chain_id=11155111

[web3]
    ; Ethereum chain id used by web3 RPC (decimal string)
    chain_id=11155111

[security]
    private_key_path=conf/node.pem
    enable_hsm=false

; FISCO gateway config — present only because the core node still builds a gateway
; object; in EL mode it is NEVER started (AirNodeInitializer skips m_gateway->start()).
; nodes.json must be an empty list so no FISCO peer is contacted.
[p2p]
    listen_ip=0.0.0.0
    listen_port=30300
    sm_ssl=false
    nodes_path=./
    nodes_file=nodes.json

[cert]
    ; directory the certificates located in
    ca_path=./conf
    ca_cert=ca.crt
    node_key=ssl.key
    node_cert=ssl.crt

[storage]
    data_path=data
    enable_cache=true
    type=RocksDB
    key_page_size=10240

[log]
    enable=true
    log_path=./log
    level=info
    max_log_file_size=200

[thread_pool]
    ; Shared IOServicePool thread count
    ;io_thread_count=
