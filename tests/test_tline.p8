pico-8 cartridge // http://www.pico-8.com
version 43
__lua__

-- tline test cart
-- tests tline() in many scenarios, verifying pixels with pget

current_test_suite_name = ""
current_test_case_name = ""
current_test_case_failed = false
current_test_suite_failed = true
current_test_suite_test_case_count = 0
current_test_suite_fail_count = 0
test_suite_count = 0
test_suite_fail_count = 0
total_test_case_count = 0
total_test_case_fail_count = 0

color(7)

function fail(message)
    if not current_test_case_failed then
        current_test_case_failed = true
        current_test_suite_fail_count += 1
        total_test_case_fail_count += 1
    end
    if not current_test_suite_failed then
        current_test_suite_failed = true
        test_suite_fail_count += 1
    end
    printh(current_test_case_name .. ": " .. message)
end

function check_eq(a, b)
    if a != b then
        fail(tostr(a) .. " != " .. tostr(b))
    end
end

function save()
    memcpy(0x8000, 0, 0x5e00)
    memcpy(0xdf00, 0x5f00, 0x80)
    memcpy(0xe000, 0x6000, 0x2000)
    cls()
    reset()
end
function restore()
    poke(0x5f54, 0)
    poke(0x5f55, 0x60)
    poke(0x5f56, 0x20)
    memcpy(0, 0x8000, 0x5e00)
    memcpy(0x5f00, 0xdf00, 0x80)
    memcpy(0x6000, 0xe000, 0x2000)
end

function test_case(name, f)
    current_test_case_name = name
    current_test_case_failed = false
    current_test_suite_test_case_count += 1
    total_test_case_count += 1
    save()
    local status, err
    if pcall != nil then
        status, err = pcall(f)
    else
        f()
        status = true
    end
    if not status then
        current_test_case_failed = true
        printh(current_test_case_name .. ": " .. err)
    end
    restore()
    if current_test_case_failed then
        printh(current_test_case_name .. ": FAIL")
    else
        printh(current_test_case_name .. ": PASS")
    end
end

function test_suite(name, f)
    current_test_suite = name
    current_test_suite_failed = false
    current_test_suite_fail_count = 0
    current_test_suite_test_case_count = 0
    printh(current_test_suite .. ": START")
    local status, err
    if pcall != nil then
        status, err = pcall(f)
    else
        f()
        status = true
    end
    if not status then
        current_test_suite_failed = true
        printh(current_test_suite .. ": " .. err)
    end
    if current_test_suite_failed then
        result = "FAIL"
        print_result = "\f8fail\f7"
    else
        result = "PASS"
        print_result = "\fbpass\f7"
    end
    printh(current_test_suite .. ": " .. result .. " (" ..
        (current_test_suite_test_case_count - current_test_suite_fail_count) .. "/" ..
        current_test_suite_test_case_count .. " passed)")
    print(current_test_suite .. " " .. print_result .. " ")
    flip()
end
function summary()
    printh((total_test_case_count - total_test_case_fail_count) .. "/" ..
        total_test_case_count .. " passed")
    print((total_test_case_count - total_test_case_fail_count) .. "/" ..
        total_test_case_count .. " passed")
end

-- common setup: clear screen to 15 (so background is distinguishable),
-- reset tline-related state, reset camera/clip/palette
function tsetup()
    camera(0,0)
    clip(0,0,128,128)
    pal()
    cls(15)
    poke(0x5f38, 0)  -- mask_x default
    poke(0x5f39, 0)  -- mask_y default
    poke(0x5f3a, 0)  -- offset_x
    poke(0x5f3b, 0)  -- offset_y
    tline(13)         -- reset precision to default (tiles)
    -- clear map (rows 0-31 only; rows 32-63 share memory with sprite sheet)
    for y=0,31 do
        for x=0,127 do
            mset(x, y, 0)
        end
    end
end

--------------------------------------------------------------------------------
-- sprites (see __gfx__ section):
--   0: empty (all 0)
--   1: solid color 1
--   2: solid color 2
--   3: row gradient (row n = color n)
--   4: column gradient (col n = color n)
--   5: diagonal (pixel(x,y) = (x+y) % 8)
--   6: solid color 7 (flags=0x05 in __gff__)
--   7: solid color 8 (flags=0x02 in __gff__)
--------------------------------------------------------------------------------

function test_horiz_basic()
    test_case("horiz_solid", function()
        tsetup()
        mset(0, 0, 1)
        tline(0, 0, 7, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
        -- verify adjacent pixel not drawn
        check_eq(pget(8, 0), 15)
    end)

    test_case("horiz_two_tiles", function()
        tsetup()
        mset(0, 0, 1)
        mset(1, 0, 2)
        tline(0, 0, 15, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
        for x=8,15 do
            check_eq(pget(x, 0), 2)
        end
    end)

    test_case("horiz_col_gradient", function()
        tsetup()
        mset(0, 0, 4)
        tline(0, 0, 7, 0, 0, 0)
        -- color 0 is transparent by default; col 0 stays as background (15)
        check_eq(pget(0, 0), 15)
        for x=1,7 do
            check_eq(pget(x, 0), x)
        end
    end)
end

function test_vert_basic()
    test_case("vert_row_gradient", function()
        tsetup()
        mset(0, 0, 3)
        tline(0, 0, 0, 7, 0, 0, 0, 0.125)
        -- color 0 is transparent by default; row 0 stays as background (15)
        check_eq(pget(0, 0), 15)
        for y=1,7 do
            check_eq(pget(0, y), y)
        end
    end)

    test_case("vert_two_tiles", function()
        tsetup()
        mset(0, 0, 1)
        mset(0, 1, 2)
        tline(0, 0, 0, 15, 0, 0, 0, 0.125)
        for y=0,7 do
            check_eq(pget(0, y), 1)
        end
        for y=8,15 do
            check_eq(pget(0, y), 2)
        end
    end)
end

function test_diagonal()
    test_case("diag_45deg", function()
        tsetup()
        mset(0, 0, 5) -- sprite 5: pixel(x,y) = (x+y)%8
        tline(0, 0, 7, 7, 0, 0, 0.125, 0.125)
        -- bresenham 45°: pixels at (k,k) for k=0..7
        -- texture: mx=k/8 -> tx=k, my=k/8 -> ty=k
        -- sprite5(k,k) = (k+k)%8 = (2k)%8
        -- color 0 is transparent (k=0,4): those pixels stay as background (15)
        for k=0,7 do
            local expected = (2*k) % 8
            if expected == 0 then expected = 15 end
            check_eq(pget(k, k), expected)
        end
    end)
end

function test_shallow_steep()
    test_case("horiz_shallow_2_1", function()
        tsetup()
        mset(0, 0, 4)
        mset(1, 0, 4)
        -- 16 pixels wide, 8 pixels tall: shallow line
        -- default mdx=1/8, mdy=0 → texture only steps in x
        tline(0, 0, 15, 7, 0, 0)
        -- 16 pixels drawn. texture tx advances by 1 each pixel.
        -- verify first pixel and last pixel
        -- first pixel (0,0): tx=0, sprite4(0,0)=0, color 0 is transparent → background
        check_eq(pget(0, 0), 15)
        -- last pixel (15,7): tx=15, tile 1 col 7 -> sprite4 col 7 = 7
        check_eq(pget(15, 7), 7)
        -- a mid-pixel (8,4): tx=8 -> tile 1 col 0 = 0
        -- note: for bresenham (0,0)→(15,7), pixel 8 might not be at y=4
        -- exact bresenham path is complex; just verify endpoints
    end)

    test_case("vert_steep_1_2", function()
        tsetup()
        mset(0, 0, 3)
        mset(0, 1, 3)
        -- 8 wide, 16 tall
        tline(0, 0, 7, 15, 0, 0, 0, 0.125)
        -- mdx=0, mdy=1/8, so ty advances +1 pixel per pixel
        -- first (0,0): ty=0, sprite3 row 0 = 0, color 0 is transparent → background
        check_eq(pget(0, 0), 15)
        -- last (7,15): ty=15, tile row 1, sprite3 row 7 = 7
        check_eq(pget(7, 15), 7)
    end)
end

function test_nonzero_start()
    test_case("nonzero_mx", function()
        tsetup()
        mset(0, 0, 4) -- col gradient
        -- start at mx=0.5 tiles = pixel 4 of sprite
        tline(0, 0, 3, 0, 0.5, 0)
        check_eq(pget(0, 0), 4)
        check_eq(pget(1, 0), 5)
        check_eq(pget(2, 0), 6)
        check_eq(pget(3, 0), 7)
    end)

    test_case("nonzero_my", function()
        tsetup()
        mset(0, 0, 3) -- row gradient
        -- start at my=0.5 tiles = row 4 of sprite
        tline(0, 0, 0, 3, 0, 0.5, 0, 0.125)
        check_eq(pget(0, 0), 4)
        check_eq(pget(0, 1), 5)
        check_eq(pget(0, 2), 6)
        check_eq(pget(0, 3), 7)
    end)
end

function test_scaling()
    test_case("scale_up_half_mdx", function()
        tsetup()
        mset(0, 0, 4) -- col gradient
        -- mdx=1/16: half speed → each texel sampled twice
        tline(0, 0, 15, 0, 0, 0, 1/16, 0)
        -- tx = floor(k/2) for k=0..15
        -- color 0 is transparent: k=0,1 map to tx=0 (color 0) → background (15)
        check_eq(pget(0, 0), 15)
        check_eq(pget(1, 0), 15)
        for k=2,15 do
            check_eq(pget(k, 0), flr(k/2))
        end
    end)

    test_case("scale_down_double_mdx", function()
        tsetup()
        mset(0, 0, 4)
        mset(1, 0, 4)
        -- mdx=1/4: double speed → skip every other texel
        tline(0, 0, 7, 0, 0, 0, 0.25, 0)
        -- tx = k*2, wraps into second tile (also sprite 4)
        -- color 0 is transparent: k=0,4 give tx=0 (color 0) → background (15)
        for k=0,7 do
            local expected = (k * 2) % 8
            if expected == 0 then expected = 15 end
            check_eq(pget(k, 0), expected)
        end
    end)
end

function test_horiz_with_mdy()
    test_case("horiz_with_mdy", function()
        tsetup()
        mset(0, 0, 5) -- diagonal sprite: pixel(x,y) = (x+y)%8
        tline(0, 0, 7, 0, 0, 0, 0.125, 0.125)
        -- tx=k, ty=k, sprite5(k,k)=(2k)%8
        -- color 0 is transparent (k=0,4): those pixels stay as background (15)
        for k=0,7 do
            local expected = (2*k) % 8
            if expected == 0 then expected = 15 end
            check_eq(pget(k, 0), expected)
        end
    end)
end

function test_mask_wrapping()
    test_case("mask_wrap_x", function()
        tsetup()
        mset(0, 0, 1)  -- solid 1
        mset(1, 0, 2)  -- solid 2
        poke(0x5f38, 1) -- mask: wrap every 1 tile
        tline(0, 0, 15, 0, 0, 0)
        -- mx wraps at 1.0 tile → always within tile (0,0) = sprite 1
        for x=0,15 do
            check_eq(pget(x, 0), 1)
        end
    end)

    test_case("mask_wrap_y", function()
        tsetup()
        mset(0, 0, 1)  -- solid 1
        mset(0, 1, 2)  -- solid 2
        poke(0x5f39, 1) -- mask: wrap every 1 tile in y
        tline(0, 0, 0, 15, 0, 0, 0, 0.125)
        -- my wraps at 1.0 tile → always within tile (0,0) = sprite 1
        for y=0,15 do
            check_eq(pget(0, y), 1)
        end
    end)

    test_case("mask_wrap_x_2tiles", function()
        tsetup()
        mset(0, 0, 1)
        mset(1, 0, 2)
        mset(2, 0, 7) -- sprite 7: solid 8 (should not appear)
        poke(0x5f38, 2) -- wrap every 2 tiles
        tline(0, 0, 23, 0, 0, 0)
        -- pixels 0-7: tile 0 = sprite 1 = color 1
        -- pixels 8-15: tile 1 = sprite 2 = color 2
        -- pixels 16-23: wrapped back to tile 0 = sprite 1 = color 1
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
        for x=8,15 do
            check_eq(pget(x, 0), 2)
        end
        for x=16,23 do
            check_eq(pget(x, 0), 1)
        end
    end)
end

function test_offset()
    test_case("offset_x", function()
        tsetup()
        mset(0, 0, 1)  -- we don't sample this
        mset(2, 0, 2)  -- solid 2
        poke(0x5f3a, 2) -- offset_x = 2 tiles
        tline(0, 0, 7, 0, 0, 0)
        -- tx = (mx >>13) + 2*8 → starts at pixel 16 → tile 2
        for x=0,7 do
            check_eq(pget(x, 0), 2)
        end
    end)

    test_case("offset_y", function()
        tsetup()
        mset(0, 0, 1)
        mset(0, 2, 2)
        poke(0x5f3b, 2) -- offset_y = 2 tiles
        tline(0, 0, 0, 7, 0, 0, 0, 0.125)
        for y=0,7 do
            check_eq(pget(0, y), 2)
        end
    end)
end

function test_layers()
    test_case("layer_match", function()
        tsetup()
        mset(0, 0, 6) -- sprite 6: solid 7, flags=0x05
        tline(0, 0, 7, 0, 0, 0, 0.125, 0, 0x5)
        for x=0,7 do
            check_eq(pget(x, 0), 7)
        end
    end)

    test_case("layer_no_match", function()
        tsetup()
        mset(0, 0, 7) -- sprite 7: solid 8, flags=0x02
        tline(0, 0, 7, 0, 0, 0, 0.125, 0, 0x5)
        -- (0x5 & 0x2) = 0 ≠ 0x5 → not drawn
        for x=0,7 do
            check_eq(pget(x, 0), 15) -- background
        end
    end)

    test_case("layer_zero_draws_all", function()
        tsetup()
        mset(0, 0, 7) -- sprite 7: solid 8, flags=0x02
        tline(0, 0, 7, 0, 0, 0)
        -- no layer arg → layer=0 → draws all
        for x=0,7 do
            check_eq(pget(x, 0), 8)
        end
    end)

    test_case("layer_partial_match", function()
        tsetup()
        mset(0, 0, 6) -- flags=0x05
        mset(1, 0, 7) -- flags=0x02
        tline(0, 0, 15, 0, 0, 0, 0.125, 0, 0x1)
        -- sprite 6: (0x1 & 0x05) == 0x1 → match → color 7
        -- sprite 7: (0x1 & 0x02) = 0x0 ≠ 0x1 → no match → bg 15
        for x=0,7 do
            check_eq(pget(x, 0), 7)
        end
        for x=8,15 do
            check_eq(pget(x, 0), 15)
        end
    end)
end

function test_sprite_zero()
    test_case("sprite_zero_not_drawn", function()
        tsetup()
        mset(0, 0, 0) -- empty
        tline(0, 0, 7, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 15) -- background
        end
    end)

    test_case("sprite_zero_gap", function()
        tsetup()
        mset(0, 0, 1) -- solid 1
        mset(1, 0, 0) -- empty
        mset(2, 0, 2) -- solid 2
        tline(0, 0, 23, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
        for x=8,15 do
            check_eq(pget(x, 0), 15) -- gap
        end
        for x=16,23 do
            check_eq(pget(x, 0), 2)
        end
    end)

    test_case("sprite_zero_draw_enable", function()
        tsetup()
        -- sprite 0 is all color 0. With bit 3 of 0x5f36 set, sprite 0 is rendered
        -- (not skipped). We also need palt(0,false) so color 0 is opaque, otherwise
        -- the transparent draw palette would hide the drawn pixels regardless.
        mset(0, 0, 0)
        palt(0, false) -- make color 0 opaque so drawn pixels are visible
        poke(0x5f36, 0x8) -- enable sprite 0 rendering
        tline(0, 0, 7, 0, 0, 0)
        -- sprite 0 is all color 0; with palt(0,false), pixels should be 0
        for x=0,7 do
            check_eq(pget(x, 0), 0)
        end
    end)
end

function test_precision()
    test_case("precision_16_pixels", function()
        tsetup()
        tline(16) -- set precision to 16 (pixel coords)
        mset(0, 0, 4) -- col gradient
        -- at precision 16, coords are in pixels. mdx=1 means 1 pixel/step.
        tline(0, 0, 7, 0, 0, 0, 1, 0)
        -- color 0 is transparent: x=0 gets tx=0 (color 0) → background (15)
        check_eq(pget(0, 0), 15)
        for x=1,7 do
            check_eq(pget(x, 0), x)
        end
    end)

    test_case("precision_reset_13", function()
        tsetup()
        tline(16) -- set to pixel precision
        tline(13) -- back to tile precision
        mset(0, 0, 4)
        tline(0, 0, 7, 0, 0, 0)
        -- default mdx=1/8 at tile precision = 1 pixel/step
        -- color 0 is transparent: x=0 gets tx=0 (color 0) → background (15)
        check_eq(pget(0, 0), 15)
        for x=1,7 do
            check_eq(pget(x, 0), x)
        end
    end)
end

function test_default_mdx()
    test_case("default_mdx_mdy", function()
        tsetup()
        mset(0, 0, 4) -- col gradient
        -- 6 args: omit mdx/mdy → defaults to 1/8, 0
        tline(0, 0, 7, 0, 0, 0)
        -- color 0 is transparent: x=0 gets tx=0 (color 0) → background (15)
        check_eq(pget(0, 0), 15)
        for x=1,7 do
            check_eq(pget(x, 0), x)
        end
    end)
end

function test_edge_cases()
    test_case("single_pixel", function()
        tsetup()
        mset(0, 0, 1) -- solid 1
        tline(5, 5, 5, 5, 0, 0)
        check_eq(pget(5, 5), 1)
        check_eq(pget(4, 5), 15)
        check_eq(pget(6, 5), 15)
        check_eq(pget(5, 4), 15)
        check_eq(pget(5, 6), 15)
    end)

    test_case("reverse_horiz", function()
        tsetup()
        mset(0, 0, 4) -- col gradient
        tline(7, 0, 0, 0, 0, 0)
        -- draws right to left: pixel (7,0) gets tx=0, pixel (6,0) gets tx=1,...
        -- color 0 is transparent: k=0 (pixel (7,0)) stays as background (15)
        check_eq(pget(7, 0), 15)
        for k=1,7 do
            check_eq(pget(7-k, 0), k)
        end
    end)

    test_case("reverse_vert", function()
        tsetup()
        mset(0, 0, 3) -- row gradient
        tline(0, 7, 0, 0, 0, 0, 0, 0.125)
        -- draws bottom to top: pixel (0,7) gets ty=0, (0,6) gets ty=1,...
        -- color 0 is transparent: k=0 (pixel (0,7)) stays as background (15)
        check_eq(pget(0, 7), 15)
        for k=1,7 do
            check_eq(pget(0, 7-k), k)
        end
    end)

    test_case("off_screen_clipped", function()
        tsetup()
        mset(0, 0, 1)
        clip(2, 0, 4, 128)
        tline(0, 0, 7, 0, 0, 0)
        -- only x=2..5 should be drawn (clip x0=2, x1=2+4=6)
        -- reset clip before pget so reads aren't restricted by clip region
        clip()
        check_eq(pget(0, 0), 15)
        check_eq(pget(1, 0), 15)
        check_eq(pget(2, 0), 1)
        check_eq(pget(3, 0), 1)
        check_eq(pget(4, 0), 1)
        check_eq(pget(5, 0), 1)
        check_eq(pget(6, 0), 15)
        check_eq(pget(7, 0), 15)
    end)

    test_case("camera_offset", function()
        tsetup()
        mset(0, 0, 1) -- solid 1
        camera(-10, -5)
        tline(0, 0, 7, 0, 0, 0)
        -- tline draws at screen coords (0,0)-(7,0)
        -- camera(-10,-5) shifts drawing by +10,+5
        -- so framebuffer pixels at (10,5)-(17,5) should be drawn
        -- reset camera before pget so reads go to absolute framebuffer coords
        camera(0, 0)
        for x=10,17 do
            check_eq(pget(x, 5), 1)
        end
        check_eq(pget(9, 5), 15)
        check_eq(pget(18, 5), 15)
    end)

    test_case("fillp_no_effect", function()
        -- DRAWTYPE_DEFAULT ignores fill pattern: tline should draw
        -- every pixel regardless of fillp setting.
        tsetup()
        mset(0, 0, 1) -- solid 1
        fillp(0b1010101001010101)
        tline(0, 0, 7, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
    end)
end

function test_negative_coords()
    -- x0 < 0: line starts off-screen left
    test_case("neg_x0", function()
        tsetup()
        mset(0, 0, 1)
        mset(1, 0, 1)
        tline(-4, 0, 3, 0, 0, 0)
        -- pixels at x=0..3 should be drawn (clipped left),
        -- texture advances from the start so visible pixels
        -- sample partway into the texture
        for x=0,3 do
            check_eq(pget(x, 0), 1)
        end
        check_eq(pget(4, 0), 15) -- not drawn
    end)

    -- y0 < 0: line starts off-screen top
    test_case("neg_y0", function()
        tsetup()
        mset(0, 0, 1)
        tline(0, -4, 0, 3, 0, 0, 0, 0.125)
        for y=0,3 do
            check_eq(pget(0, y), 1)
        end
        check_eq(pget(0, 4), 15)
    end)

    -- x1 < 0: line ends off-screen left (nothing visible)
    test_case("neg_x1", function()
        tsetup()
        mset(0, 0, 1)
        tline(0, 0, -5, 0, 0, 0)
        -- starts at (0,0) which is on-screen, then goes left
        -- pixel at (0,0) should be drawn
        check_eq(pget(0, 0), 1)
    end)

    -- y1 < 0: line ends off-screen top (nothing visible)
    test_case("neg_y1", function()
        tsetup()
        mset(0, 0, 1)
        tline(0, 0, 0, -5, 0, 0, 0, 0.125)
        check_eq(pget(0, 0), 1)
    end)

    -- both endpoints negative
    test_case("both_neg", function()
        tsetup()
        mset(0, 0, 1)
        tline(-10, -10, -1, -1, 0, 0)
        -- entirely off-screen: background untouched
        for x=0,7 do
            check_eq(pget(x, 0), 15)
        end
    end)
end

function test_offscreen_coords()
    -- x0 >= 128: starts off-screen right
    test_case("x0_past_right", function()
        tsetup()
        mset(0, 0, 1)
        tline(130, 0, 125, 0, 0, 0)
        -- starts at 130 (off-screen), walks left to 125
        -- pixels 125..127 should be drawn
        check_eq(pget(124, 0), 15)
        check_eq(pget(125, 0), 1)
        check_eq(pget(126, 0), 1)
        check_eq(pget(127, 0), 1)
    end)

    -- y0 >= 128: starts off-screen bottom
    test_case("y0_past_bottom", function()
        tsetup()
        mset(0, 0, 1)
        tline(0, 130, 0, 125, 0, 0, 0, 0.125)
        check_eq(pget(0, 124), 15)
        check_eq(pget(0, 125), 1)
        check_eq(pget(0, 126), 1)
        check_eq(pget(0, 127), 1)
    end)

    -- x1 >= 128: ends off-screen right
    test_case("x1_past_right", function()
        tsetup()
        mset(0, 0, 1)
        mset(1, 0, 1)
        mset(2, 0, 1)
        tline(124, 0, 132, 0, 0, 0)
        -- pixels 124..127 drawn, rest clipped
        for x=124,127 do
            check_eq(pget(x, 0), 1)
        end
    end)

    -- y1 >= 128: ends off-screen bottom
    test_case("y1_past_bottom", function()
        tsetup()
        mset(0, 0, 1)
        tline(0, 124, 0, 132, 0, 0, 0, 0.125)
        for y=124,127 do
            check_eq(pget(0, y), 1)
        end
    end)

    -- entirely off-screen right
    test_case("entirely_offscreen_right", function()
        tsetup()
        mset(0, 0, 1)
        tline(130, 0, 140, 0, 0, 0)
        -- nothing visible
        check_eq(pget(127, 0), 15)
    end)
end

function test_negative_texture()
    -- mx < 0: negative starting texture x
    test_case("neg_mx", function()
        tsetup()
        mset(0, 0, 4) -- col gradient
        -- start at mx=-0.125 (= -1 pixel), advance right
        -- first pixel: tx=-1, wraps or clamps depending on impl
        -- after 1 step, tx=0 (color 0 = transparent), tx=1 (color 1)
        tline(0, 0, 9, 0, -0.125, 0)
        -- pixel 1 should be tx=0 (transparent), pixel 2 tx=1
        check_eq(pget(1, 0), 15) -- tx=0, transparent
        check_eq(pget(2, 0), 1)  -- tx=1
    end)

    -- my < 0: negative starting texture y
    test_case("neg_my", function()
        tsetup()
        mset(0, 0, 3) -- row gradient
        tline(0, 0, 0, 9, 0, -0.125, 0, 0.125)
        -- pixel 1: ty=0 (transparent), pixel 2: ty=1
        check_eq(pget(0, 1), 15) -- ty=0, transparent
        check_eq(pget(0, 2), 1)  -- ty=1
    end)
end

function test_negative_step()
    -- mdx < 0: texture walks backwards
    test_case("neg_mdx", function()
        tsetup()
        mset(0, 0, 4) -- col gradient
        -- start at mx=0.875 (= pixel 7), step -1/8
        tline(0, 0, 7, 0, 0.875, 0, -0.125, 0)
        -- pixel 0: tx=7, pixel 1: tx=6, ..., pixel 7: tx=0 (transparent)
        check_eq(pget(0, 0), 7)
        check_eq(pget(1, 0), 6)
        check_eq(pget(6, 0), 1)
        check_eq(pget(7, 0), 15) -- tx=0, transparent
    end)

    -- mdy < 0: texture walks backwards in y
    test_case("neg_mdy", function()
        tsetup()
        mset(0, 0, 3) -- row gradient
        -- start at my=0.875 (= row 7), step -1/8
        tline(0, 0, 0, 7, 0, 0.875, 0, -0.125)
        check_eq(pget(0, 0), 7)
        check_eq(pget(0, 1), 6)
        check_eq(pget(0, 6), 1)
        check_eq(pget(0, 7), 15) -- ty=0, transparent
    end)

    -- both negative: diagonal sampling backwards
    test_case("neg_mdx_mdy", function()
        tsetup()
        mset(0, 0, 5) -- diagonal sprite
        -- start at (0.875, 0.875), step (-1/8, -1/8)
        tline(0, 0, 7, 0, 0.875, 0.875, -0.125, -0.125)
        -- pixel k: tx=7-k, ty=7-k, sprite5(7-k,7-k) = (2*(7-k))%8
        -- k=0: (14)%8=6, k=1: (12)%8=4, k=2: (10)%8=2, k=3:8%8=0(transp)
        check_eq(pget(0, 0), 6)
        check_eq(pget(1, 0), 4)
        check_eq(pget(2, 0), 2)
        check_eq(pget(3, 0), 15) -- (2*4)%8=0, transparent
    end)
end

function test_precision_17()
    test_case("precision_17", function()
        tsetup()
        tline(17) -- set precision to 17
        mset(0, 0, 4) -- col gradient
        -- at precision 17, texture coords are in half-pixels
        -- mdx=2 means 1 pixel/step at this precision
        tline(0, 0, 7, 0, 0, 0, 2, 0)
        -- tx = k*2 >> 17... let's just check the pattern
        check_eq(pget(0, 0), 15) -- tx=0, color 0 transparent
        for x=1,7 do
            check_eq(pget(x, 0), x)
        end
    end)

    test_case("precision_17_fine", function()
        tsetup()
        tline(17)
        mset(0, 0, 4)
        -- mdx=1: half a pixel per step → each texel sampled twice
        tline(0, 0, 15, 0, 0, 0, 1, 0)
        check_eq(pget(0, 0), 15) -- tx=0, transparent
        check_eq(pget(1, 0), 15) -- tx=0, transparent
        check_eq(pget(2, 0), 1)
        check_eq(pget(3, 0), 1)
        check_eq(pget(4, 0), 2)
        check_eq(pget(5, 0), 2)
    end)
end

function test_combined_edge()
    -- negative coords + negative mdx
    test_case("neg_coords_neg_mdx", function()
        tsetup()
        mset(0, 0, 4)
        -- start off-screen left at x=-3, go to x=4
        -- texture starts at mx=0.875, steps -1/8
        tline(-3, 0, 4, 0, 0.875, 0, -0.125, 0)
        -- pixel -3: tx=7 (clipped)
        -- pixel -2: tx=6 (clipped)
        -- pixel -1: tx=5 (clipped)
        -- pixel 0: tx=4
        -- pixel 1: tx=3
        -- pixel 2: tx=2
        -- pixel 3: tx=1
        -- pixel 4: tx=0 (transparent)
        check_eq(pget(0, 0), 4)
        check_eq(pget(1, 0), 3)
        check_eq(pget(2, 0), 2)
        check_eq(pget(3, 0), 1)
        check_eq(pget(4, 0), 15) -- tx=0, transparent
    end)

    -- off-screen right + negative texture x
    test_case("offscreen_right_neg_mx", function()
        tsetup()
        mset(0, 0, 4)
        -- draw from 124 past edge
        tline(124, 0, 132, 0, -0.5, 0)
        -- tx starts at -4, then -3, -2, -1, 0, 1, 2, 3
        -- only x 124..127 visible; tx values: -4,-3,-2,-1
        -- negative tx: no map tile at negative coords → bg
        -- this verifies negative texture coords don't crash
        -- just check it doesn't blow up and visible pixels are sane
        -- pixel 127: tx=-1 or wrapping behavior
        -- At minimum: no crash, and pixels >= 128 untouched
        check_eq(pget(123, 0), 15)
    end)

    -- precision 17 + negative texture
    test_case("precision_17_neg_mx", function()
        tsetup()
        tline(17)
        mset(0, 0, 4)
        -- negative starting mx with precision 17
        tline(0, 0, 3, 0, -4, 0, 2, 0)
        -- texture starts negative, walks positive
        -- tx: -2, -1, 0, 1; pixel 2 has tx=0 (transparent)
        check_eq(pget(2, 0), 15) -- tx=0, transparent
        check_eq(pget(3, 0), 1)  -- tx=1
    end)

    -- large screen coords crossing both edges
    test_case("cross_screen", function()
        tsetup()
        for i=0,15 do
            mset(i, 0, 1)
        end
        -- line from far left to far right; offset mx so
        -- visible pixels (0..127) map to tiles 0..15
        tline(-64, 0, 191, 0, -8, 0)
        check_eq(pget(0, 0), 1)
        check_eq(pget(64, 0), 1)
        check_eq(pget(127, 0), 1)
    end)

    -- fractional mdx in normal 0..1 range
    test_case("fractional_mdx", function()
        tsetup()
        mset(0, 0, 4)
        -- mdx = 0.0625 = 1/16: half-pixel advance per step
        tline(0, 0, 15, 0, 0, 0, 0.0625, 0)
        -- tx = floor(k * 0.5) for k=0..15
        check_eq(pget(0, 0), 15) -- tx=0, transparent
        check_eq(pget(1, 0), 15) -- tx=0, transparent
        check_eq(pget(2, 0), 1)
        check_eq(pget(3, 0), 1)
        check_eq(pget(4, 0), 2)
    end)

    -- fractional mdy in normal 0..1 range
    test_case("fractional_mdy", function()
        tsetup()
        mset(0, 0, 3) -- row gradient
        -- mdy = 0.0625 = 1/16: half-pixel y advance per step
        tline(0, 0, 0, 15, 0, 0, 0, 0.0625)
        check_eq(pget(0, 0), 15) -- ty=0, transparent
        check_eq(pget(0, 1), 15) -- ty=0, transparent
        check_eq(pget(0, 2), 1)
        check_eq(pget(0, 3), 1)
        check_eq(pget(0, 4), 2)
    end)
end

-- mask/offset tests modeled on real POOM register values
function test_mask_offset_poom()
    -- poom pattern: mask=(8,8), offset=(8,8)
    -- wraps at 8x8 tiles, looks up from tile (8,8) in map
    test_case("mask8_offset8", function()
        tsetup()
        -- put sprite 1 at map pos (8,8)
        mset(8, 8, 1)
        poke(0x5f38, 8) -- mask_w = 8
        poke(0x5f39, 8) -- mask_h = 8
        poke(0x5f3a, 8) -- offset_x = 8
        poke(0x5f3b, 8) -- offset_y = 8
        -- texture at (0,0) → wrap to tile 0 → offset → map(8,8)
        tline(0, 0, 7, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
    end)

    -- poom pattern: mask=(2,2), offset=(0,0)
    -- wraps every 2x2 tiles (floor/ceiling)
    test_case("mask2_wrap", function()
        tsetup()
        mset(0, 0, 1)
        mset(1, 0, 2)
        poke(0x5f38, 2)
        -- draw 3 tiles worth; third should wrap back to tile 0
        tline(0, 0, 23, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
        for x=8,15 do
            check_eq(pget(x, 0), 2)
        end
        for x=16,23 do
            check_eq(pget(x, 0), 1)
        end
    end)

    -- poom pattern: mask=(2,2), offset=(0,2)
    test_case("mask2_offset_y2", function()
        tsetup()
        -- put sprite 2 at map pos (0,2)
        mset(0, 2, 2)
        poke(0x5f38, 2) -- mask_w = 2
        poke(0x5f39, 2) -- mask_h = 2
        poke(0x5f3b, 2) -- offset_y = 2
        -- texture at (0,0) → wrap → offset → map(0,2)
        tline(0, 0, 7, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 2)
        end
    end)

    -- poom pattern: mask=(8,8), offset=(2,0)
    test_case("mask8_offset_x2", function()
        tsetup()
        mset(2, 0, 1) -- sprite 1 at map(2,0)
        mset(3, 0, 2) -- sprite 2 at map(3,0)
        poke(0x5f38, 8)
        poke(0x5f3a, 2) -- offset_x = 2
        -- texture tile 0 → offset → map(2,0) = sprite 1
        -- texture tile 1 → offset → map(3,0) = sprite 2
        tline(0, 0, 15, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
        for x=8,15 do
            check_eq(pget(x, 0), 2)
        end
    end)

    -- poom: mask=(16,8), offset=(94,8)
    -- large offsets — tile wraps at 16 tiles, offset deep into map
    test_case("mask16_offset94", function()
        tsetup()
        -- put sprite 1 at map pos (94, 8)
        mset(94, 8, 1)
        poke(0x5f38, 16) -- mask_w = 16
        poke(0x5f39, 8)  -- mask_h = 8
        poke(0x5f3a, 94) -- offset_x = 94
        poke(0x5f3b, 8)  -- offset_y = 8
        tline(0, 0, 7, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
    end)

    -- verify wrap + offset: mx goes past mask boundary
    test_case("mask8_wrap_with_offset", function()
        tsetup()
        -- map(8+0, 0) = sprite 1, map(8+1, 0) = sprite 2
        mset(8, 0, 1)
        mset(9, 0, 2)
        poke(0x5f38, 8) -- wrap at 8 tiles
        poke(0x5f3a, 8) -- offset_x = 8
        -- mx starts at 7 tiles, advances 1 tile/8px
        -- tile 7 → offset → map(15,0) = empty
        -- tile 8 wraps to 0 → offset → map(8,0) = sprite 1
        tline(0, 0, 15, 0, 7, 0)
        -- first 8 pixels: tile 7, map(15,0) = empty → background
        for x=0,7 do
            check_eq(pget(x, 0), 15)
        end
        -- next 8 pixels: tile 8→wraps to 0, map(8,0) = sprite 1
        for x=8,15 do
            check_eq(pget(x, 0), 1)
        end
    end)

    -- poom critical pattern: precision 17 + mask 8 + offset 8
    -- at precision 17, 1 pixel = 2 tex units, 1 tile = 16 tex units
    -- mask must account for precision to wrap correctly
    test_case("p17_mask8_offset8", function()
        tsetup()
        tline(17)
        mset(8, 0, 4)  -- col gradient at map(8,0)
        mset(9, 0, 2)  -- solid 2 at map(9,0)
        poke(0x5f38, 8) -- mask = 8 tiles
        poke(0x5f3a, 8) -- offset = 8
        -- mdx=2 at precision 17 = 1 pixel/step
        tline(0, 0, 15, 0, 0, 0, 2, 0)
        -- first 8 pixels: tile 0 → map(8,0) = sprite 4 col gradient
        check_eq(pget(0, 0), 15) -- col 0 transparent
        check_eq(pget(1, 0), 1)
        check_eq(pget(4, 0), 4) -- key: must not wrap early
        check_eq(pget(7, 0), 7)
        -- next 8 pixels: tile 1 → map(9,0) = sprite 2 solid
        for x=8,15 do
            check_eq(pget(x, 0), 2)
        end
    end)

    -- precision 17, mask wrapping past boundary
    test_case("p17_mask2_wrap", function()
        tsetup()
        tline(17)
        mset(0, 0, 1)
        mset(1, 0, 2)
        poke(0x5f38, 2) -- 2-tile wrap
        -- 3 tiles at precision 17: 24 pixels, mdx=2 → 1px/step
        tline(0, 0, 23, 0, 0, 0, 2, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
        for x=8,15 do
            check_eq(pget(x, 0), 2)
        end
        -- wraps back at tile 2 → tile 0
        for x=16,23 do
            check_eq(pget(x, 0), 1)
        end
    end)

    -- precision 16 + mask
    test_case("p16_mask8", function()
        tsetup()
        tline(16)
        mset(0, 0, 4) -- col gradient
        mset(1, 0, 2) -- solid 2
        poke(0x5f38, 8) -- mask 8 tiles
        -- at precision 16: 1 unit = 1 pixel, mdx=1 → 1px/step
        tline(0, 0, 15, 0, 0, 0, 1, 0)
        -- first 8 pixels: tile 0 = sprite 4 (col gradient)
        check_eq(pget(0, 0), 15) -- col 0 transparent
        check_eq(pget(1, 0), 1)
        check_eq(pget(4, 0), 4)
        check_eq(pget(7, 0), 7)
        -- next 8 pixels: tile 1 = sprite 2 (solid 2)
        for x=8,15 do
            check_eq(pget(x, 0), 2)
        end
    end)
end

function test_mask_subtile()
    -- verify sub-tile pixel coords preserved through mask wrap
    test_case("mask_subtile_x", function()
        tsetup()
        mset(0, 0, 4) -- column gradient sprite
        poke(0x5f38, 1) -- wrap every 1 tile
        -- mx=1.0 wraps to 0.0; draw full tile
        tline(0, 0, 7, 0, 1, 0)
        -- should see column gradient: col 0=transparent(15),1,2,...,7
        check_eq(pget(0, 0), 15)
        check_eq(pget(1, 0), 1)
        check_eq(pget(7, 0), 7)
    end)

    -- sub-tile preserved through wrap: start mid-tile
    test_case("mask_subtile_midtile", function()
        tsetup()
        mset(0, 0, 4) -- column gradient
        poke(0x5f38, 1) -- wrap every 1 tile
        -- mx=1.5 wraps to 0.5 (mid-tile, pixel 4 within tile)
        tline(0, 0, 3, 0, 1.5, 0)
        check_eq(pget(0, 0), 4) -- col 4
        check_eq(pget(1, 0), 5) -- col 5
        check_eq(pget(2, 0), 6) -- col 6
        check_eq(pget(3, 0), 7) -- col 7
    end)

    -- mask wrap in y with sub-tile precision
    test_case("mask_subtile_y", function()
        tsetup()
        mset(0, 0, 3) -- row gradient sprite
        poke(0x5f39, 1) -- wrap every 1 tile in y
        -- my=2.5 wraps to 0.5 (row 4 within tile)
        tline(0, 0, 0, 3, 0, 2.5, 0, 0.125)
        check_eq(pget(0, 0), 4)
        check_eq(pget(0, 1), 5)
        check_eq(pget(0, 2), 6)
        check_eq(pget(0, 3), 7)
    end)
end

function test_mask_offset_combined()
    -- offset should not affect sub-tile coordinates
    test_case("offset_preserves_subtile", function()
        tsetup()
        mset(5, 0, 4) -- column gradient at map(5,0)
        poke(0x5f3a, 5) -- offset_x = 5
        -- mx=0 → offset → map(5,0) = sprite 4 (col gradient)
        tline(0, 0, 7, 0, 0, 0)
        check_eq(pget(0, 0), 15) -- col 0, transparent
        check_eq(pget(1, 0), 1)
        check_eq(pget(4, 0), 4)
        check_eq(pget(7, 0), 7)
    end)

    -- mask + offset together
    test_case("mask4_offset10", function()
        tsetup()
        -- put distinct sprites at map(10..13, 0)
        mset(10, 0, 1) -- solid 1
        mset(11, 0, 2) -- solid 2
        mset(12, 0, 1)
        mset(13, 0, 2)
        poke(0x5f38, 4)  -- wrap at 4 tiles
        poke(0x5f3a, 10) -- offset_x = 10
        -- tex tile 0 → wrap to 0 → offset → map(10,0) = 1
        -- tex tile 1 → wrap to 1 → offset → map(11,0) = 2
        tline(0, 0, 15, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
        for x=8,15 do
            check_eq(pget(x, 0), 2)
        end
    end)

    -- wrap past mask boundary with offset
    test_case("wrap_past_mask_offset", function()
        tsetup()
        mset(10, 0, 1)
        mset(11, 0, 2)
        poke(0x5f38, 2)  -- wrap at 2 tiles
        poke(0x5f3a, 10) -- offset_x = 10
        -- tex tile 0 → map(10,0)=1, tile 1 → map(11,0)=2
        -- tex tile 2 → wraps to 0 → map(10,0)=1
        tline(0, 0, 23, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
        for x=8,15 do
            check_eq(pget(x, 0), 2)
        end
        for x=16,23 do
            check_eq(pget(x, 0), 1)
        end
    end)

    -- y offset + y mask + mdy stepping
    test_case("mask_offset_y", function()
        tsetup()
        mset(0, 5, 1)
        mset(0, 6, 2)
        poke(0x5f39, 2)  -- wrap at 2 tiles in y
        poke(0x5f3b, 5)  -- offset_y = 5
        tline(0, 0, 0, 15, 0, 0, 0, 0.125)
        for y=0,7 do
            check_eq(pget(0, y), 1)
        end
        for y=8,15 do
            check_eq(pget(0, y), 2)
        end
    end)

    -- both mask and offset in x and y simultaneously
    test_case("mask_offset_xy", function()
        tsetup()
        mset(10, 5, 1) -- sprite 1 at (10,5)
        poke(0x5f38, 2) poke(0x5f39, 2) -- mask 2x2
        poke(0x5f3a, 10) poke(0x5f3b, 5) -- offset (10,5)
        tline(0, 0, 7, 0, 0, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 1)
        end
    end)

    -- large mx values wrapping correctly
    test_case("large_mx_wrap", function()
        tsetup()
        mset(0, 0, 4) -- col gradient
        poke(0x5f38, 1) -- wrap every 1 tile
        -- mx=100 should wrap to 0 (100 mod 1 = 0)
        tline(0, 0, 7, 0, 100, 0)
        check_eq(pget(0, 0), 15) -- col 0, transparent
        check_eq(pget(1, 0), 1)
        check_eq(pget(7, 0), 7)
    end)

    -- negative mx with mask: should wrap correctly
    test_case("neg_mx_wrap", function()
        tsetup()
        mset(0, 0, 1)
        mset(1, 0, 2)
        poke(0x5f38, 2) -- wrap at 2 tiles
        -- mx = -1.0; -1 mod 2 = 1 → tile 1
        tline(0, 0, 7, 0, -1, 0)
        for x=0,7 do
            check_eq(pget(x, 0), 2)
        end
    end)
end

-- run all test suites
test_suite("horiz_basic", test_horiz_basic)
test_suite("vert_basic", test_vert_basic)
test_suite("diagonal", test_diagonal)
test_suite("shallow_steep", test_shallow_steep)
test_suite("nonzero_start", test_nonzero_start)
test_suite("scaling", test_scaling)
test_suite("horiz_mdy", test_horiz_with_mdy)
test_suite("mask_wrap", test_mask_wrapping)
test_suite("offset", test_offset)
test_suite("layers", test_layers)
test_suite("sprite_zero", test_sprite_zero)
test_suite("precision", test_precision)
test_suite("default_mdx", test_default_mdx)
test_suite("edge_cases", test_edge_cases)
test_suite("neg_coords", test_negative_coords)
test_suite("offscreen", test_offscreen_coords)
test_suite("neg_texture", test_negative_texture)
test_suite("neg_step", test_negative_step)
test_suite("precision_17", test_precision_17)
test_suite("combined_edge", test_combined_edge)
test_suite("mask_poom", test_mask_offset_poom)
test_suite("mask_subtile", test_mask_subtile)
test_suite("mask_off_combo", test_mask_offset_combined)
summary()
__gfx__
00000000111111112222222200000000012345670123456777777777888888880000000000000000000000000000000000000000000000000000000000000000
00000000111111112222222211111111012345671234567077777777888888880000000000000000000000000000000000000000000000000000000000000000
00000000111111112222222222222222012345672345670177777777888888880000000000000000000000000000000000000000000000000000000000000000
00000000111111112222222233333333012345673456701277777777888888880000000000000000000000000000000000000000000000000000000000000000
00000000111111112222222244444444012345674567012377777777888888880000000000000000000000000000000000000000000000000000000000000000
00000000111111112222222255555555012345675670123477777777888888880000000000000000000000000000000000000000000000000000000000000000
00000000111111112222222266666666012345676701234577777777888888880000000000000000000000000000000000000000000000000000000000000000
00000000111111112222222277777777012345677012345677777777888888880000000000000000000000000000000000000000000000000000000000000000
__gff__
0000000000000502000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
