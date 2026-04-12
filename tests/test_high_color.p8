pico-8 cartridge // http://www.pico-8.com
version 43
__lua__

-- high-color mode test cart
-- exercises each poke(0x5f5f) mode
-- press z/x to advance

function draw_mode_0()
  poke(0x5f5f, 0)
  poke(0x5f2c, 0)
  cls(0)
  -- draw colored stripes
  for i=0,15 do
    rectfill(0, i*8, 127, i*8+7, i)
  end
  print("mode 0", 44, 58, 7)
  print("normal 16-color", 24, 66, 7)
end

function draw_mode_10()
  poke(0x5f5f, 0x10)
  poke(0x5f2c, 0)
  cls(0)
  -- set secondary palette: remap to dark colors
  for i=0,15 do
    pal(i, i+128, 2)
  end
  -- set bitfield: bottom half uses secondary palette
  memset(0x5f70, 0x00, 8)
  memset(0x5f78, 0xff, 8)
  -- draw colored stripes
  for i=0,15 do
    rectfill(0, i*8, 127, i*8+7, i)
  end
  print("mode 0x10", 36, 26, 7)
  print("per-line palette", 20, 34, 7)
  print("top=primary", 32, 74, 7)
  print("bottom=secondary", 16, 82, 7)
end

function draw_mode_20()
  poke(0x5f5f, 0x20)
  poke(0x5f2c, 1) -- required: horizontal stretch
  cls(0)
  -- set secondary palette: remap to dark colors
  for i=0,15 do
    pal(i, i+128, 2)
  end
  -- draw colored stripes in left half (visible)
  for i=0,15 do
    rectfill(0, i*8, 63, i*8+7, i)
  end
  -- draw mask in right half: non-zero where secondary palette applies
  -- fill right half top portion with color 1 as mask
  for y=0,63 do
    for x=64,127 do
      pset(x, y, 1)
    end
  end
  print("mode 0x20", 4, 26, 7)
  print("5-bitplane", 8, 34, 7)
  print("top=2nd pal", 4, 74, 7)
  print("bottom=1st", 8, 82, 7)
end

function draw_mode_30()
  poke(0x5f5f, 0x30) -- replace color 0 with gradient
  poke(0x5f2c, 0)
  cls(0) -- fill with color 0 (will be replaced)
  -- set gradient colors in secondary palette
  pal({[0]=1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,8}, 2)
  -- clear bitfield
  memset(0x5f70, 0x00, 16)
  -- draw a small label area with non-zero color
  rectfill(20, 52, 108, 76, 5)
  print("mode 0x30", 36, 56, 7)
  print("gradient fill", 28, 64, 7)
end

function draw_mode_3n()
  poke(0x5f5f, 0x35) -- replace color 5 with gradient
  poke(0x5f2c, 0)
  cls(0)
  -- fill screen with color 5 to show gradient
  rectfill(0, 0, 127, 127, 5)
  -- set gradient colors
  pal({[0]=128+1,1,128+12,12,128+13,13,6,7,128+8,8,128+14,14,128+15,15,128+9,9}, 2)
  -- blend lines with alternating bitfield
  memset(0x5f70, 0xaa, 16)
  rectfill(20, 52, 108, 76, 0)
  print("mode 0x3n", 36, 56, 7)
  print("gradient + blend", 16, 64, 7)
end

modes = {
  draw_mode_0,
  draw_mode_10,
  draw_mode_20,
  draw_mode_30,
  draw_mode_3n,
}

current = 1

function _init()
  modes[current]()
end

function _update()
  if btnp(4) or btnp(5) then
    -- reset palette and modes before switching
    pal()
    poke(0x5f5f, 0)
    poke(0x5f2c, 0)
    memset(0x5f70, 0, 16)
    current += 1
    if current > #modes then
      current = 1
    end
    modes[current]()
  end
end

function _draw()
  -- drawing is done in mode functions
end
