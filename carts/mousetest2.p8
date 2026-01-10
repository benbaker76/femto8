pico-8 cartridge // http://www.pico-8.com
version 16
__lua__
function _init()
	-- enable mouse and keyboard devkit mode
	-- 0x1 = enable, 0x2 = buttons trigger btn(4..6), 0x4 = pointer lock
	poke(0x5f2d, 7)
	cls()
	x = 0
	mx = 64
	my = 64
	mz = 0
	mb = 0
	mxrel = 0
	myrel = 0
	-- track previous mouse pos for clearing
	pmx = 64
	pmy = 64
	pmb = 0
	-- buffer to save screen under cursor (9x9 area)
	cbuf = {}
	for i=0,80 do
		cbuf[i] = 0
	end
end

function save_cursor_area()
	-- save previous position
	pmx = mx
	pmy = my
	pmb = mb

	-- save 9x9 area centered on cursor
	local idx = 0
	for yy=-4,4 do
		for xx=-4,4 do
			local px = pmx + xx
			local py = pmy + yy
			if px >= 0 and px < 128 and py >= 0 and py < 128 then
				cbuf[idx] = pget(px, py)
			else
				cbuf[idx] = 0
			end
			idx += 1
		end
	end
end

function restore_cursor_area()
	-- restore saved area
	local idx = 0
	for yy=-4,4 do
		for xx=-4,4 do
			local px = pmx + xx
			local py = pmy + yy
			if px >= 0 and px < 128 and py >= 0 and py < 128 then
				pset(px, py, cbuf[idx])
			end
			idx += 1
		end
	end
end

function _update()
	-- read mouse position
	mx = stat(32)
	my = stat(33)
	mb = stat(34)

	-- read mouse wheel
	local wheel = stat(36)
	if wheel == 1 then
		mz += 1
	elseif wheel == -1 then
		mz -= 1
	end

	-- read relative mouse movement
	mxrel = stat(38)
	myrel = stat(39)
end

function _draw()
	-- restore old cursor area
	restore_cursor_area()

	-- button trace like btntest
	pset(x-1, 8, 0)
	pset(x, 8, 7)

	-- show button states (includes mouse buttons via btn emulation)
	for n=0,6 do
		if btn(n) then
			pset(x, 16 + n * 2, 7)
		end
		if btnp(n) then
			pset(x, 16 + n * 2, 8)
		end
	end

	x += 1
	if x > 127 then
		x = 0
		cls()
	end

	-- draw mouse info panel
	rectfill(0, 100, 127, 127, 1)
	print("x:"..mx, 2, 102, 7)
	print("y:"..my, 2, 108, 7)
	print("z:"..mz, 2, 114, 7)
	print("b:"..mb, 2, 120, 7)
	print("xr:"..mxrel, 50, 102, 7)
	print("yr:"..myrel, 50, 108, 7)

	-- decode button bits
	local btxt = ""
	if band(mb, 0x1) > 0 then btxt = btxt.."l" end
	if band(mb, 0x2) > 0 then btxt = btxt.."r" end
	if band(mb, 0x4) > 0 then btxt = btxt.."m" end
	print(btxt, 20, 120, 8)

	-- save area under cursor position
	save_cursor_area()

	-- draw cursor sprite at mouse position
	-- use crosshair cursor from sprite 1
	if mx >= 0 and mx < 128 and my >= 0 and my < 128 then
		spr(1, mx-3, my-3)

		-- show button press with filled circle
		if mb > 0 then
			circfill(mx, my, 4, 8)
		end
	end
end
__gfx__
00000000000700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00700700000700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00077000777877700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00077000000700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00700700000700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

