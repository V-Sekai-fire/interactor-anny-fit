# The rotation-star kernels on the GPU: the SPIR-V in kernels/spv, run through a Godot
# RenderingDevice in a windowed Godot, never --headless. Dispatches star_kabsch and swing_twist
# on the oracle's inputs (written by rs_kernel_test --write) and on the planted controls' inputs,
# and writes the outputs for rs_kernel_test --gpu to gate.
#
#   RS_IN=<dir> RS_SPV=<kernels/spv> godot --path <project> -s gpu_run.gd
extends SceneTree

var rd: RenderingDevice
var io: String
var spv: String

func _init() -> void:
	io = OS.get_environment("RS_IN")
	spv = OS.get_environment("RS_SPV")
	rd = RenderingServer.create_local_rendering_device()
	if rd == null:
		print("FAIL rotation-star gpu: no RenderingDevice")
		quit(1)
		return
	print("device: %s" % rd.get_device_name())
	quit(_run())

func _bytes(path: String) -> PackedByteArray:
	var b := FileAccess.get_file_as_bytes(path)
	if b.is_empty():
		push_error("cannot read " + path)
	return b

func _sbuf(b: PackedByteArray) -> RID:
	var c := b.duplicate()
	c.resize(maxi(16, (b.size() + 15) / 16 * 16))
	return rd.storage_buffer_create(c.size(), c)

func _zeros(n_bytes: int) -> RID:
	var b := PackedByteArray()
	b.resize(n_bytes)
	return _sbuf(b)

func _params(n: int) -> RID:
	var b := PackedByteArray()
	b.resize(16)
	b.encode_u32(0, n)
	return rd.uniform_buffer_create(16, b)

func _shader(k: String) -> RID:
	var s := RDShaderSPIRV.new()
	s.set_stage_bytecode(RenderingDevice.SHADER_STAGE_COMPUTE, _bytes(spv + "/" + k + ".spv"))
	if s.compile_error_compute != "":
		push_error(s.compile_error_compute)
	return rd.shader_create_from_spirv(s, k)

# Binding 0 the params block, bindings 1.. the storage buffers in order; one dispatch, read back.
func _dispatch(sh: RID, n: int, bufs: Array) -> void:
	var us := []
	var u := RDUniform.new()
	u.uniform_type = RenderingDevice.UNIFORM_TYPE_UNIFORM_BUFFER
	u.binding = 0
	u.add_id(_params(n))
	us.append(u)
	for i in bufs.size():
		var s := RDUniform.new()
		s.uniform_type = RenderingDevice.UNIFORM_TYPE_STORAGE_BUFFER
		s.binding = i + 1
		s.add_id(bufs[i])
		us.append(s)
	var uset := rd.uniform_set_create(us, sh, 0)
	var cl := rd.compute_list_begin()
	rd.compute_list_bind_compute_pipeline(cl, rd.compute_pipeline_create(sh))
	rd.compute_list_bind_uniform_set(cl, uset, 0)
	rd.compute_list_dispatch(cl, (n + 63) / 64, 1, 1)
	rd.compute_list_end()
	rd.submit()
	rd.sync()

func _save(buf: RID, n_bytes: int, name: String) -> void:
	var f := FileAccess.open(io + "/" + name, FileAccess.WRITE)
	f.store_buffer(rd.buffer_get_data(buf, 0, n_bytes))
	f.close()

func _run() -> int:
	var meta: Dictionary = JSON.parse_string(FileAccess.get_file_as_string(io + "/meta.json"))
	var nk: int = meta["nk"]
	var ns: int = meta["ns"]
	var sk := _shader("star_kabsch")
	var st := _shader("swing_twist")
	var src := _sbuf(_bytes(io + "/k_src.bin"))
	for v in [["k_tgt.bin", "k_rot.bin"], ["k_tgt_bent.bin", "k_rot_bent.bin"]]:
		var rot := _zeros(4 * 9 * nk)
		_dispatch(sk, nk, [_sbuf(_bytes(io + "/" + v[0])), src, rot])
		_save(rot, 4 * 9 * nk, v[1])
	var srot := _sbuf(_bytes(io + "/s_rot.bin"))
	for v in [["s_axis.bin", ""], ["s_axis_wrong.bin", "_wrong"]]:
		var sw := _zeros(4 * 9 * ns)
		var tw := _zeros(4 * 9 * ns)
		var an := _zeros(4 * ns)
		_dispatch(st, ns, [srot, _sbuf(_bytes(io + "/" + v[0])), sw, tw, an])
		_save(sw, 4 * 9 * ns, "s_swing" + v[1] + ".bin")
		_save(tw, 4 * 9 * ns, "s_twist" + v[1] + ".bin")
		_save(an, 4 * ns, "s_angle" + v[1] + ".bin")
	print("DONE rotation-star gpu (%d stars, %d splits)" % [nk, ns])
	return 0
