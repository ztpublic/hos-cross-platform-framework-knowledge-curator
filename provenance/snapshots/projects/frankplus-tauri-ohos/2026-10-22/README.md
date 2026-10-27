# Tauri on OpenHarmony

This project aims to establish OpenHarmony (HarmonyOS Next) as a first-class compilation target for the [Tauri](https://tauri.app) framework.

## Build Instructions

To build the project for OpenHarmony, you need to set up the necessary environment variables and then run the build command.

### Prerequisites

Ensure the following environment variables are set:

*   `OHOS_BASE_SDK_HOME`: The base path to your OpenHarmony SDK.
    *   Example: `/home/francesco/setup-ohos-sdk/linux`
*   `OHOS_CMD_TOOLS` (Optional): Path to the OpenHarmony command line tools if `hvigorw` is not in your system PATH.

### Running the Build

From the root of the project, run:
```bash
cargo tauri ohos build
```

This command will:
1.  **Build the Rust Library**: It compiles the Rust code in `examples/basic-app/src-tauri` using `cargo` for the `aarch64-unknown-linux-ohos` target.
2.  **Copy Artifacts**: It copies the generated `.so` file and the `libc++_shared.so` from the SDK to the OpenHarmony project's libs directory.
3.  **Package the App**: It runs `hvigorw assembleHap` in the `examples/basic-app/openharmony` directory to create the final `.hap` package.

The following command installs the application package on the device, starts the application and monitor logs:
```bash
cargo tauri ohos dev
```
