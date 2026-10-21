pico-8 cartridge // http://www.pico-8.com
version 43
__lua__

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

function check_true(a)
    if not a then
        fail("false")
    end
end

function check_eq(a, b)
    if a != b then
        fail(tostring(a) .. " != " .. tostring(b))
    end
end

function check_pixel(x, y, expected_color, msg)
    local actual = pget(x, y)
    if actual != expected_color then
        local prefix = msg and (msg .. ": ") or ""
        fail(prefix .. "pixel at (" .. x .. "," .. y .. ") is " .. actual .. ", expected " .. expected_color)
    end
end

function save()
    memcpy(0x8000, 0, 0x5e00)
    memcpy(0xdf00, 0x5f00, 0x80)
    memcpy(0xe000, 0x6000, 0x2000)
    cls()
    cursor(0, 0)
    color(7)
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
    if pcall ~= nil then
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
    end
end

function test_suite(name, f)
    current_test_suite = name
    current_test_suite_failed = false
    current_test_suite_fail_count = 0
    current_test_suite_test_case_count = 0
    printh(current_test_suite .. ": START")
    local status, err
    if pcall ~= nil then
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

function test_basic_control_codes()
    test_case("newline", function()
        print("line1\nline2")
        local x, y = peek(0x5f26), peek(0x5f27)
        check_eq(y, 12)
    end)

    test_case("carriage_return", function()
        cursor(0, 0)
        print("ii\rxy\0")
        local x, y = peek(0x5f26), peek(0x5f27)
        check_eq(x, 8)
        check_eq(y, 0)
        check_pixel(0, 0, 7)
        check_pixel(4, 0, 7)
    end)

    test_case("tab", function()
        print("a\tb\0")
        local x = peek(0x5f26)
        check_eq(x, 20)
    end)

    test_case("backspace", function()
        cursor(10, 0)
        print("\babc\0")
        local x = peek(0x5f26)
        check_eq(x, 18)
    end)

    test_case("nul", function()
        print("abc\0def")
        check_pixel(12, 2, 0)
    end)
end

function test_color_control()
    test_case("foreground_color", function()
        print("\fca\0")
        check_pixel(0, 0, 12)
        check_pixel(1, 0, 12)
        check_pixel(2, 0, 12)
        check_pixel(1, 2, 12)
        check_pixel(0, 3, 12)
        check_pixel(2, 4, 12)
    end)

    test_case("background_color", function()
        print("\#ca\0")
        check_pixel(3, 0, 12)
        check_pixel(0, 1, 7)
        check_pixel(1, 1, 12)
        check_pixel(3, 5, 12)
        check_pixel(0, 5, 12)
        check_pixel(1, 5, 12)
    end)

    test_case("all_colors", function()
        local hex = "0123456789abcdef"
        for i=0,15 do
            cursor(0, i*6)
            local h = sub(hex, i+1, i+1)
            print("\f"..h.."x\0")
        end

        check_pixel(0, 12, 2)
        check_pixel(0, 30, 5)
        check_pixel(2, 42, 7)
        check_pixel(2, 60, 10)
    end)
end

function test_cursor_control()
    test_case("horizontal_shift", function()
        cursor(10, 10)
        print("\-8a\0")
        local x = peek(0x5f26)
        check_eq(x, 6)
    end)

    test_case("vertical_shift", function()
        cursor(10, 10)
        print("\|ca\0")
        local y = peek(0x5f27)
        check_eq(y, 6)
    end)

    test_case("xy_shift", function()
        cursor(10, 10)
        print("\+88a\0")
        local x = peek(0x5f26)
        check_eq(x, 6)
    end)
end

function test_repeat()
    test_case("repeat_char", function()
        print("\*3a\0")
        local x = peek(0x5f26)
        check_eq(x, 12)
    end)
end

function test_special_commands()
    test_case("clear_screen", function()
        rectfill(0, 0, 127, 127, 8)
        print("\^c0\0")
        check_pixel(64, 64, 0)
        local x, y = peek(0x5f26), peek(0x5f27)
        check_eq(x, 0)
        check_eq(y, 0)
    end)

    test_case("jump_absolute", function()
        print("\^j48a\0")
        local x, y = peek(0x5f26), peek(0x5f27)
        check_eq(x, 20)
        check_eq(y, 32)
    end)

    test_case("home_set", function()
        -- TODO: can't figure out the behaviour of \^g and \^h .
    end)

    test_case("fill_command", function()
        cursor(10, 10)
        print("\^f5\0")
        local x = peek(0x5f26)
        check_eq(x, 14)
        check_pixel(10, 10, 7)
        check_pixel(11, 12, 7)
        check_pixel(10, 14, 7)
    end)

    test_case("wrap_boundary_with_special_command_r", function()
        cursor(0, 0)
        print("\^r8abcdefghijklmnop\0")
        local y = peek(0x5f27)
        check_true(y >= 6)
        check_pixel(0, 0, 7)
        check_pixel(0, 6, 7)
    end)

    test_case("no_wrap_without_special_command_r_or_miscflag", function()
        cursor(120, 0)
        print("abcdefgh\0")
        local x, y = peek(0x5f26), peek(0x5f27)
        check_eq(y, 0)
        check_true(x > 128)
    end)

    test_case("wrap_with_miscflag", function()
        poke(0x5f36, 0x80)
        cursor(120, 0)
        print("abcdefgh\0")
        local x, y = peek(0x5f26), peek(0x5f27)
        check_true(y >= 6)
        poke(0x5f36, 0)
    end)

    test_case("character_width", function()
        cursor(0, 0)
        print("\^x8ab\0")
        local x = peek(0x5f26)
        check_eq(x, 16)
        check_pixel(0, 0, 7)
        check_pixel(8, 0, 7)

        cursor(0, 10)
        print("\^x4cd\0")
        local x2 = peek(0x5f26)
        check_eq(x2, 8)
    end)

    test_case("character_height", function()
        cursor(0, 0)
        print("\^y8a\0")
        check_pixel(0, 0, 7)
        check_pixel(0, 4, 7)

        cursor(10, 0)
        print("\^y3b\0")
        check_pixel(10, 0, 7)
        check_pixel(10, 4, 0)
    end)
end

function test_rendering_modes()
    test_case("wide_mode", function()
        print("\^wa\0")
        check_pixel(0, 0, 7)
        check_pixel(5, 0, 7)
        check_pixel(0, 1, 7)
        check_pixel(1, 2, 7)
        check_pixel(4, 3, 7)
        check_pixel(1, 4, 7)
    end)

    test_case("tall_mode", function()
        print("\^ta\0")
        check_pixel(0, 0, 7)
        check_pixel(1, 0, 7)
        check_pixel(2, 0, 7)
        check_pixel(0, 4, 7)
        check_pixel(2, 6, 7)
        check_pixel(2, 8, 7)
    end)

    test_case("pinball_mode", function()
        print("\^pa\0")
        check_pixel(1, 0, 7)
        check_pixel(5, 0, 7)
        check_pixel(1, 2, 7)
        check_pixel(5, 2, 7)
        check_pixel(1, 4, 7)
        check_pixel(5, 4, 7)
    end)

    test_case("invert_mode", function()
        print("\^ia\0")
        check_pixel(3, 0, 7)
        check_pixel(1, 1, 7)
        check_pixel(3, 1, 7)
        check_pixel(3, 2, 7)
        check_pixel(1, 3, 7)
        check_pixel(0, 5, 7)
    end)

    test_case("disable_mode", function()
        print("\^w\^-wa\0")
        local x = peek(0x5f26)
        check_eq(x, 4)
    end)

    test_case("wide_tall_no_outline", function()
        print("\^w\^tab\0")
        check_pixel(0, 0, 7)
        check_pixel(5, 0, 7)
        check_pixel(0, 9, 7)
        check_pixel(9, 0, 7)
        check_pixel(10, 4, 7)
        local x = peek(0x5f26)
        check_eq(x, 16)
    end)
end

function test_outline()
    test_case("outline_basic", function()
        print("\^o001a\0")
        check_pixel(0, 0, 7)
        check_pixel(1, 0, 7)
        check_pixel(2, 0, 7)
        check_pixel(0, 2, 7)
        check_pixel(1, 2, 7)
        check_pixel(2, 2, 7)
    end)

    test_case("outline_full", function()
        print("\f7\^ocffa\0")
        check_pixel(3, 0, 12)
        check_pixel(3, 5, 12)
        check_pixel(0, 5, 12)
        check_pixel(1, 1, 12)
        check_pixel(1, 3, 12)
        check_pixel(2, 5, 12)
    end)
end

function test_one_off_chars()
    test_case("hex_char", function()
        print("\^:aa55aa55aa55aa55\0")
        check_pixel(1, 0, 7)
        check_pixel(0, 0, 0)
        check_pixel(0, 1, 7)
        check_pixel(1, 1, 0)
        check_pixel(5, 4, 7)
        local x = peek(0x5f26)
        check_eq(x, 8)
    end)
end

function test_decoration()
    test_case("decoration_char", function()
        cursor(0, 0)
        print("ab\v5*c\0")
        local x = peek(0x5f26)
        check_eq(x, 12)
        check_pixel(0, 0, 7)
        check_pixel(4, 0, 7)
        check_pixel(2, 0, 7)

        cursor(0, 20)
        print("x\vb,y\0")
        check_eq(peek(0x5f26), 8)
        check_pixel(0, 20, 7)
        check_pixel(4, 20, 7)
        check_pixel(2, 17, 7)
        check_pixel(1, 18, 7)
    end)
end

function test_default_attributes()
    test_case("default_wide", function()
        poke(0x5f58, 0x1 | 0x4)
        print("a\0")
        check_pixel(0, 0, 7)
        check_pixel(5, 0, 7)
        check_pixel(0, 1, 7)
        check_pixel(4, 2, 7)
        check_pixel(1, 3, 7)
        check_pixel(4, 4, 7)
        poke(0x5f58, 0)
    end)

    test_case("default_tall", function()
        poke(0x5f58, 0x1 | 0x8)
        print("a\0")
        check_pixel(0, 0, 7)
        check_pixel(2, 0, 7)
        check_pixel(0, 2, 7)
        check_pixel(1, 4, 7)
        check_pixel(0, 6, 7)
        check_pixel(2, 8, 7)
        poke(0x5f58, 0)
    end)

    test_case("default_invert", function()
        poke(0x5f58, 0x1 | 0x20)
        print("a\0")
        check_pixel(3, 0, 7)
        check_pixel(1, 1, 7)
        check_pixel(3, 1, 7)
        check_pixel(3, 2, 7)
        check_pixel(1, 3, 7)
        check_pixel(0, 5, 7)
        poke(0x5f58, 0)
    end)
end

function test_custom_font()
    test_case("custom_font_switch", function()
        memset(0x5600, 0, 128 + 240*8)

        poke(0x5600, 8)
        poke(0x5601, 8)
        poke(0x5602, 8)

        local x_addr = 0x5600 + 128 + (88-16)*8
        poke(x_addr, 129, 66, 36, 24, 24, 36, 66, 129)

        cursor(0, 0)
        print("\014X\0")
        local x1 = peek(0x5f26)
        check_eq(x1, 8)
        check_pixel(0, 0, 7)
        check_pixel(7, 0, 7)
        check_pixel(3, 3, 7)

        cursor(0, 20)
        print("\015ab\0")
        local x2 = peek(0x5f26)
        check_eq(x2, 8)
        check_pixel(0, 20, 7)
        check_pixel(4, 20, 7)
    end)

    test_case("variable_width_font", function()
        memset(0x5600, 0, 128 + 240*8)

        poke(0x5600, 8, 8, 8, 0, 0, 1)

        poke(0x5600 + 36, 0x40)

        local a_addr = 0x5600 + 128 + (65-16)*8
        poke(a_addr, 24, 36, 66, 66, 126, 66, 66, 0)

        local i_addr = 0x5600 + 128 + (73-16)*8
        poke(i_addr, 126, 24, 24, 24, 24, 24, 126, 0)

        cursor(0, 0)
        print("\014AIIA\0")

        local x_final = peek(0x5f26)
        check_eq(x_final, 24)

        check_pixel(3, 0, 7)
        check_pixel(1, 2, 7)

        check_pixel(9, 0, 7)
        check_pixel(9, 6, 7)
    end)

    test_case("draw_offset", function()
        memset(0x5600, 0, 128 + 240*8)

        poke(0x5600, 8, 8, 8, 2, 3)

        local x_addr = 0x5600 + 128 + (88-16)*8
        poke(x_addr, 129, 66, 36, 24, 24, 36, 66, 129)

        cursor(0, 0)
        print("\014X\0")

        check_pixel(2, 3, 7)
        check_pixel(9, 3, 7)
        check_pixel(5, 6, 7)
    end)

    test_case("character_y_offset", function()
        memset(0x5600, 0, 128 + 240*8)

        poke(0x5600, 8, 8, 8, 0, 0, 1)

        poke(0x5600 + 32, 0x80)

        local a_addr = 0x5600 + 128 + (65-16)*8
        poke(a_addr, 24, 36, 66, 66, 126, 66, 66, 0)

        local b_addr = 0x5600 + 128 + (66-16)*8
        poke(b_addr, 63, 66, 66, 62, 66, 66, 63, 0)

        cursor(0, 0)
        print("\014AB\0")

        check_pixel(3, 0, 0)
        check_pixel(8, 0, 7)
    end)

    test_case("tab_width", function()
        memset(0x5600, 0, 128 + 240*8)

        poke(0x5600, 8, 8, 8, 0, 0, 0, 12)

        local x_addr = 0x5600 + 128 + (88-16)*8
        poke(x_addr, 129, 66, 36, 24, 24, 36, 66, 129)

        cursor(0, 0)
        print("\014X\tX\0")

        local x_final = peek(0x5f26)
        check_eq(x_final, 20)

        check_pixel(0, 0, 7)
        check_pixel(12, 0, 7)
    end)
end

function test_combined_features()
    test_case("wide_tall_outline", function()
        print("\^w\^t\^ocffa\0")
        check_pixel(0, 0, 7)
        check_pixel(5, 0, 7)
        check_pixel(6, 1, 12)
        check_pixel(0, 6, 7)
        check_pixel(5, 9, 7)
        check_pixel(3, 10, 12)
    end)

    test_case("color_background_wide", function()
        print("\f7\#8\^wa\0")
        check_pixel(0, 0, 7)
        check_pixel(6, 0, 8)
        check_pixel(7, 0, 8)
        check_pixel(2, 1, 8)
        check_pixel(1, 3, 7)
        check_pixel(0, 5, 8)
    end)
end

function test_error_handling()
    test_case("incomplete_color_code", function()
        print("a\fb")
        local x = peek(0x5f26)
        check_eq(x, 0)
        check_eq(peek(0x5f25), 11)
        check_pixel(0, 0, 7)
        check_pixel(1, 0, 7)
        check_pixel(2, 2, 7)
    end)

    test_case("incomplete_jump_code", function()
        print("a\^jb\0")
        local x, y = peek(0x5f26), peek(0x5f27)
        check_eq(x, 0)
        check_eq(y, 6)
        check_pixel(0, 0, 7)
        check_pixel(2, 0, 7)
    end)

    test_case("invalid_hex_in_color", function()
        print("\fgx\0")
        local x = peek(0x5f26)
        check_eq(x, 4)
        check_eq(peek(0x5f25), 16)
        check_pixel(0, 0, 0)
        check_pixel(1, 2, 0)
        check_pixel(3, 0, 0)
    end)
end

function bubble_text_log_out_manual(s, x, y, fg, shadow)
    print(s, x - 1, y, shadow)
    print(s, x + 1, y, shadow)
    print(s, x, y - 1, shadow)
    print(s, x, y + 1, shadow)
    print(s, x, y, fg)
end

function bubble_text_log_out_p8scii(s, x, y, fg, shadow)
    color(shadow)
    print("\-f"..s.."\^g\-h"..s.."\^g\|f"..s.."\^g\|h"..s, x, y)
    print(s, x, y, fg)
end

function test_bubble_text()
    test_case("shadow_offsets_match_manual", function()
        local text = "goo"
        local x, y = 64, 64
        local fg, shadow = 7, 0
        local expected = {}

        cls()
        bubble_text_log_out_manual(text, x, y, fg, shadow)

        for yy = 56, 72 do
            expected[yy] = {}
            for xx = 56, 76 do
                expected[yy][xx] = pget(xx, yy)
            end
        end

        cls()
        bubble_text_log_out_p8scii(text, x, y, fg, shadow)

        for yy = 56, 72 do
            for xx = 56, 76 do
                check_eq(pget(xx, yy), expected[yy][xx])
            end
        end
    end)
    test_case("goto_home_returns_to_start", function()
        cls()
        print('\-h\^g\^:0100000000000000', 30, 30, 7)
        check_pixel(30, 30, 7)
        check_pixel(31, 30, 0)
    end)

    test_case("set_home_then_goto_home", function()
        cls()
        print('\-h\^h\-h\^g\^:0100000000000000', 30, 30, 7)
        check_pixel(31, 30, 7)
        check_pixel(30, 30, 0)
        check_pixel(32, 30, 0)
    end)

    -- Shadow sequence used by super_world_of_goo log_out()
    test_case("shadow_offsets", function()
        local dot='\^:0100000000000000'
        cls()
        print('\-f'..dot..'\^g\-h'..dot..'\^g\|f'..dot..'\^g\|h'..dot, 40, 40, 7)
        check_pixel(39, 40, 7)
        check_pixel(41, 40, 7)
        check_pixel(40, 39, 7)
        check_pixel(40, 41, 7)
        check_pixel(40, 40, 0)
    end)
end

function setup_custom_font()
    poke4(unpack(split"0x5680,0x707.0707,0x.0007,0x7.0707,,0x7.0507,,5.0079,,0x5.0005,,0x5.0505,,1543.0235,0x.0004,0x307.0301,0x.0001,0x101.0107,0x.0001,1028.0157,0x.0007,0x702.0705,0x.0002,,0x.0002,256,0x.0002,768,0x.0003,0x.0505,,0x2.0502,,,,0x2.0202,0x.0002,0x.0505,,0x705.0705,0x.0005,0x706.0307,0x.0002,0x102.0405,0x.0005,0x506.0303,0x.0007,0x.0102,,0x101.0102,0x.0002,0x404.0402,0x.0002,519.0079,0x.0005,0x207.02,,512,0x.0001,7,,,0x.0002,0x202.0204,0x.0001,0x505.0507,0x.0007,0x202.0302,0x.0007,0x106.0403,0x.0007,0x406.0403,0x.0007,1797.0235,0x.0004,0x403.0107,0x.0003,1283.004,0x.0007,0x202.0407,0x.0002,0x507.0507,0x.0007,0x407.0506,0x.0003,0x200.02,,0x200.02,0x.0001,0x201.0204,0x.0004,0x700.07,,0x204.0201,0x.0001,0x6.0407,0x.0002,0x105.0502,0x.0006,0x704.03,0x.0006,0x503.0101,0x.0003,0x101.06,0x.0006,1286.0157,0x.0006,0x107.02,0x.0006,0x207.0204,0x.0002,0x605.06,0x.0003,0x503.0101,0x.0005,0x202.0002,0x.0002,0x202.0002,0x.0001,0x503.0501,0x.0005,0x202.0202,0x.0004,0x507.07,0x.0005,0x505.03,0x.0005,0x505.02,0x.0002,0x305.03,0x.0001,0x605.06,0x.0004,0x101.06,0x.0001,0x401.06,0x.0003,0x202.0702,0x.0004,0x505.05,0x.0006,0x705.05,0x.0002,0x705.05,0x.0007,0x502.05,0x.0005,0x205.05,0x.0001,0x104.07,0x.0007,0x101.0103,0x.0003,0x202.0201,0x.0004,0x202.0203,0x.0003,0x.0502,,,0x.0007,0x.0402,,0x507.0507,0x.0005,0x503.0507,0x.0007,257.004,0x.0006,0x505.0503,0x.0007,0x103.0107,0x.0007,0x103.0107,0x.0001,1281.004,0x.0007,0x507.0505,0x.0005,0x202.0207,0x.0007,0x202.0207,0x.0003,0x503.0505,0x.0005,0x101.0101,0x.0007,0x505.0707,0x.0005,0x505.0503,0x.0005,0x505.0506,0x.0003,0x107.0507,0x.0001,0x305.0502,0x.0006,0x503.0507,0x.0005,1031.004,0x.0003,0x202.0207,0x.0002,0x505.0505,0x.0006,0x705.0505,0x.0002,0x705.0505,0x.0007,0x502.0505,0x.0005,0x407.0505,0x.0007,0x102.0407,0x.0007,0x203.0206,0x.0006,0x202.0202,0x.0002,0x206.0203,0x.0003,0x107.04,,0x205.02,,257.0078,0x101.0101,0x.0001,,0xfefe.fe,0xfefe.fefe,0x101.0102,0x101.0101,-8192,0x.00e,0x202.02,0x202.0202,,-256,0x101.0101,0x101.0101,0x8080.808,0x8080.808,4112,16.0644,0x33.4433,0x.0011,115.3338,0x25.2757,37.1463,0xe2.a6ee,0xc8.9494,0x4a.4ae6,0x1c.0c1d,0x404c.e,,0x2ee4.6e,0x.0001,,0xa42e,0x.a265,0x.01fe,,0x68.08,0.2198,0x.fe,,0x.0302,,,,0x7f7f.7f,0x7f7f.7f7f,0x.007f,,0x8080.8,0x8080.808,0.0039,,0x101.01fe,0x101.0101,0x1fff.ffff,0xffff.ff1f,0x87.c7e7,0xe7c7.87,,0x.00e,,0x1820.4087,,,0x101.0101,0x101.0101,0x303.0303,0x303.0303,0x707.0707,0x707.0707,0xf0f.0f0f,0xf0f.0f0f,0x1f1f.1f1f,0x1f1f.1f1f,0x3f3f.3f3f,0x3f3f.3f3f,0x7f7f.7f7f,0x7f7f.7f7f,0xffff.ffff,0xffff.ffff,2110.1095,0x.0008,0x1c3e.0808,0x.0008,3134.047,0x.0008,0x183e.1808,0x.0008,0x6b77.6b3e,0x.003e,0x636b.633e,0x.003e,,,0x101.0101,0x.0001,,0x.fe,,0x.ff,0x8080.808,0x.7f8,0xfefe.fefe,0x.00fe,0xffff.ffff,0.0039,0x7f7f.7f7f,0x.007f,,,,23296,,23514,,0x5bda.89,,0x5bda.8991,0xd300,0x5bda.8991,0xd34a,0x5bda.8991,0xd34a.82,0x5bda.8991,0xd34a.8201,0x5bda.8991,0xffff.ffff,0xffff.ffff,0xffff.ffff,0xffff.ffff,0xffff.ffff,0x212f.e7bf,0x90ef.ffff,0x224.639d,0x92d6.ffff,0x98e1.171,0x232c.ffff,0xffff.806a,0x3f0.ffff,0xffff.ff0a,0xffff.ffff,0xffff.ffe2,0xffff.ffff,0xffff.ffff,0xffff.ffff,0xffff.ffff,0xffff.ffff,0xffff.ffff,0xffff.ffff,0xffff.ffff,0xffff.ffff,0x5fdf.ffff,0xffff.9f7f,0x58ff.ffff,0xffff.7f03,0xcbff.ffff,1793.0081,0x366d.3f1f,0xffff.ffff,0xff17.11ff,0x45a7.ffff,0xf1fb.fe1,0xed65.3e9c,0xfdfd.ffff,0xffe4.a025,0xffff.ffff,0xffff.fdfc,0xffff.ffff,0xe7f3.c7ff,0xffe7.f7f3,0xdce3.ffff,0x4f4d.c0c,0xffff.ffff,0x84fb.ffff,0xffff.ffff,0xe87f.ffff,0xffff.ffff,0xbdb.ffff,0xffff.ffff,0x6.fbbf,0xffff.ffff,0x2.152c,0x7fbf.4bff,0x2c2.42e,0x53a.0382,0xf75b.d97f,0x60.e89e,0x4c0e.0701,0x627b.2a6d,16464.3452,9911.7871,0x875f.9035,0x1061.e5da,0x3f04.8401,0xfff9.cbff,0xfcff.ffff,0xffff.ffff,0xffff.ffff,0xffff.ffff,0xffff.ffff,0xffff.ffff,0x3973.ffff,0xffff.b553,-3359.0508,0x7fef.ff7a,0xffb7.feff,0xdbff.ff17,0x7fed.bfff,0xed2d.ffb5,0xdbff.f5bf,0xf13e.0f7f,0xbeda.ff6f,0xf00.204,0x9bcb.0002,0x8781.8303,29043.8428,0x4169.3794,0xd7b7.7da1,0x120a.0201,0xd436.4749,,0xc080.02,-0.996,0xffff.ffff,0xffff.0548,0xffff.ffff,0xff0f.02e,0xffff.ffff,0x9c0.feff,0xffff.ff3f,0xc0ff.ffff,0xffff.3f,-8.0177,-1,-0.0118,0xffff.40fe,0xffbf.ffff,-0.5314,0xffff.dbff,0xffff.b7ff,0xffb7.feff,0xffff.f7ff,0xff7d.eff7,0xffff.2eef,0xfffb.2f9f,0xffff.f52e,31679.7851,-0.0029,0xc01b.95fe,0x504.01,0x3f8.1d0b,,0x12.fe3"))
    poke(0x5f58,0x81)
    poke(0x5600,4,8,8,0,0,0,0,0)
end

function test_box_drawing()
    test_case("box_drawing1", function()
        setup_custom_font()
        print("そ",0,0)
        expected={{7,7,7,7,7,7,7,7,0,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,0,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,0,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,0,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,0,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,0,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,0,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
    end)
    test_case("box_drawing2", function()
        setup_custom_font()
        print("そ⬇️",0,0)
        expected={{7,7,7,7,7,7,7,7,0,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
    end)
    test_case("box_drawing3", function()
        setup_custom_font()
        print("そ⬇️\r⁴o★",0,0)
        expected={{7,7,7,7,7,7,7,7,0,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{0,7,7,7,7,7,7,7,0,0,0,0,0,0,0,0},{7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
    end)
    test_case("box_drawing4", function()
        setup_custom_font()
        print("そ⬇️\r⁴o★…",0,0)
        expected={{7,7,7,7,7,7,7,7,0,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{0,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
    end)
    test_case("box_drawing5", function()
        setup_custom_font()
        print("そ⬇️\r⁴o★…\rᶜ7⬆️",0,0)
        expected={{7,7,7,7,7,7,7,7,0,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{0,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
    end)
    test_case("box_drawing6", function()
        setup_custom_font()
        print("そ⬇️\r⁴o★…\rᶜ7⬆️ˇ",0,0)
        expected={{7,7,7,7,7,7,7,7,0,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{0,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
    end)
    test_case("box_drawing7", function()
        setup_custom_font()
        print("そ⬇️\r⁴o★…\rᶜ7⬆️ˇ\rᶜd…",0,0)
        expected={{7,7,7,7,7,7,7,7,0,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
    end)
    test_case("box_drawing8", function()
        setup_custom_font()
        print("そ⬇️\r⁴o★…\rᶜ7⬆️ˇ\rᶜd…\rᶜd⁴8█",0,0)
        expected={{13,13,13,13,13,13,13,13,0,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
    end)
    test_case("box_drawing9", function()
        setup_custom_font()
        print("そ⬇️\r⁴o★…\rᶜ7⬆️ˇ\rᶜd…\rᶜd⁴8█",0,0)
        expected={{13,13,13,13,13,13,13,13,0,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
    end)
    test_case("box_drawing10", function()
        setup_custom_font()
        print("そ⬇️\r⁴o★…\rᶜ7⬆️ˇ\rᶜd…\rᶜd⁴8█▒",0,0)
        expected={{13,13,13,13,13,13,13,13,13,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
    end)
    test_case("box_drawing11", function()
        setup_custom_font()
        print("そ⬇️\r⁴o★…\rᶜ7⬆️ˇ\rᶜd…\rᶜd⁴8█▒\rᶜ7き",0,0)
        expected={{13,13,13,13,13,13,13,13,13,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
    end)
    test_case("box_drawing12", function()
        setup_custom_font()
        print("そ⬇️\r⁴o★…\rᶜ7⬆️ˇ\rᶜd…\rᶜd⁴8█▒\rᶜ7き✽",0,0)
        expected={{13,13,13,13,13,13,13,13,13,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{13,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,15 do
            row=expected[y+1]
            for x=0,15 do
                c=row[x+1]
                check_pixel(x, y, c)
            end
        end
        --[[
        s="{"
        for y=0,15 do
            s..="{"
            for x=0,15 do
                c=pget(x, y)
                s..=c
                if x != 15 then
                    s..=","
                end
            end
            s..="}"
            if y != 15 then
                s..=","
            end
        end
        printh(s)
        ]]--
    end)
    test_case("box_drawing_hud", function()
        setup_custom_font()
        ?"⁶@5f360001@ᶜ7⁶jff웃⁶j5t⁵fhᶜd²6█▒³a█▒³a█▒³a█▒³a█▒³a█▒³a█▒³a█▒³a█⁶-#▒³8ᶜ6⬇️⁶j7t⁵fhᶜ7✽³i✽³i✽³i✽³i✽³i✽³i✽³i✽³i✽⁶j5v⁵fhᶜd…³i…³i…³i…³i…³i…³i…³i…³i…⁶j5v⁵fhᶜ6★…³a★…³a★…³a★…³a★…³a★…³a★…³a★…³a★…⁶j5v⁵fhᶜ7⬆️ˇ³a⬆️ˇ³a⬆️ˇ³a⬆️ˇ³a⬆️ˇ³a⬆️ˇ³a⬆️ˇ³a⬆️ˇ³a⬆️ˇ"
        expected={{13,13,13,13,13,13,13,13,13,6,13,13,13,13,13,13,13,13,13,6,13,13,13,13,13,13,13,13,13,6},{13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7},{13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7},{13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7},{13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7},{13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7},{13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7},{13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7},{13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7,13,6,6,6,6,6,6,6,6,7},{6,7,7,7,7,7,7,7,7,7,6,7,7,7,7,7,7,7,7,7,6,7,7,7,7,7,7,7,7,7},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
        for y=0,10 do
            row=expected[y+1]
            for x=0,29 do
                c=row[x+1]
                check_pixel(x+19, y+117, c)
            end
        end
    end)
end

function test_return()
    test_case("width return", function()
        local w = print("a")
        check_eq(w, 4)
    end)

    test_case("height return", function()
        local w,h = print("a")
        check_eq(w, 4)
        check_eq(h, 6)
    end)

    test_case("multi-line", function()
        local w,h = print("a\nb")
        check_eq(w, 4)
        check_eq(h, 12)
    end)

    test_case("ragged line", function()
        local w,h = print("a\nbbb\nc")
        check_eq(w, 12)
        check_eq(h, 18)
    end)

    test_case("ragged line with offset", function()
        local w,h = print("a\nbbb\nc", 30, 40)
        check_eq(w, 42)
        check_eq(h, 58)
    end)
end

test_suite("basic_control_codes", test_basic_control_codes)
test_suite("color_control", test_color_control)
test_suite("cursor_control", test_cursor_control)
test_suite("repeat", test_repeat)
test_suite("special_commands", test_special_commands)
test_suite("rendering_modes", test_rendering_modes)
test_suite("outline", test_outline)
test_suite("one_off_chars", test_one_off_chars)
test_suite("decoration", test_decoration)
test_suite("default_attributes", test_default_attributes)
test_suite("custom_font", test_custom_font)
test_suite("combined_features", test_combined_features)
test_suite("error_handling", test_error_handling)
test_suite("bubble_text", test_bubble_text)
test_suite("box_drawing", test_box_drawing)
test_suite("return", test_return)

summary()