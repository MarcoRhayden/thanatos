<a id="readme-top"></a>

<p align="center">
  <img src="docs/img/thanatos-banner.png" alt="Thanatos Banner" width="240" style="border-radius:16px; box-shadow:0 8px 28px rgba(0,0,0,.35)"/>
</p>

<h1 align="center">Thanatos <small style="font-weight:400"></small></h1>


<p align="center">
  <em><strong>Thanatos</strong> is a user-space <u>Ragnarok Online protocol terminator / emulator</u>.<br/>
  Inspired by the Perl “Poseidon”, now re-engineered in modern <b>C++20</b> with an async, testable core.</em>
</p>

<p align="center">
  <sub>
    ユーザ空間で動作する RO プロトコル終端エミュレータ。Perl 製「Poseidon」を継承し、C++20 で再設計・再実装。
  </sub>
</p>


<p align="center">
  <a href="#-features"><img src="https://img.shields.io/badge/Engine-C%2B%2B20-5b8def?logo=c%2B%2B&labelColor=1b1f24"></a>
  <a href="#-quickstart"><img src="https://img.shields.io/badge/Quickstart-3min-ff77aa?labelColor=1b1f24"></a>
  <a href="#-architecture"><img src="https://img.shields.io/badge/Architecture-Clean-8b949e?labelColor=1b1f24"></a>
  <a href="#-license"><img src="https://img.shields.io/badge/License-MIT-ffd400?labelColor=1b1f24"></a>
</p>

<p align="center">
  <img src="docs/img/openkore.png" height="90" alt="OpenKore"/>
  &nbsp;&nbsp;&nbsp;
  <img src="docs/img/arkansoftware.png" height="90" alt="Arkan Software"/>
</p>

<p align="center" class="muted">
  <strong>Thanatos</strong> sits in front of the official <em>Ragnarok Online</em> client,
  terminating the <span class="sc">Login / Char / Map</span> handshake in user space.
  The official client connects to it in place of the live <code>login/char/map</code> servers.
  It services anti-cheat liveness (e.g., <abbr title="nProtect">GameGuard</abbr> / HackShield) locally,
  capturing the client’s genuine <em>challenge/response</em> and exposing a compact
  <span class="chip">Query&nbsp;Server</span> endpoint to <strong>OpenKore</strong>.
  In practice, <strong>Thanatos</strong> produces the exact artifacts the server expects,
  and OpenKore uses them to answer the official backend faithfully.
</p>

> ⚠️ **For Research/Education Only / 研究・教育目的のみ** — Do not use on third-party servers or in commercial environments. / 第三者サーバーや商用環境では使用しないでください。


<p align="center">
  <img src="docs/img/thanatos.png" alt="Thanatos" height="260" style="border-radius:12px; box-shadow:0 6px 22px rgba(0,0,0,.35)"/>
</p>

<p align="center">
  <img src="docs/badges/protocol-terminator.svg" alt="Protocol Terminator" height="22">&nbsp;
  <img src="docs/badges/gameguard-heartbeat.svg" alt="GameGuard Heartbeat" height="22">&nbsp;
  <img src="docs/badges/openkore-query-server.svg" alt="OpenKore Query Server" height="22">&nbsp;
  <img src="docs/badges/pluggable-seed-checksum.svg" alt="Pluggable Seed/Checksum" height="22">
</p>

## コンセプト / Concept

> 「光と闇、刃とコード。Thanatos はクライアントを“だます”ための最小限の世界を描く。」

* 🎮 **GameGuard ハンドシェイク**：seed/nonce、challenge/response、rolling checksum、心拍（heartbeat）。
* 🔌 **プロトコル終端**：Login / Char / Map の各フェーズを最小実装、サイズ／opcode 検証。
* 🤝 **OpenKore ブリッジ（計画）**：正規化した RO ストリームを転送、返信を注入。
* 🧱 **セーフなコーデック**：LE プリミティブ、境界チェック、opcode レジストリ。
* 🧼 **クリーンアーキテクチャ**：`domain → application → infrastructure → interface` の一方向依存。

<br>

<details>
  <summary>
    🟡 <strong>English</strong> — Concept <em>(click to expand)</em>
    &nbsp;&nbsp;
    <img alt="English" src="https://img.shields.io/badge/Open-English-FFC107?style=for-the-badge&logo=readme&logoColor=000" />
  </summary>

- 🎮 **GameGuard handshake & heartbeats**: seed/nonce, challenge/response, rolling checksums, timers.
- 🔌 **Protocol terminator**: minimal Login/Char/Map phases with size/opcode validation.
- 🤝 **OpenKore bridge (planned)**: normalized RO stream → OpenKore, inject replies back.
- 🧱 **Safety‑first codec**: LE primitives, bounds checks, opcode registry.
- 🧼 **Clean Architecture**: single‑direction dependency: `domain → application → infrastructure → interface`.
</details>

---

## 🧭 Quickstart / はじめに

**前提 (Windows):**  
- Windows 10/11 ・ PowerShell  
- Visual Studio 2022（Desktop development with C++）  
- 初回のみ `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` が必要な場合があります。

### 1) 依存関係の準備（最初の一度だけ）
このスクリプトは **vcpkg** を取得し、必要なライブラリをセットアップします。
```powershell
./scripts/setup-vcpkg.ps1
```

### 2) ビルド（Release 推奨）
標準（Release / x86 / 静的リンク）。必要に応じて引数を変更できます。
```powershell
./scripts/build-static.ps1
# 例: 64bit でビルド
./scripts/build-static.ps1 -Arch x64
# 例: Debug ビルド
./scripts/build-static.ps1 -Config Debug
```

### 3) 設定ファイルを編集
`config/thanatos.toml` を開き、**ログ・ポート・スポーン初期値** を調整します。
```toml
[thanatos]
ro_host     = "127.0.0.1"
login_ports = [6900, 6901, 6902]
char_ports  = [6121, 6122, 6123]

[protocol]
max_packet = 4194304    # 4 MiB

[query]
host    = "127.0.0.1"
ports   = [24395, 24396, 24397]
max_buf = 1048576       # 1 MiB
```

> クライアントが公式ドメインに固定されている場合は、ビルドに依存する方法で **アドレス差し替え** の準備が必要です。

### 4) 実行
```powershell
# 環境に合わせてパスを選択してください
./build/win64/Release/Thanatos.exe
# または
./build/win32/Release/Thanatos.exe
```

### 5) 動作確認（簡易チェック）
- クライアントが接続し、**マップに入る**。  
- **2 分以上** 切断されない。  
- コンソールに **ヘルス/ハートビート** のログが定期的に表示される。

<br>

<details>
  <summary>
    🟡 <strong>English</strong> — Quickstart <em>(click to expand)</em>
    &nbsp;&nbsp;
    <img alt="English" src="https://img.shields.io/badge/Open-English-FFC107?style=for-the-badge&logo=readme&logoColor=000" />
  </summary>

**Prerequisites (Windows):**  
- Windows 10/11, PowerShell  
- Visual Studio 2022 (Desktop development with C++)  
- You may need: `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` (first run).

### 1) Setup dependencies (first time only)
Fetches **vcpkg** and installs required libraries.
```powershell
./scripts/setup-vcpkg.ps1
```

### 2) Build (Release recommended)
Default is Release/x86/static linking. Adjust flags as needed.
```powershell
./scripts/build-static.ps1
# Build 64-bit
./scripts/build-static.ps1 -Arch x64
# Debug build
./scripts/build-static.ps1 -Config Debug
```

### 3) Configure
Edit `config/thanatos.toml` (logging, ports, spawn defaults). Start simple (127.0.0.1).  
If your client is hard‑wired to official domains, prepare an **address replacer** (build‑dependent).

### 4) Run
```powershell
./build/win64/Release/Thanatos.exe
# or
./build/win32/Release/Thanatos.exe
```

### 5) Verify
- Client connects and **enters the map**.  
- **> 2 minutes** without disconnect.  
- Console shows periodic **health/heartbeat** logs.
</details>

---

## ✨ Features / 特徴

* **GameGuard**：クライアント側アンチチートの擬似応答で「健全」状態を維持。
* **Phase Machines**：Handshake → Auth → Redirect → Map Enter の順序を厳格に。
* **SessionRegistry**：フェーズ横断の接続追跡、`PhaseSignal` で遷移を可視化。
* **テスト**：gtest + ctest。コーデック、状態機械、境界条件を重視。

<br>

<details>
  <summary>
    🟡 <strong>English</strong> — Features <em>(click to expand)</em>
    &nbsp;&nbsp;
    <img alt="English" src="https://img.shields.io/badge/Open-English-FFC107?style=for-the-badge&logo=readme&logoColor=000" />
  </summary>

- **GameGuard**: client‑side anti‑cheat emulation keeps the client “healthy”.
- **Phase machines**: strict order — Handshake → Auth → Redirect → Map Enter.
- **SessionRegistry** with **PhaseSignal** for cross‑phase lifecycle tracing.
- **Tests**: gtest + ctest with focus on codec, state machines, edge cases.
</details>

---

## 🧱 Architecture / アーキテクチャ

```mermaid
flowchart LR
  subgraph Interface_Ragnarok["Interface · Ragnarok"]
    RS[RagnarokServer]
    LFL[LoginFlow] --> LHD[LoginHandler]
    CFL[CharFlow]  --> CHD[CharHandler]
    PROTO["protocol/Codec · Opcodes · Coords"]
    MODEL["model/PhaseSignal · SpawnTable"]
    DTO["dto/*  mappers/*"]
  end

  subgraph Application["Application"]
    SREG[State · SessionRegistry]
    SVC[Service · GameGuardBridge]
    PORTS["Ports · net/crypto/query"]
  end

  subgraph Infrastructure["Infrastructure"]
    CFG[Config]
    LOG[Logger]
    NET[AsioTcpServer/Client]
    PRES[Presentation · StartupSummary]
  end

  subgraph Shared["Shared"]
    SH[Utils · Hex · Terminal · BuildInfo]
  end

  RS --- LFL
  RS --- CFL
  RS --- PROTO
  RS --- MODEL
  RS --- DTO
  RS --- SREG
  RS --- SVC
  RS --- PORTS

  CFG --> RS
  LOG --> RS
  NET --> RS
  PRES --> RS
  SH --> RS
```

> 依存は内向きのみ：`domain ← application ← infrastructure ← interface`。

---

## 🕹️ Flow / フロー（実際にやること & 内部動作）

### 👣 What you actually do / 手順（ユーザー視点）

1. Configure `thanatos.toml` and launch **Thanatos**.
2. Point the RO client to **Thanatos** (Login/Char/Map). Address replacement is fine.
3. Start the game and **log in with any credentials** (no validation).
4. Enter the initial map (defaults come from SpawnTable/Coords).
5. Once stable, the client’s **heartbeat/health packets** are **mirrored/forwarded to the Query Server** endpoint.



### 🔧 Behind the scenes / 内部で起きていること

- Thanatos minimally implements Login/Char/Map to keep the client **healthy**.
- It **skips real authentication** (accepts any ID/PW) and fast-paths to **enter-map**.
- After entering the map, it maintains **heartbeats/rolling checksums** (GameGuard-style) with timers to avoid disconnects.
- Once stabilized, the **health/heartbeat stream** is **non-invasively duplicated** to the **Query Server** so external tools can subscribe without touching the live game socket.
- The Query Server is **one-way (server → query clients)**, ensuring safe observation.



---

## ⚙️ Requirements / 開発環境

| Item      | Versão / Detalhe                                |
| --------- | ----------------------------------------------- |
| OS        | Windows 10/11（Linux でもビルド可能）                    |
| Toolchain | Visual Studio 2022 / MSVC Build Tools 2022      |
| Build     | CMake ≥ 3.26 · vcpkg (manifest)                 |
| Deps      | `spdlog`, `tomlplusplus`, `boost-asio`, `gtest` |


---

## 🛠️ thanatos.toml — Annotated Example / 注釈付きサンプル

```toml
[app]
service_name = "Thanatos"

# Semantic version of your build
# ビルドのセマンティックバージョン
version      = "0.1.3"

# Verbose checks and extra diagnostics (disable in production)
# 詳細チェックと追加診断（本番では無効に）
debug        = false


[thanatos]
# Host/interface for RO login/char listeners (IPv4/IPv6 accepted)
# RO ログイン／キャラ用のバインド先ホスト（IPv4/IPv6 を受け付け）
ro_host = "127.0.0.1"

# Login ports (ordered). Thanatos tries each set index in lockstep.
# ログイン用ポート（順序付き）。同じインデックスをポートセットとして順に試行。
login_ports = [6900, 6901, 6902]

# Char ports (ordered). Must be same length as login_ports.
# キャラ用ポート（順序付き）。login_ports と同じ長さにする。
char_ports  = [6121, 6122, 6123]


[protocol]
# Maximum accepted packet size (bytes). 4 MiB = 4 * 1024 * 1024
# 受信パケットの最大サイズ（バイト）。4 MiB = 4 * 1024 * 1024
max_packet = 4_194_304


[query]
# Max buffer for query server (bytes). 1 MiB is usually safe.
# クエリサーバーの最大バッファ（バイト）。1 MiB が無難
max_buf = 1_048_576


[net]
# Host/interface for Query BUS listener
# Query BUS 用のバインド先ホスト
query_host = "127.0.0.1"

# Query ports (ordered). Must align with login/char arrays.
# クエリ用ポート（順序付き）。login/char と同じ順序・長さに揃える。
query_ports = [24395, 24396, 24397]

# Max queued writes per socket (protects memory pressure)
# ソケット毎の送信キュー上限（メモリ圧迫の防止）
max_write_queue = 1024

# Disable Nagle to reduce latency
# 遅延削減のため Nagle 無効化
tcp_nodelay = true

# Keep TCP alive to detect dead peers
# 相手切断の検知用に TCP KeepAlive を有効化
tcp_keepalive = true


[log]
# Log level: trace|debug|info|warn|error
# ログレベル：trace|debug|info|warn|error
level = "info"

# Write logs to file (false = console only)
# ファイルへ出力（false の場合はコンソールのみ）
to_file = false

# Log file path (used when to_file = true)
# ログファイルのパス（to_file=true のとき使用）
file = "logs/thanatos.log"

# Keep up to N rotated files
# ローテーションファイルの最大保持数
max_files = 3

# Rotate when file exceeds this size (bytes)
# このサイズ（バイト）を超えたらローテーション
max_size_bytes = 2_097_152
```

---

## ⚡Scripts / スクリプト

```powershell
# vcpkg セットアップ / Setup vcpkg
./scripts/setup-vcpkg.ps1
# ビルド / Build
./scripts/build-static.ps1
# テスト / Tests
./scripts/run-tests.ps1 -Config Release
```
<p align="right">(<a href="#readme-top">back to top</a>)</p>

## 📜 License / ライセンス

MIT — `LICENSE` を参照。

---

<h2 id="thanks">🙌 Thanks / 謝辞</h2>

<img
  src="docs/img/ro_thanatos.webp"
  alt="Thanatos (Ragnarok Online)"
  width="220"
  align="right"
  style="margin:0 0 8px 24px; filter:drop-shadow(0 8px 24px rgba(0,0,0,.35));"
/>

<ul>
  <li><a href="https://github.com/OpenKore/openkore">OpenKore</a> community</li>
</ul>

<div style="clear: both;"></div>



