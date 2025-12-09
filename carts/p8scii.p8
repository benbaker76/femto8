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

    test_case("wrap_boundary", function()
        cursor(0, 0)
        print("\^r8abcdefghijklmnop\0")
        local y = peek(0x5f27)
        check_true(y >= 6)
        check_pixel(0, 0, 7)
        check_pixel(0, 6, 7)
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

summary()