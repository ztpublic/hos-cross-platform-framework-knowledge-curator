# Build the OHOS askpass helper as a real ELF executable (not a .so).
#
# Why not add_qtc_executable: on OHOS it calls qt6_add_executable
# (QtCreatorAPI.cmake:930-931) which produces a .so shared library, not an
# executable. But ssh needs to fork+exec a real ELF binary with a main
# entry point — a .so has no entry point and cannot be exec'd.
#
# Why not guarded by if(OHOS): the parent src/CMakeLists.txt already guards
# this subdirectory with if(OHOS), so OHOS is guaranteed here. Also, the
# OHOS variable is not a cache variable and may not be visible in this
# child scope.
#
# Why not add_qtc_executable's BUILD_EXECUTABLES_BY_DEFAULT: that is set OFF
# by the top-level CMakeLists.txt:75 for OHOS static builds. Using native
# add_executable bypasses that gate entirely, so no FORCE hack is needed.
#
# The OHOS cross-compiler (aarch64-linux-ohos-clang++) produces a real ELF
# executable by default. Signing for on-device execution is a separate
# post-build step (hapsigner elf mode), handled during HAP packaging.
add_executable(qtc-askpass-ohos askpass-ohos.cpp)
target_compile_features(qtc-askpass-ohos PRIVATE cxx_std_17)

# Link OHOS hilog NDK library for system logging (visible via `hdc shell hilog`).
# The lib is libhilog_ndk.z.so in the OHOS NDK sysroot; the .z. in the filename
# breaks the default -l lookup (lib<name>.so), so we resolve the full path.
# This is a system lib on OHOS, always available at runtime.
set(_qtc_hilog_lib "")
if(DEFINED CMAKE_SYSROOT AND CMAKE_SYSROOT)
    file(GLOB _qtc_hilog_candidates "${CMAKE_SYSROOT}/usr/lib/*/libhilog_ndk.z.so")
    if(_qtc_hilog_candidates)
        list(GET _qtc_hilog_candidates 0 _qtc_hilog_lib)
    endif()
endif()
if(_qtc_hilog_lib)
    target_link_libraries(qtc-askpass-ohos PRIVATE ${_qtc_hilog_lib})
    message(STATUS "qtc-askpass-ohos: linked hilog lib at ${_qtc_hilog_lib}")
else()
    # Fallback: let the linker search sysroot with -l: syntax (exact filename)
    target_link_libraries(qtc-askpass-ohos PRIVATE "-l:libhilog_ndk.z.so")
    message(WARNING "qtc-askpass-ohos: hilog lib not found via glob, using -l:libhilog_ndk.z.so fallback")
endif()

# Output the binary to the bin directory (same as add_qtc_executable does).
# Without this, native add_executable defaults to ${CMAKE_CURRENT_BINARY_DIR}
# (src/tools/qtc-askpass-ohos/), not bin/. qtc_output_binary_dir resolves the
# correct build-tree path (matches QtCreatorAPI.cmake:973-986).
qtc_output_binary_dir(_output_binary_dir)
set_target_properties(qtc-askpass-ohos PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${_output_binary_dir}/${IDE_BIN_PATH}"
)

# Also install to bin/ for `cmake --install`.
install(TARGETS qtc-askpass-ohos DESTINATION "${IDE_BIN_PATH}")

# Inject a .permission section into the ELF binary so OHOS can read the
# permission manifest at runtime. This uses llvm-objcopy from the DevEco
# OHOS SDK to add the section in-place (input == output path; llvm-objcopy
# reads the input into memory before writing, so in-place overwriting is safe).
#
# Path resolution order:
#   1. QTC_OHOS_LLVM_OBJCOPY cache variable (explicit override)
#   2. ${OHOS_SDK_ROOT}/native/llvm/bin/llvm-objcopy.exe (DevEco SDK layout)
#      or ${OHOS_SDK_ROOT}/llvm/bin/llvm-objcopy.exe (if OHOS_SDK_ROOT points
#      directly at the "native" directory)
#
# The permission JSON lives at ${QtCreator_SOURCE_DIR}/bin_permission.json.
# If llvm-objcopy or the permission file is not found, this step is skipped
# with a warning so non-OHOS-SDK builds don't fail.
set(QTC_OHOS_LLVM_OBJCOPY "" CACHE FILEPATH
  "Path to llvm-objcopy for patching OHOS ELF binaries (defaults to OHOS_SDK_ROOT/native/llvm/bin/llvm-objcopy.exe)")
set(_qtc_llvm_objcopy "${QTC_OHOS_LLVM_OBJCOPY}")
if(NOT _qtc_llvm_objcopy AND DEFINED OHOS_SDK_ROOT AND OHOS_SDK_ROOT)
  cmake_path(NORMAL_PATH OHOS_SDK_ROOT OUTPUT_VARIABLE _qtc_ohos_sdk_root)
  cmake_path(GET _qtc_ohos_sdk_root FILENAME _qtc_ohos_sdk_leaf)
  if(_qtc_ohos_sdk_leaf STREQUAL "openharmony")
    set(_qtc_llvm_objcopy "${_qtc_ohos_sdk_root}/native/llvm/bin/llvm-objcopy.exe")
  elseif(_qtc_ohos_sdk_leaf STREQUAL "native")
    set(_qtc_llvm_objcopy "${_qtc_ohos_sdk_root}/llvm/bin/llvm-objcopy.exe")
  endif()
endif()

set(_qtc_permission_file "${QtCreator_SOURCE_DIR}/bin_permission.json")

if(_qtc_llvm_objcopy AND EXISTS "${_qtc_llvm_objcopy}" AND EXISTS "${_qtc_permission_file}")
  add_custom_command(TARGET qtc-askpass-ohos POST_BUILD
    COMMAND "${_qtc_llvm_objcopy}"
      --add-section ".permission=${_qtc_permission_file}"
      "$<TARGET_FILE:qtc-askpass-ohos>"
      "$<TARGET_FILE:qtc-askpass-ohos>"
    COMMENT "Injecting .permission section into qtc-askpass-ohos"
    VERBATIM
  )
else()
  message(WARNING
    "llvm-objcopy or bin_permission.json not found; skipping .permission section injection for qtc-askpass-ohos. "
    "Set QTC_OHOS_LLVM_OBJCOPY or OHOS_SDK_ROOT to enable.")
endif()
