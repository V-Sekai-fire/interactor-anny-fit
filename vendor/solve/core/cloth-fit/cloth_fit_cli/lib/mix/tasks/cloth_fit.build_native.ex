defmodule Mix.Tasks.ClothFit.BuildNative do
  @shortdoc "Build the PolyFEM static libs and generate the NIF flag files"

  @moduledoc """
  Portable native build for the Fine NIF's PolyFEM dependency.

  Replaces the old platform-specific shell script: this Mix task works on
  Windows (llvm-mingw), Linux and macOS by detecting the toolchain and picking
  the right CMake generator. It:

    1. Builds and installs a static oneTBB 2021.9.0 (the pinned OpenVDB needs
       `TBB_INTERFACE_VERSION >= 12002`; system TBB is often too new).
    2. Configures and builds PolyFEM's `polyfem` static lib with the option set
       that keeps the heavy dependency stack buildable everywhere (OpenVDB
       serialization off; polysolve off MKL/external solvers -> Eigen fallback).
    3. Generates `c_src/polyfem_defines.rsp` (the exact `-D` defines) and
       `c_src/polyfem_link.rsp` (the exact static libraries, resolved per-OS)
       so the Makefile hardcodes no paths or lib names.

  One step from a fresh checkout (after `mix deps.get`): builds the libs, the
  flag files, and the NIF itself. `elixir_make` is skipped automatically until
  `polyfem_link.rsp` exists (see `mix.exs`), so no separate compile is needed:

      mix cloth_fit.build_native

  Afterwards, plain `mix compile` rebuilds the NIF incrementally.
  """

  use Mix.Task

  @tbb_tag "v2021.9.0"

  @impl Mix.Task
  def run(_args) do
    {:ok, _} = Application.ensure_all_started(:jason)

    repo_root = Path.expand("../../../..", __DIR__)
    build_dir = Path.join(repo_root, "build")
    deps_dir = Path.join(repo_root, "build-deps")
    tbb_install = Path.join(deps_dir, "tbb-install")
    cli_root = Path.join(repo_root, "cloth_fit_cli")
    jobs = System.schedulers_online() |> to_string()

    tc = toolchain()
    Mix.shell().info("Toolchain: #{inspect(tc)}")

    usd = usd_config(tc)

    # The garment solve now lives entirely behind the weftfit_retarget dlopen
    # bridge (a SHARED lib that links libpolyfem + the garment core + static TBB,
    # and — with USD — delay-loads the cloth_fit_usd bridge internally). The NIF
    # links ONLY the bridge's loader stub + import lib and dlopens it at runtime, so
    # the NIF itself carries ZERO PolyFEM/TBB/USD. PolyFEM + its deps are still
    # built here because the bridge links them.
    build_tbb(tc, repo_root, deps_dir, tbb_install, jobs)
    configure_polyfem(tc, repo_root, build_dir, tbb_install, usd)
    build(build_dir, "polyfem", jobs)
    # cloth_fit_usd is delay-loaded by the bridge (not a hard link dep), so build it
    # explicitly first; then the weftfit_retarget bridge itself.
    if usd, do: build(build_dir, "cloth_fit_usd", jobs)
    build(build_dir, "weftfit_retarget", jobs)
    # The consumer stub (loader + POSIX dlsym table) isn't linked by anything in the
    # CMake tree — the external NIF links it — so build it explicitly.
    build(build_dir, "weftfit_retarget_stub", jobs)

    gen_defines(build_dir, Path.join(cli_root, "c_src/polyfem_defines.rsp"))
    gen_link(build_dir, tc, Path.join(cli_root, "c_src/polyfem_link.rsp"))

    build_nif(cli_root, tc)
    bundle_runtime_libs(cli_root, build_dir, usd)

    Mix.shell().info("Done. NIF built at priv/polyfem.*")
  end

  # OpenUSD I/O in the NIF is opt-in via CLOTH_FIT_WITH_USD; default builds (incl.
  # nif.yml/Burrito) stay USD-free. ABI-matched to PolyFEM's compiler on every OS:
  # gcc on Linux, clang on macOS, llvm-mingw on Windows (the x86_64-windows-gnu
  # usd_ms triplet — the shipped MSVC triplet would not link against llvm-mingw).
  defp usd_config(tc) do
    if System.get_env("CLOTH_FIT_WITH_USD") in ~w(1 true) do
      # Windows defaults to the MSVC triplet; force the gnu one for llvm-mingw.
      if tc.os == :windows do
        System.put_env("OPENUSD_TARGET", "x86_64-windows-gnu")
      end

      root = StageRuntime.root()
      Mix.shell().info("==> OpenUSD I/O enabled: #{root}")
      %{os: tc.os, root: root, include: StageRuntime.include_dir(), lib: StageRuntime.lib_dir()}
    else
      nil
    end
  end

  defp usd_flags(nil), do: []
  defp usd_flags(%{root: root}), do: ["-DPOLYFEM_WITH_USD=ON", "-DPOLYFEM_USD_ROOT=#{root}"]

  # Static link inputs the NIF adds for the solve bridge: the loader stub always,
  # plus (on Windows) the bridge's import lib so wf_retarget_* bind via delay-load
  # (POSIX binds via the dlsym stub baked into libweftfit_retarget_stub.a).
  defp bridge_link_libs(%{os: :windows}), do: ["libweftfit_retarget_stub.a", "libweftfit_retarget.dll.a"]
  defp bridge_link_libs(_tc), do: ["libweftfit_retarget_stub.a"]

  # Bundle the self-contained solve bridge next to the NIF. With USD it internally
  # delay-loads cloth_fit_usd, so that bridge + its runtime deps (usd_ms + oneTBB) +
  # the USD plugin tree go next to it too — the bridge's cfusd_loader finds priv/usd
  # and LoadLibraryEx's cloth_fit_usd.dll from priv/ (both bridges share the dir).
  defp bundle_runtime_libs(cli_root, build_dir, usd) do
    priv = Path.join(cli_root, "priv")
    File.mkdir_p!(priv)

    bridge =
      Path.wildcard(Path.join(build_dir, "libweftfit_retarget.*")) ++
        Path.wildcard(Path.join(build_dir, "weftfit_retarget.dll"))

    usd_libs =
      case usd do
        %{lib: usd_lib} ->
          Path.wildcard(Path.join(build_dir, "libcloth_fit_usd.*")) ++
            Path.wildcard(Path.join(build_dir, "cloth_fit_usd.dll")) ++
            Path.wildcard(Path.join(usd_lib, "libusd_ms*")) ++
            Path.wildcard(Path.join(usd_lib, "usd_ms*")) ++
            Path.wildcard(Path.join(usd_lib, "libtbb*")) ++
            Path.wildcard(Path.join(usd_lib, "tbb*"))

        nil ->
          []
      end

    for src <- bridge ++ usd_libs,
        not String.ends_with?(src, ".a"),
        not String.ends_with?(src, ".dll.a") do
      File.cp!(src, Path.join(priv, Path.basename(src)))
    end

    bundle_json_specs(build_dir, priv)

    case usd do
      %{root: root} ->
        plugin_src = Path.join([root, "lib", "usd"])
        plugin_dst = Path.join(priv, "usd")

        if File.dir?(plugin_src) do
          File.rm_rf!(plugin_dst)
          File.cp_r!(plugin_src, plugin_dst)
        end

        Mix.shell().info("bundled weftfit_retarget + cloth_fit_usd + usd_ms + oneTBB + plugins into priv/")

      nil ->
        Mix.shell().info("bundled weftfit_retarget bridge into priv/")
    end
  end

  # The spec paths compiled into libpolyfem name the machine that built it --
  # D:/a/cloth-fit on a Windows runner, /home/runner/work on a Linux one -- and
  # exist nowhere else, so a shipped binary could only validate its input on the
  # build machine. CMake stages every spec into build/json-specs; carry that next
  # to the bridge, where SpecPaths.cpp looks before the compiled-in paths.
  defp bundle_json_specs(build_dir, priv) do
    src = Path.join(build_dir, "json-specs")
    dst = Path.join(priv, "json-specs")

    if File.dir?(src) do
      File.rm_rf!(dst)
      File.cp_r!(src, dst)
      Mix.shell().info("bundled #{length(File.ls!(dst))} JSON spec(s) into priv/json-specs")
    else
      Mix.raise("no staged JSON specs at #{src}; the binary would not be relocatable")
    end
  end

  # Build the NIF directly via the Makefile. We invoke make here (rather than
  # relying on elixir_make) because within this single `mix` invocation the
  # compiler list was fixed before polyfem_link.rsp existed.
  defp build_nif(cli_root, tc) do
    Mix.shell().info("==> Building NIF")
    make = if tc.os == :windows, do: find!("mingw32-make"), else: find!("make")

    # The NIF links only the weftfit_retarget stub + import lib (via
    # polyfem_link.rsp) and dlopens the bridge — zero PolyFEM/usd_ms/TBB on its
    # link line, so no USD env is needed here.
    env = [
      {"FINE_INCLUDE_DIR", Fine.include_dir()},
      {"ERTS_INCLUDE_DIR", erts_include_dir()}
    ]

    case System.cmd(make, [], cd: cli_root, env: env,
           into: IO.stream(:stdio, :line), stderr_to_stdout: true) do
      {_, 0} -> :ok
      {_, code} -> Mix.raise("NIF build failed (#{code})")
    end
  end

  defp erts_include_dir do
    root = List.to_string(:code.root_dir())
    vsn = List.to_string(:erlang.system_info(:version))
    Path.join([root, "erts-#{vsn}", "include"])
  end

  # --- toolchain / OS detection ---------------------------------------------

  defp toolchain do
    case :os.type() do
      {:win32, _} ->
        %{
          os: :windows,
          generator: "MinGW Makefiles",
          cc: find!("gcc"),
          cxx: find!("g++"),
          make: find!("mingw32-make")
        }

      {:unix, os} ->
        %{os: os, generator: nil, cc: System.find_executable("cc") || find!("gcc"),
          cxx: System.find_executable("c++") || find!("g++"), make: nil}
    end
  end

  defp find!(name) do
    System.find_executable(name) ||
      Mix.raise("required tool not found on PATH: #{name}")
  end

  defp gen_flags(%{generator: nil}), do: []

  defp gen_flags(%{generator: g, cc: cc, cxx: cxx, make: make}) do
    ["-G", g, "-DCMAKE_MAKE_PROGRAM=#{make}", "-DCMAKE_C_COMPILER=#{cc}",
     "-DCMAKE_CXX_COMPILER=#{cxx}"]
  end

  # --- oneTBB ----------------------------------------------------------------

  defp build_tbb(tc, _repo_root, deps_dir, tbb_install, jobs) do
    if tbb_lib(tbb_install) do
      Mix.shell().info("oneTBB already installed at #{tbb_install}")
    else
      Mix.shell().info("==> Building oneTBB #{@tbb_tag} (static)")
      src = Path.join(deps_dir, "oneTBB")
      build = Path.join(deps_dir, "tbb-build")
      File.mkdir_p!(deps_dir)

      unless File.dir?(src) do
        cmd("git", ["clone", "--depth", "1", "--branch", @tbb_tag,
                    "https://github.com/oneapi-src/oneTBB.git", src])
      end

      cmd("cmake", ["-S", src, "-B", build] ++ gen_flags(tc) ++
            ["-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=OFF",
             "-DTBB_TEST=OFF", "-DTBB_EXAMPLES=OFF", "-DTBB_STRICT=OFF",
             "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
             "-DCMAKE_INSTALL_PREFIX=#{tbb_install}"])

      cmd("cmake", ["--build", build, "-j", jobs, "--target", "install"])
    end
  end

  defp tbb_lib(tbb_install) do
    # oneTBB installs to lib on Debian/Ubuntu but lib64 on Fedora (GNUInstallDirs).
    (Path.wildcard(Path.join([tbb_install, "lib*", "libtbb12.a"])) ++
       Path.wildcard(Path.join([tbb_install, "lib*", "libtbb.a"])))
    |> List.first()
  end

  # --- PolyFEM ---------------------------------------------------------------

  defp configure_polyfem(tc, repo_root, build_dir, tbb_install, usd) do
    Mix.shell().info("==> Configuring PolyFEM")

    cmd("cmake", ["-S", repo_root, "-B", build_dir] ++ gen_flags(tc) ++ usd_flags(usd) ++
          ["-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
           # Emit the polyfem target's include flags into includes_CXX.rsp on
           # every generator (MinGW does this by default, Unix Makefiles inlines
           # them) so the NIF Makefile can reuse that exact include set.
           "-DCMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES=ON",
           "-DOPENVDB_USE_DELAYED_LOADING=OFF",
           "-DTBB_ROOT=#{tbb_install}", "-DCMAKE_PREFIX_PATH=#{tbb_install}",
           "-DTBB_USE_STATIC_LIBS=ON",
           "-DUSE_BLOSC=OFF", "-DUSE_ZLIB=OFF", "-DUSE_IMATH_HALF=OFF",
           "-DPOLYSOLVE_WITH_MKL=OFF", "-DPOLYSOLVE_WITH_CHOLMOD=OFF",
           "-DPOLYSOLVE_WITH_UMFPACK=OFF", "-DPOLYSOLVE_WITH_SUPERLU=OFF",
           "-DPOLYSOLVE_WITH_HYPRE=OFF", "-DPOLYSOLVE_WITH_AMGCL=OFF",
           "-DPOLYSOLVE_WITH_SPECTRA=OFF", "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"])
  end

  defp build(build_dir, target, jobs) do
    Mix.shell().info("==> Building #{target}")
    cmd("cmake", ["--build", build_dir, "-j", jobs, "--target", target])
  end

  # --- generated flag files --------------------------------------------------

  # Extract the exact -D defines PolyFEM compiles with from compile_commands.json
  # and write them to a gcc/clang response file (verbatim, preserving \"..\").
  defp gen_defines(build_dir, out) do
    entries = build_dir |> Path.join("compile_commands.json") |> File.read!() |> Jason.decode!()

    entry =
      Enum.find(entries, fn e ->
        String.ends_with?(String.replace(e["file"], "\\", "/"), "garment/optimize.cpp")
      end) || Mix.raise("no polyfem TU in compile_commands.json")

    defines =
      Regex.scan(~r/(?:^|\s)(-D\S+)/, entry["command"])
      |> Enum.map(fn [_, d] -> d end)
      |> Enum.uniq()

    File.write!(out, Enum.join(defines, "\n") <> "\n")
    Mix.shell().info("wrote #{length(defines)} defines -> #{out}")
  end

  # The NIF links ONLY the weftfit_retarget solve bridge — its loader stub + (on
  # Windows) its delay-loaded import lib. PolyFEM, its deps, TBB and USD all live
  # inside the bridge .dll/.so, so none of them appear on the NIF's link line.
  defp gen_link(build_dir, tc, out) do
    libs =
      bridge_link_libs(tc)
      |> Enum.map(&Path.join(build_dir, &1))
      |> Enum.filter(&File.exists?/1)

    # On Windows the bridge is delay-loaded: the NIF loads without it (the bridge's
    # own deps — cloth_fit_usd/usd_ms/tbb — aren't on the OS search path), and
    # wfrt_loader::load_from_env() LoadLibraryEx's it from priv/ (altered search
    # path) before the first wf_retarget_* call binds the delay thunks.
    delay =
      case tc do
        %{os: :windows} -> "-Wl,-delayload,libweftfit_retarget.dll\n"
        _ -> ""
      end

    body =
      "-Wl,--start-group\n" <> Enum.join(libs, "\n") <> "\n-Wl,--end-group\n" <> delay

    File.write!(out, body)
    Mix.shell().info("wrote #{length(libs)} link inputs -> #{out}")
  end

  # --- helpers ---------------------------------------------------------------

  defp cmd(exe, args) do
    exe = System.find_executable(exe) || exe
    Mix.shell().info("$ #{exe} #{Enum.join(args, " ")}")

    case System.cmd(exe, args, into: IO.stream(:stdio, :line), stderr_to_stdout: true) do
      {_, 0} -> :ok
      {_, code} -> Mix.raise("command failed (#{code}): #{exe}")
    end
  end
end
