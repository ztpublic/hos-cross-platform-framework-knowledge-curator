# tauri-harmony

[![Toolchain image](https://github.com/LeenHawk/tauri-harmony/actions/workflows/image.yml/badge.svg)](https://github.com/LeenHawk/tauri-harmony/actions/workflows/image.yml)

A shared **Linux AMD64** build image for experimental Tauri **OpenHarmony / HarmonyOS NEXT** applications.
This repository builds the toolchain only. Each application builds and tests in its own repository.

```sh
docker pull ghcr.io/leenhawk/tauri-harmony:latest
```

## What's included

| Component | Version / location |
| --- | --- |
| Rust | 1.98, with ARM64 and x86_64 OHOS targets |
| Host Node / pnpm | Node 24 / pnpm 11.22.0 |
| HarmonyOS command-line tools / SDK | 6.0.0.858, API 20, `/opt/harmony/command-line-tools` |
| Tauri CLI and source | Experimental `feat/open-harmony` revisions in [tauri-pins.json](tauri-pins.json) |
| ohrs | 1.5.0 |
| Experimental dependency source | `/opt/ohos-tauri/{tauri,wry,tao,cargo-mobile2,ability}` |
| Native build utilities | CMake, Ninja, Clang, libclang, OpenSSL headers, Java, Python, json5, zip |

This is not an official Tauri or Huawei image. Stable Tauri does not yet provide this
experimental integration. Applications must use compatible experimental dependencies
and adapt unsupported plugins themselves. The image contains no application secrets,
signing profiles, or device certificates.

## Daily publication

The image workflow runs at **19:23 UTC daily (03:23 China Standard Time the following day)**,
on toolchain changes to `main`, and on manual dispatch. GitHub scheduled jobs can be delayed.

1. Build a candidate using layer caches and refreshed base image tags.
2. Pull that exact digest into a separate job; check the tools and SDK runtime libraries,
   and compile/link small Rust and C++ programs for ARM64 and x86_64 OHOS.
3. Only after those checks pass, promote the digest to `latest` and `YYYY-MM-DD` (UTC).
4. Check that the registry manifest is available without login.

`build-<run-id>-<attempt>` identifies an individual candidate; check its workflow result
before using it. `latest` and date tags can move. For reproducible consumers, pin the
**digest** shown in the successful workflow summary. The source revision is recorded in
OCI image labels. A failed build or toolchain check does not replace `latest`.

Experimental source commits and SDK archive checksums are pinned. The daily job does
**not** silently follow upstream branches. Update [tauri-pins.json](tauri-pins.json) to
advance the experimental stack together, then let this workflow validate it.
The pinned Tauri runtime is 2.11.6. Rust Ability and its ArkTS HAR use the same
fork source; cargo-mobile2 respects the explicitly configured SDK root.

## Use from another repository

```yaml
jobs:
  harmony:
    runs-on: ubuntu-latest
    container:
      image: ghcr.io/leenhawk/tauri-harmony:latest # Prefer @sha256:... in releases.
    defaults:
      run:
        shell: bash
    steps:
      - uses: actions/checkout@v7.0.1
      - run: git config --global --add safe.directory "$PWD"
      - run: pnpm install --frozen-lockfile
      # Apply your application's experimental Tauri dependency overlay here.
      # Build the frontend with host Node 24 before selecting the SDK's Node.
      - run: pnpm run build:frontend
      - run: |
          export TARGET_TRIPLE=aarch64-unknown-linux-ohos
          source /opt/tauri-harmony/env.sh
          export PATH="$HARMONY_TOOLS_DIR/command-line-tools/tool/node/bin:$PATH"
          cd src-tauri
          cargo tauri ohos build --ci --target aarch64
```

Adapt the frontend command, Tauri directory, and generated HAP configuration to your
application. Initialize its OHOS project with `cargo tauri ohos init` first if necessary.
`ohos build` compiles Rust once for the requested target and profile; the generated
Hvigor callback skips native compilation during packaging. `ohos dev` retains its
rebuild callback and passes the selected target/profile. Application-owned templates
that omit the callback remain compatible. The CLI loads `tauri.ohos.conf.json`
automatically and does not synchronize versions into `AppScope/app.json5`.

Host Node 24 stays the default for frontend tools. Hvigor may require the SDK's bundled
Node; select it only for HAP commands, as shown above. Use `pnpm` at the version your
application requires. `env.sh` supports `aarch64-unknown-linux-ohos` and
`x86_64-unknown-linux-ohos`, setting target-specific C/C++ compilers, linker and CMake
variables without replacing the compiler for host build scripts.

These checks prove tool availability and cross-linking, not device execution, HAP
signing, or application compatibility. Sign and test your application on its target
HarmonyOS/OpenHarmony device separately.

## Public access

The repository and image are public; consumers need no registry credentials.
`org.opencontainers.image.source` links the package to this repository, and Actions
publishes using its own `GITHUB_TOKEN`. The publication job checks anonymous access.

When reusing this workflow under another account, if that check reports a private
package, open its package settings and change visibility to **Public**, then rerun
the failed job. This repository's [package settings](https://github.com/users/LeenHawk/packages/container/tauri-harmony/settings)
are available to its owner.

## Sources and licensing

Repository-authored scripts are MIT licensed. Bundled third-party software retains its
own licenses and terms; the repository license does not relicense the SDK or its tools.
The SDK is fetched from the [6.0.0.858 archive mirror](https://github.com/ErBWs/ohos-sdk/releases/tag/6.0.0.858),
with both archive part checksums verified in [install-sdk.sh](scripts/install-sdk.sh).
Tauri and its dependencies are fetched from the repositories and commits listed in
[tauri-pins.json](tauri-pins.json).

## CLI source identity

`cargo-tauri` is compiled from the exact `tauri` repository/revision in
[tauri-pins.json](tauri-pins.json), using the native TLS backend. The CLI package
version and the Rust `tauri` runtime version are separate version numbers.
The image records both, the source revision, and dependency pins in
`/opt/ohos-tauri/cargo-tauri-source.json`; inspect it alongside `cargo tauri --version`.
The OCI revision label identifies this image repository, not the Tauri source commit.

Keep application version constraints and CLI mismatch checks enabled. gproxy and
TauriTavern can use this common runtime baseline while owning their application
metadata, version injection and signing configuration independently.
