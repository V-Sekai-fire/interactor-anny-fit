# Pin each kernel's emitted Slang into its Lean module.
#
# Every lean/RotationStar/Slang/<Module>.lean carries a block
#
#     -- BEGIN PIN
#     def expected : String := "..."
#     example : LeanSlang.emit shader = expected := by native_decide
#     example : shader.entryPointName = "main" := by native_decide
#     -- END PIN
#
# This rewrites that block from kernels/slang/<kernel>.slang, the text `lake exe
# emit_rotation_star` wrote. Run it only after a deliberate change to a kernel (then gen.sh,
# then `lake build` re-checks every pin); an unintended emission change is what the pins catch.
#
#     elixir kernels/pin.exs            # rewrite the pins
#     elixir kernels/pin.exs --check    # exit 1 if any pin is stale
#     elixir kernels/pin.exs --clear    # empty the blocks (a changed kernel cannot build its
#                                       # emitter past its own stale pin), then gen.sh, then pin
here = __DIR__
lean = Path.join([here, "..", "lean", "RotationStar", "Slang"])
modules = [{"StarKabsch", "star_kabsch"}, {"SwingTwist", "swing_twist"}]
check = System.argv() == ["--check"]
clear = System.argv() == ["--clear"]

escape = fn text ->
  text
  |> String.replace("\\", "\\\\")
  |> String.replace("\"", "\\\"")
  |> String.replace("\n", "\\n")
end

stale =
  Enum.filter(modules, fn {mod, kernel} ->
    path = Path.join(lean, mod <> ".lean")
    src = File.read!(path)
    slang = File.read!(Path.join([here, "slang", kernel <> ".slang"])) |> String.replace_suffix("\n", "")

    block =
      if clear do
        "-- BEGIN PIN\n-- END PIN"
      else
        "-- BEGIN PIN\n" <>
          "def expected : String := \"" <> escape.(slang) <> "\"\n" <>
          "example : LeanSlang.emit shader = expected := by native_decide\n" <>
          "example : shader.entryPointName = \"main\" := by native_decide\n" <>
          "-- END PIN"
      end

    [pre, rest] = String.split(src, "-- BEGIN PIN", parts: 2)
    [_, post] = String.split(rest, "-- END PIN", parts: 2)
    new = pre <> block <> post

    if new != src do
      if check, do: IO.puts("stale pin: #{mod}"), else: File.write!(path, new)
      true
    else
      false
    end
  end)

cond do
  check and stale != [] -> System.halt(1)
  check -> IO.puts("pins current")
  true -> IO.puts("rewrote #{length(stale)} pin(s)")
end
