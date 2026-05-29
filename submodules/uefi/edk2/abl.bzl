load("//build/kernel/kleaf:directory_with_structure.bzl", dws = "directory_with_structure")
load("//build/kernel/kleaf:hermetic_tools.bzl", "hermetic_toolchain")
load("//msm-kernel:target_variants.bzl", "get_all_variants")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("@kernel_toolchain_info//:dict.bzl", "VARS")
load(
    "//build:abl_extensions.bzl",
    "extra_build_configs",
    "extra_function_snippets",
    "extra_post_gen_snippets",
    "extra_srcs",
)

def _abl_impl(ctx):
    inputs = []
    inputs += ctx.files.srcs
    inputs += ctx.files.deps
    inputs += ctx.files.kernel_build_config

    output_files = [ctx.actions.declare_file("{}.tar.gz".format(ctx.label.name))]

    hermetic_tools = hermetic_toolchain.get(ctx)

    command = hermetic_tools.setup
    command += """
      set -o errexit
      ROOT_DIR="$PWD"
      ABL_OUT_DIR=${{ROOT_DIR}}/bootable/bootloader/edk2/out
      CLANG_VERSION="{clang_version}"
      CLANG_PREBUILT_BIN="prebuilts/clang/host/linux-x86/clang-$CLANG_VERSION/bin"

      # Stub out append_cmd
      append_cmd() {{
        :
      }}
      export -f append_cmd

      source "{kernel_build_config}"

      if [ -n "$CLANG_VERSION" ]; then
        export PATH="${{ROOT_DIR}}/prebuilts/clang/host/linux-x86/clang-${{CLANG_VERSION}}/bin:${{PATH}}"
      else
        export PATH="${{ROOT_DIR}}"/prebuilts/clang/host/linux-x86/*/bin:"${{PATH}}"
      fi

      abl_image_generate() {{
        PREBUILT_HOST_TOOLS="BUILD_CC=clang BUILD_CXX=clang++ LDPATH=-fuse-ld=lld BUILD_AR=llvm-ar"

        MKABL_ARGS=("-C" "${{ROOT_DIR}}/${{ABL_SRC}}")
        MKABL_ARGS+=("BOOTLOADER_OUT=${{ABL_OUT_DIR}}/obj/ABL_OUT" "all")
        MKABL_ARGS+=("BUILDDIR=${{ROOT_DIR}}/${{ABL_SRC}}")
        MKABL_ARGS+=("PREBUILT_HOST_TOOLS=${{PREBUILT_HOST_TOOLS}}")
        MKABL_ARGS+=("${{MAKE_FLAGS[@]}}")
        MKABL_ARGS+=("CLANG_BIN=${{ROOT_DIR}}/${{CLANG_PREBUILT_BIN}}/")

        echo "MAKING"
        make "${{MKABL_ARGS[@]}}"
        echo "MADE"

        ABL_DEBUG_FILE="$(find "${{ABL_OUT_DIR}}" -name LinuxLoader.debug)"
        if [ -e "${{ABL_DEBUG_FILE}}" ]; then
          cp "${{ABL_DEBUG_FILE}}" "${{ABL_IMAGE_DIR}}/LinuxLoader_${{TARGET_BUILD_VARIANT}}.debug"
          cp "${{ABL_OUT_DIR}}/unsigned_abl.elf" "${{ABL_IMAGE_DIR}}/unsigned_abl_${{TARGET_BUILD_VARIANT}}.elf"
        fi
        if [ "${{AUTO_VIRT_ABL}}" = "1" ]; then
          cp "${{ABL_OUT_DIR}}/LinuxLoader.efi" "${{ABL_IMAGE_DIR}}/LinuxLoader_${{TARGET_BUILD_VARIANT}}.efi"
        fi

        find "${{ABL_OUT_DIR}}" -type d -name "abl-${{TARGET_BUILD_VARIANT}}" -exec cp -ar {{}} "$ABL_IMAGE_DIR" \\;
      }}
    """.format(
        kernel_build_config = ctx.file.kernel_build_config.path,
        clang_version = ctx.attr.clang_version,
    )

    for snippet in ctx.attr.extra_function_snippets:
        command += snippet

    command += """
      echo "========================================================"
      echo " Building abl"

      [ -z "${{ABL_SRC}}" ] && ABL_SRC=bootable/bootloader/edk2

      if [ ! -d "${{ROOT_DIR}}/${{ABL_SRC}}" ]; then
        echo "*** STOP *** Please check the edk2 path: ${{ROOT_DIR}}/${{ABL_SRC}}"
        exit 1
      fi

      export TARGET_BUILD_VARIANT={target_build_variant}

      for extra_config in {extra_build_configs}; do
        source "${{extra_config}}"
      done

      source "${{ABL_SRC}}/QcomModulePkg/{build_config}"

      [ -z "${{ABL_OUT_DIR}}" ] && ABL_OUT_DIR=${{COMMON_OUT_DIR}}

      ABL_OUT_DIR=${{ABL_OUT_DIR}}/abl-${{TARGET_BUILD_VARIANT}}
      ABL_IMAGE_NAME=abl_${{TARGET_BUILD_VARIANT}}.elf

      export ABL_IMAGE_DIR=$(mktemp -d)
      mkdir -p "$ABL_IMAGE_DIR"

      abl_image_generate
    """.format(
        extra_build_configs = " ".join(ctx.attr.extra_build_configs),
        build_config = ctx.attr.abl_build_config,
        target_build_variant = ctx.attr.target_build_variant[BuildSettingInfo].value,
    )

    for snippet in ctx.attr.extra_post_gen_snippets:
        command += snippet

    command += """
      # Copy to bazel output dir
      abs_out_dir="${{PWD}}/{abl_out_dir}"
      mkdir -p "${{abs_out_dir}}"
      file_list="./*.elf ./abl-${{TARGET_BUILD_VARIANT}}"
      if [ "${{AUTO_VIRT_ABL}}" = "1" ]; then
        file_list+=" ./*.efi"
      fi
      cd "${{ABL_IMAGE_DIR}}" && tar -czf "${{abs_out_dir}}/{abl_out_name}" ${{file_list}}
      """.format(
        abl_out_dir = output_files[0].dirname,
        abl_out_name = output_files[0].basename,
        rule_ext = ctx.label.name,
    )

    ctx.actions.run_shell(
        mnemonic = "Abl",
        inputs = depset(inputs),
        outputs = output_files,
        tools = hermetic_tools.deps,
        command = command,
        progress_message = "Building {}".format(ctx.label),
    )

    return [
        DefaultInfo(
            files = depset(output_files),
        ),
    ]

abl = rule(
    implementation = _abl_impl,
    doc = """Generates a rule that builds the Android Bootloader (ABL)

    Example:
    ```
    abl(
        name = "kalama_gki_abl",
        srcs = glob(["**"])
        abl_build_config = "build.config.msm.kalama",
        kernel_build = "//msm-kernel:kalama_gki",
    )
    ```

    Args:
        name: Name of the abl target
        srcs: Source files to build the abl elf. If unspecified or value
          is `None`, it is by default a full glob of the directory:
          ```
          glob(["**"])
          ```
        kernel_build_config: Label referring to the kernel build.config
        abl_build_config: ABL build config
        extra_function_snippets: list of additional shell functions to define at the top of
          the build script
        extra_post_gen_snippets: list of additional shell commands to run at the end of
          the build script
        extra_build_configs: list of additional build configs to source prior to building
        clang_version: version of clang to use (e.g. "clang-r450784e"). By default, use the
          version from the kernel build. Make sure whatever version you use is added to
          `extra_deps` as well.
        kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """,
    attrs = {
        "srcs": attr.label_list(
            mandatory = True,
            allow_files = True,
        ),
        "deps": attr.label_list(),
        "kernel_build_config": attr.label(
            allow_single_file = True,
        ),
        "abl_build_config": attr.string(),
        "target_build_variant": attr.label(default = ":target_build_variant"),
        "extra_function_snippets": attr.string_list(),
        "extra_post_gen_snippets": attr.string_list(),
        "extra_build_configs": attr.string_list(),
        "clang_version": attr.string()
    },
    toolchains = [
        hermetic_toolchain.type,
    ],
)

def define_abl_targets():
    for target, variant in get_all_variants():
        define_abl(target, variant)

def define_abl(msm_target, variant):
    target = msm_target + "_" + variant

    if msm_target == "autogvm":
        return

    clang_version = VARS["CLANG_VERSION"]
    extra_deps = ["//prebuilts/clang/host/linux-x86/clang-{}:binaries".format(clang_version)]

    kernel_build_config = "//msm-kernel:{}_build_config_bazel".format(target)
    abl_build_config = "build.config.msm.{}".format(msm_target.replace("-", "."))
    # Use "{}.lxc" config if its a non-GKI target/variant combination
    if msm_target == "gen4auto":
        if variant == "perf-defconfig" or variant == "debug-defconfig":
            abl_build_config = "{}.lxc".format(abl_build_config)

    abl(
        name = "{}_abl".format(target),
        kernel_build_config = kernel_build_config,
        abl_build_config = abl_build_config,
        srcs = native.glob(
            ["**"],
            exclude=[
                "**/*.pyc",
                "**/__pycache__/**",
                "Conf/BuildEnv.sh",
                "Conf/.AutoGenIdFile.txt",
                "Build/**",
                "out/**",
            ],
        ) + extra_srcs,
        extra_function_snippets = extra_function_snippets,
        extra_post_gen_snippets = extra_post_gen_snippets,
        extra_build_configs = extra_build_configs,
        clang_version = clang_version,
        deps = extra_deps,
        visibility = ["//visibility:public"],
    )
