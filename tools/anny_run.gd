extends SceneTree
# godot --path <any project> -s anny_run.gd -- <anny_fit.elf> <soma_pheno.bin> <targets.txt> <labels.txt> "<config>"
func _initialize() -> void:
	var a := OS.get_cmdline_user_args()
	var sb: Node = ClassDB.instantiate("Sandbox")
	sb.memory_max = 1536
	sb.references_max = 4096
	sb.allocations_max = 10000000
	sb.execution_timeout = 0
	root.add_child(sb)
	sb.load_buffer(FileAccess.get_file_as_bytes(a[0]))
	print("ANNY loaded, functions: ", sb.has_function("anny_fit_start"))
	var f := FileAccess.open(a[1], FileAccess.READ)
	var t0 := Time.get_ticks_msec()
	var chunk := 8 << 20
	var total := 0
	while f.get_position() < f.get_length():
		var part := f.get_buffer(chunk)
		total = int(sb.vmcall("anny_data_put", "pheno", part))
	print("ANNY uploaded ", total, " bytes in ", (Time.get_ticks_msec() - t0) / 1000.0, " s")
	print("ANNY put targets -> ", sb.vmcall("anny_data_put", "targets", FileAccess.get_file_as_bytes(a[2])))
	print("ANNY put labels -> ", sb.vmcall("anny_data_put", "labels", FileAccess.get_file_as_bytes(a[3])))
	print("ANNY start: ", sb.vmcall("anny_fit_start", a[4]))
	print("ANNY stage: ", sb.vmcall("anny_stage"))
	var frames := 0
	var s0 := Time.get_ticks_msec()
	while true:
		await process_frame
		var r := str(sb.vmcall("anny_fit_tick"))
		frames += 1
		if frames % 200 == 0:
			print("ANNY ", r, "  (", (Time.get_ticks_msec() - s0) / 1000.0, " s)")
		if r.begins_with("DONE") or r.begins_with("FAIL"):
			print("ANNY ", r)
			break
		if (Time.get_ticks_msec() - s0) > int(a[5]) * 1000:
			print("ANNY timeout after ", frames, " frames: ", r)
			break
	print("ANNY report:\n", sb.vmcall("anny_fit_report"))
	quit()
