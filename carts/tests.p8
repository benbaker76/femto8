pico-8 cartridge // http://www.pico-8.com
version 43
__lua__

-- If run() does not accept param_str, then set NO_RUN=0 below
-- or the cart will continuously loop.
NO_RUN = 1

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
-- Re-run with a parameter if started without one.
if NO_RUN == 0 then
    if stat(6) != 'myparam' then
        run("myparam")
    end
end

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

function tostr1(x)
    if x == nil then
        return "<nil>"
    elseif type(x) == "boolean" then
        if x then
            return "true"
        else
            return "false"
        end
    elseif type(x) == "table" then
        ret="{"
        for k,v in pairs(x) do
            ret ..= "["..tostr1(k).."]="..tostr1(v)..","
        end
        ret..="}"
        return ret
    elseif type(x) == "string" then
        return '"'..x..'"'
    else
        return x
    end
end
function eq(a, b)
    if type(a) == "table" then
        for ak,av in pairs(a) do
            found = false
            for bk,bv in pairs(b) do
                if ak==bk then
                    if av != bv then
                        return false
                    end
                    found = true
                end
            end
            if not found then
                return false
            end
        end
        for bk,bv in pairs(b) do
            found = false
            for ak,av in pairs(a) do
                if ak==bk then
                    found=true
                end
            end
            if not found then
                return false
            end
        end
        return true
    else
        return a == b
    end
end

function check_true(a)
    if not a then
        fail("false")
    end
end
function check_false(a)
    if a then
        fail("true")
    end
end
function check_eq(a, b)
    if not eq(a, b) then
        fail(tostr1(a) .. " != " .. tostr1(b))
    end
end
function check_ne(a, b)
    if eq(a,b) then
        fail(tostr1(a) .. " == " .. tostr1(b))
    end
end
function check_lt(a, b)
    if a >= b then
        fail(tostr1(a) .. " >= " .. tostr1(b))
    end
end
function check_le(a, b)
    if a > b then
        fail(tostr1(a) .. " > " .. tostr1(b))
    end
end
function check_gt(a, b)
    if a <= b then
        fail(tostr1(a) .. " <= " .. tostr1(b))
    end
end
function check_ge(a, b)
    if a < b then
        fail(tostr1(a) .. " < " .. tostr1(b))
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
function sleep(seconds)
    start = time()
    while time() - start < seconds and time() >= start do
        flip()
    end
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
    else
        --printh(current_test_case_name .. ": PASS")
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

--------------------------------------------------------------------------------

function test_test_framework()
    test_case("check_eq", function()check_eq(1, 1)end)
    test_case("check_eq_array", function()check_eq({1, 2, 3}, {1, 2, 3})end)
    test_case("check_eq_table", function()check_eq({["a"]=1, ["b"]=2}, {["a"]=1, ["b"]=2})end)
end

function test_hello_world()
    test_case("hello_world", function()print("hello world") rectfill(80,80,120,100,12) circfill(70,90,20,14) for i=1,4 do print(i) end end)
    test_case("pinkcirc", function()x = 64 y = 64 function _update() if (btn(0)) then local x=x-1 end if (btn(1)) then local x=x+1 end if (btn(2)) then y=y-1 end if (btn(3)) then y=y+1 end end function _draw() cls(5) circfill(x,y,7,14) end end)
end

function test_comments()
    test_case("NAMEME", function()-- use two dashes like this to write a comment --[[ multi-line comments ]]
        end)
end

function test_types_and_assignment()
    test_case("types", function()local num = 12/100 local s = "THIS IS A STRING" local b = false local t = {1,2,3}end)
    test_case("hex", function()check_eq(0x11, 17)end)
    test_case("hex_fractional", function()check_eq(0x11.4000, 17.25)end)
    test_case("tostr_hex", function()check_eq(tostr(-32768,true), "0x8000.0000")end)
    test_case("tostr_hex_fractional", function()check_eq(tostr(32767.99999,true), "0x7fff.ffff")end)
    test_case("divide_by_zero_positive", function()check_eq(1.0 / 0.0, 0x7fff.ffff)end)
    test_case("divide_by_zero_negative", function()check_eq(-1.0 / 0.0, -0x7fff.ffff)end)
end

function test_conditionals()
    test_case("if1", function()local x="" local b=false if not b then s="B IS FALSE" else s="B IS NOT FALSE" end check_eq(s, "B IS FALSE")end)
    test_case("if2", function()local x="" local b=true if not b then s="B IS FALSE" else s="B IS NOT FALSE" end check_eq(s, "B IS NOT FALSE")end)
    test_case("elseif1", function()local s="" local x=0 if x == 0 then s="X IS 0" elseif x < 0 then s="X IS NEGATIVE" else s="X IS POSITIVE" end check_eq(s, "X IS 0")end)
    test_case("elseif2", function()local s="" local x=-1 if x == 0 then s="X IS 0" elseif x < 0 then s="X IS NEGATIVE" else s="X IS POSITIVE" end check_eq(s, "X IS NEGATIVE")end)
    test_case("elseif3", function()local s="" local x=1 if x == 0 then s="X IS 0" elseif x < 0 then s="X IS NEGATIVE" else s="X IS POSITIVE" end check_eq(s, "X IS POSITIVE")end)
    test_case("equal1", function()local s="" if (4 == 4) then s="EQUAL" end check_eq(s, "EQUAL") end)
    test_case("equal2", function()local s="" if (4 == 3) then s="EQUAL" end check_eq(s, "") end)
    test_case("not_equal1", function()local s="" if (4 ~= 3) then s="NOT EQUAL" end check_eq(s, "NOT EQUAL") end)
    test_case("not_equal2", function()local s="" if (4 ~= 4) then s="NOT EQUAL" end check_eq(s, "") end)
    test_case("less_than_or_equal1", function()local s="" if (4 <= 4) then s="LESS THAN OR EQUAL" end check_eq(s, "LESS THAN OR EQUAL") end)
    test_case("less_than_or_equal2", function()local s="" if (4 <= 3) then s="LESS THAN OR EQUAL" end check_eq(s, "") end)
    test_case("more_than1", function()local s="" if (4 > 3) then s="MORE THAN" end check_eq(s, "MORE THAN") end)
    test_case("more_than2", function()local s="" if (4 > 5) then s="MORE THAN" end check_eq(s, "") end)
end

function test_loops()
    test_case("for", function()local s="" for x=1,5 do s..=x.."\n" end check_eq(s, "1\n2\n3\n4\n5\n") end)
    test_case("while", function()local s="" local x = 1 while(x <= 5) do s..=x.."\n" x = x + 1 end check_eq(s, "1\n2\n3\n4\n5\n") end)
    test_case("for_step", function()local s="" for x=1,10,3 do s..=x.."\n" end  check_eq(s, "1\n4\n7\n10\n") end)
    test_case("for_reverse", function()local s="" for x=5,1,-2 do s..=x.."\n" end check_eq(s, "5\n3\n1\n") end)
end

function test_functions_and_local_variables()
    test_case("local_variable_scope", function()y=0 function plusone(x) local y = x+1 return y end check_eq(plusone(2), 3) check_eq(y, 0) end)
end

function test_tables()
    test_case("tables", function()local a={} a[1] = "blah" a[2] = 42 a["foo"] = {1,2,3} check_eq(a[1], "blah") check_eq(a[2], 42) end)
    test_case("array_1_based_indexing", function()local a = {11,12,13,14} check_eq(a[2], 12) end)
    test_case("array_0_based_indexing", function()local a = {[0]=10,11,12,13,14} check_eq(a[0], 10) check_eq(a[2], 12) end)
    test_case("array_len", function()local a = {11,12,13,14} check_eq(#a, 4) add(a, 15) check_eq(#a, 5) end)
    test_case("dot_notation", function()local player = {} player.x = 2 check_eq(player.x, player["x"]) player.y = 3 check_eq(player.y, 3) end)
end

function test_pico_8_shorthand()
    test_case("if_shorthand1", function()local b=true local i=0 local j=0 if (not b) i=1 j=2 check_eq(i,0) check_eq(j, 0) end)
    test_case("if_shorthand2", function()local b=false local i=0 local j=0 if (not b) i=1 j=2 check_eq(i,1) check_eq(j, 2) end)
    test_case("assignment_shorthand", function()local a=0 a += 2 check_eq(a, 2) end)
    test_case("not_equals_shorthand", function()check_eq(1 != 2, true) end)
    test_case("not_equals_shorthand_intern", function()check_eq("foo", "foo") end)
end

function test_pico_8_program_structure()
    test_case("simple_program", function()function init() col = 7 end function update() if (btnp(5)) col = 8 + rnd(8) end function draw() cls() circfill(64,64,32,col) end end)
end

-- global1=81 global2=11
-- #include somecode.lua
-- #include onetab.p8:1
-- #include alltabs.p8

function test_include()
    -- #include makes no sense without a filesystem.
    --test_case("include", function()check_eq(somecode(), 91) check_eq(global1, 81) check_eq(onetab_tab1(), 20) check_eq(global2, 98) check_eq(alltabs_tab1(), 78) end)
end

function test_quirks_of_pico_8()
    test_case("number_wrap", function()local x=0x7fff.ffff x=x+0x0.0001 check_eq(x, -0x8000.0000) end)
    test_case("min_number1", function()local x=-0x8000.0000 check_eq(x, -0x8000.0000) end)
    test_case("min_number2", function()local x=-0x8000.0001 check_eq(x, 0x7fff.ffff) end)
    test_case("max_number1", function()local x=0x7fff.ffff check_eq(x, 0x7fff.ffff) end)
    test_case("max_number2", function()local x=0x8000.0000 check_eq(x, -0x8000.0000) end)
end

function test_system()
    --test_case("load", function()load(filename, [breadcrumb], [param_str]) end)
    --test_case("save", function()load("#mygame_level2", "back to map", "lives="..lives)  end)
    --test_case("folder", function()folder end)
    --test_case("ls", function()ls([directory]) end)
    --test_case("run", function()run([param_str]) end)
    --test_case("stop", function()stop([message]) end)
    --test_case("resume", function()resume end)
    --test_case("assert", function()assert(addr = 0 and addr <= 0x7fff, "out of range") poke(addr, 42) -- the memory address is ok, for sure!  end)
    --test_case("reboot", function()reboot end)
    --test_case("reset", function()reset() end)
    --test_case("info", function()info() end)
    --test_case("flip", function()::_:: cls() for i=1,100 do a=i/50 - t() local x=64+cos(a)*i y=64+sin(a)*i circfill(x,y,1,8+(i/4)%8) end flip()goto _  end)
    --test_case("printh", function()printh(str, [filename], [overwrite], [save_to_desktop]) end)
    test_case("time", function()local a=time() flip() local b=time() check_lt(a, b)end)
    test_case("t", function()local a=t() flip() local b=t() check_lt(a, b)end)

    test_case("stat0", function()local x=stat(0) check_gt(x, 0) check_lt(x, 2048)end)
    test_case("stat1", function()local x=stat(1) check_gt(x, 0) check_le(x, 1.0)end)
    -- stat(2) returns the system CPU usage:
    -- {3} Current Display stat(3) returns the current display being accessed (set by _map_display(n)), ranging from 0-3.
    test_case("stat4", function()printh("Hello, world!", "@clip", true) local x=stat(4) check_eq(x, "Hello, world!") end)
    -- {5} PICO-8 version The running PICO-8 version ID can be found in stat(5), as a number.
    test_case("stat6", function()local x=stat(6) check_eq(x, "myparam")end)
    test_case("stat7", function()flip() local x=stat(7) check_gt(x, 0) check_le(x, 60) end)
    -- {8} Target frame rate
    -- {9} PICO-8 frame rate
    -- {11} Number of displays
    -- {12…15} Pause menu location
    test_case("stat16_19", function()sfx(0, 0) check_eq(stat(16), 0) check_eq(stat(17), -1) check_eq(stat(18), -1) check_eq(stat(19), -1) sfx(-1, 0) sleep(0.1) check_eq(stat(16), -1) check_eq(stat(17), -1) check_eq(stat(18), -1) check_eq(stat(19), -1) end)
    test_case("stat20_23", function()sfx(0, 0) sleep(0.1) local x=stat(20) check_gt(x, 0) check_lt(x, 32) check_eq(stat(21), -1) check_eq(stat(22), -1) check_eq(stat(23), -1) sfx(-1, 0) sleep(0.1) check_eq(stat(20), -1) check_eq(stat(21), -1) check_eq(stat(22), -1) check_eq(stat(23), -1) end)
    test_case("stat24", function()music(0) sleep(0.1) check_eq(stat(54), 0) music(-1) sleep(0.1) check_eq(stat(54), -1) end)
    test_case("stat25", function()check_eq(stat(25), -1) music(0) sleep(0.1) check_eq(stat(25), 0) music(-1) end)
    -- 26 Ticks played on current pattern
    -- {28} Raw keyboard
    -- stat(29) reportedly returns the number of connected controllers,
    --STAT(30) -- (Boolean) True when a keypress is available
    --STAT(31) -- (String) character returned by keyboard
    --STAT(32) -- Mouse X
    --STAT(33) -- Mouse Y
    --STAT(34) -- Mouse buttons (bitfield)
    --STAT(36) -- Mouse wheel event
    --STAT(38) -- Relative x movement (in host desktop pixels) -- requires flag 0x4
    --STAT(39) -- Relative y movement (in host desktop pixels) -- requires flag 0x4
    test_case("stat46_49", function()sfx(0, 0) check_eq(stat(46), 0) check_eq(stat(47), -1) check_eq(stat(48), -1) check_eq(stat(49), -1) sfx(-1, 0) sleep(0.1) check_eq(stat(46), -1) check_eq(stat(47), -1) check_eq(stat(48), -1) check_eq(stat(49), -1) end)
    test_case("stat50_53", function()sfx(0, 0) sleep(0.1) local x=stat(50) check_gt(x, 0) check_lt(x, 32) check_eq(stat(51), -1) check_eq(stat(52), -1) check_eq(stat(53), -1) sfx(-1, 0) sleep(0.1) check_eq(stat(50), -1) check_eq(stat(51), -1) check_eq(stat(52), -1) check_eq(stat(53), -1) end)
    test_case("stat54", function()music(0) sleep(0.1) check_eq(stat(54), 0) music(-1) sleep(0.1) check_eq(stat(54), -1) end)
    test_case("stat55", function()check_eq(stat(55), -1) music(0) sleep(0.1) check_eq(stat(55), 0) music(-1) end)
    -- 56 Ticks played on current pattern
    test_case("stat57", function()music(0) sleep(0.1) check_eq(stat(57), true) music(-1) sleep(0.1) check_eq(stat(57), false) end)
    test_case("stat80", function()local x=stat(80) check_ge(x, 2025) check_lt(x, 2100) end)
    test_case("stat81", function()local x=stat(81) check_ge(x, 1) check_le(x, 12) end)
    test_case("stat82", function()local x=stat(82) check_ge(x, 1) check_le(x, 31) end)
    test_case("stat83", function()local x=stat(83) check_ge(x, 0) check_lt(x, 24) end)
    test_case("stat84", function()local x=stat(84) check_ge(x, 0) check_le(x, 60) end)
    test_case("stat85", function()local x=stat(85) check_ge(x, 0) check_le(x, 60) end)
    test_case("stat90", function()local x=stat(90) check_ge(x, 2025) check_lt(x, 2100) end)
    test_case("stat91", function()local x=stat(91) check_ge(x, 1) check_le(x, 12) end)
    test_case("stat92", function()local x=stat(92) check_ge(x, 1) check_le(x, 31) end)
    test_case("stat93", function()local x=stat(93) check_ge(x, 0) check_lt(x, 24) end)
    test_case("stat94", function()local x=stat(94) check_ge(x, 0) check_le(x, 61) end)
    test_case("stat95", function()local x=stat(95) check_ge(x, 0) check_le(x, 61) end)
    -- {99} Garbage Collection (Raw)
    -- {100…102} BBS information
    -- {108…109} 5kHz PCM Audio
    -- 100 Current breadcrumb label, or nil
    -- 110 Returns true when in frame-by-frame mode
    -- 0x800  dropped file   //  stat(120) returns TRUE when data is available
    -- 0x802  dropped image  //  stat(121) returns TRUE when data is available
    -- {124} Current path

    --test_case("extcmd", function()extcmd("rec_frames")  end)
end

function test_graphics()
    -- Note: pget is affected by clip.
    test_case("clip_none", function()clip(0, 0, 128, 128) pset(0, 0, 1) check_eq(pget(0, 0), 1)end)
    test_case("clip", function()clip(0, 0, 128, 128) pset(0, 0, 1) clip(20, 20, 10, 10) pset(0, 0, 2) clip(0, 0, 128, 128) check_eq(pget(0, 0), 1)end)
    test_case("clip_too_big", function()clip(-10, -10, 148, 148) pset(-10, -10, 1) check_eq(pget(-10, -10), 0)end)
    test_case("clip_ok", function()clip(0, 0, 128, 128) pset(25, 25, 1) clip(20, 20, 10, 10) pset(25, 25, 2) check_eq(pget(25, 25), 2)end)
    test_case("clip_replace", function()clip(0, 0, 128, 128) pset(0, 0, 1) clip(20, 20, 10, 10) pset(0, 0, 2) clip(0, 0, 128, 128) check_eq(pget(0, 0), 1) clip(0, 0, 10, 10) pset(0, 0, 2) clip(0, 0, 128, 128) check_eq(pget(0, 0), 2)end)
    test_case("clip_empty", function()clip(0, 0, 128, 128)  pset(0, 0, 1) clip(0, 0, 0, 0) pset(0, 0, 2) clip(0, 0, 128, 128) check_eq(pget(0, 0), 1)end)
    test_case("clip_previous", function()clip(0, 0, 128, 128) pset(10, 10, 1) clip(20, 20, 10, 10) clip(5, 5, 50, 50, true) pset(10, 10, 2) clip(0, 0, 128, 128) check_eq(pget(10, 10), 1)end)
    test_case("clip_no_param", function()clip(0, 0, 128, 128) pset(0, 0, 1) clip(20, 20, 10, 10) clip() pset(0, 0, 2) clip(0, 0, 128, 128) check_eq(pget(0, 0), 2)end)
    test_case("pset_pget", function()for y=0,127 do for x=0,127 do pset(x, y, x*y/8) end end check_eq(pget(0, 0), 0) check_eq(pget(3, 7), 2) end)
    test_case("pset_clip", function()pset(25, 25, 8) check_eq(pget(25, 25), 8) clip(3, 3, 7, 7) pset(25, 25, 3) clip(0, 0, 128, 128) check_eq(pget(25, 25), 8) end)
    test_case("pset_out_of_bounds", function()pset(-10, -10, 7)check_eq(pget(-10, -10), 0)end)
    test_case("pget_clip", function()pset(25, 25, 8) check_eq(pget(25, 25), 8) clip(3, 3, 7, 7) check_eq(pget(25, 25), 0) end)
    test_case("pget_out_of_bounds", function()check_eq(pget(-10, -10), 0)end)
    test_case("pset_screen_mem", function()pset(50, 2, 7) pset(51, 2, 4) check_eq(peek(0x6000+2*64+50/2), 0x47) end)
    test_case("pget_screen_mem", function()poke(0x6000+2*64+50/2, 0x74) check_eq(pget(50, 2), 4) check_eq(pget(51, 2), 7)  end)
    test_case("sset_sget", function()for y=0,127 do for x=0,127 do sset(x, y, x*y/8) end end check_eq(sget(0, 0), 0) check_eq(sget(3, 7), 2) end)
    test_case("sset_out_of_bounds", function()sset(-10, -10, 7)check_eq(sget(-10, -10), 0) end)
    test_case("sget_out_of_bounds", function()check_eq(sget(-10, -10), 0)end)
    test_case("fset_fget_index", function()check_eq(fget(7, 3), false) fset(7, 3, true) check_eq(fget(7, 3), true) end)
    test_case("fset_fget_bitfield", function()check_eq(fget(8), 0) fset(8, 1 | 2 | 8) check_eq(fget(8), 1 | 2 | 8) end)
    test_case("print_screen_mem", function()cls()color(9)print("hello, world!") check_eq(pget(0, 0), 9)end)
    test_case("print_xy", function()cls()color(9)print("hello, world!", 28, 16) check_eq(pget(28, 16), 9)end)
    test_case("print_scroll", function()cls()for i=1,20 do print("line") check_eq(peek(0x5f27), i*6) end print("line") check_eq(peek(0x5f27), 120) print("line") check_eq(peek(0x5f27), 120) end)
    test_case("print_no_newline", function()cls()print("HOGE\0") check_eq(peek(0x5f26), 16) check_eq(peek(0x5f27), 0) end)
    test_case("print_width", function()local x=print("HOGE", 0, -20) check_eq(x, 16) end)
    test_case("cursor_print", function()cursor(80, 50) color(11) print("hello, world!") check_eq(pget(80, 50), 11)end)
    test_case("color", function()color(3)pset(17, 8) check_eq(pget(17, 8), 3)end)
    test_case("color_no_param", function()color(9) color() pset(17, 8) check_eq(pget(17, 8), 6)end)
    test_case("cls", function()cls() check_eq(pget(0, 0), 0)end)
    test_case("cls_col", function()cls(7) check_eq(pget(0, 0), 7)end)
    test_case("camera", function()pset(10, 10, 1) pset(0, 0, 2) camera(10, 10) pset(10, 10, 3) camera(0, 0) check_eq(pget(0, 0), 3) check_eq(pget(10, 10), 1)end)
    test_case("camera_no_param", function()pset(10, 10, 1) pset(0, 0, 2) camera(10, 10) camera() pset(10, 10, 3) check_eq(pget(0, 0), 2) check_eq(pget(10, 10), 3)end)
    -- clip is not affected by camera
    test_case("camera_clip1", function()pset(10, 10, 1) pset(0, 0, 2) clip(0, 0, 5, 5) camera(10, 10) pset(10, 10, 3) camera(0, 0) clip(0, 0, 128, 128) check_eq(pget(0, 0), 3) check_eq(pget(10, 10), 1)end)
    test_case("camera_clip2", function()pset(10, 10, 1) pset(0, 0, 2) camera(10, 10) clip(0, 0, 5, 5) pset(10, 10, 3) camera(0, 0) clip(0, 0, 128, 128) check_eq(pget(0, 0), 3) check_eq(pget(10, 10), 1)end)
    test_case("camera_clip3", function()pset(10, 10, 1) pset(0, 0, 2) clip(7, 7, 5, 5) camera(10, 10) pset(10, 10, 3) camera(0, 0) clip(0, 0, 128, 128) check_eq(pget(0, 0), 2) check_eq(pget(10, 10), 1)end)
    test_case("camera_clip4", function()pset(10, 10, 1) pset(0, 0, 2) camera(10, 10) clip(7, 7, 5, 5) pset(10, 10, 3) camera(0, 0) clip(0, 0, 128, 128) check_eq(pget(0, 0), 2) check_eq(pget(10, 10), 1)end)
    test_case("circ", function()cls()color(9)circ(50, 40, 20)check_eq(pget(50, 40), 0) check_eq(pget(50, 20), 9) check_eq(pget(50, 19), 0) check_eq(pget(50, 60), 9) check_eq(pget(50, 61), 0) check_eq(pget(30, 40), 9) check_eq(pget(29, 40), 0) check_eq(pget(70, 40), 9) check_eq(pget(71, 40), 0)check_eq(pget(31, 21), 0)end)
    test_case("circ_col", function()cls()color(7)circ(50, 40, 20, 9)check_eq(pget(50, 40), 0) check_eq(pget(50, 20), 9) check_eq(pget(50, 19), 0) check_eq(pget(50, 60), 9) check_eq(pget(50, 61), 0) check_eq(pget(30, 40), 9) check_eq(pget(29, 40), 0) check_eq(pget(70, 40), 9) check_eq(pget(71, 40), 0)check_eq(pget(31, 21), 0)end)
    test_case("circfill", function()cls()color(9)circfill(50, 40, 20)check_eq(pget(50, 40), 9) check_eq(pget(50, 20), 9) check_eq(pget(50, 19), 0) check_eq(pget(50, 60), 9) check_eq(pget(50, 61), 0) check_eq(pget(30, 40), 9) check_eq(pget(29, 40), 0) check_eq(pget(70, 40), 9) check_eq(pget(71, 40), 0)check_eq(pget(31, 21), 0)end)
    test_case("circfill_col", function()cls()color(7)circfill(50, 40, 20, 9)check_eq(pget(50, 40), 9) check_eq(pget(50, 20), 9) check_eq(pget(50, 19), 0) check_eq(pget(50, 60), 9) check_eq(pget(50, 61), 0) check_eq(pget(30, 40), 9) check_eq(pget(29, 40), 0) check_eq(pget(70, 40), 9) check_eq(pget(71, 40), 0)check_eq(pget(31, 21), 0)end)
    test_case("oval", function()cls()oval(40, 30, 80, 60, 7) check_eq(pget(60, 45), 0) check_eq(pget(40, 30), 0) check_eq(pget(80, 60), 0) check_eq(pget(46, 34), 0) check_eq(pget(47, 35), 7) pset(47, 35, 4) check_eq(pget(48, 35), 0) end)
    test_case("ovalfill", function()cls()ovalfill(40, 30, 80, 60, 7) check_eq(pget(60, 45), 7) check_eq(pget(40, 30), 0) check_eq(pget(80, 60), 0) check_eq(pget(46, 34), 0) check_eq(pget(47, 35), 7) check_eq(pget(48, 35), 7) end)
    test_case("line", function()cls() line(90, 90, 80, 30, 3) check_eq(pget(90, 90), 3) check_eq(pget(80, 30), 3) check_eq(pget(80, 31), 3) end)
    test_case("line_end_points", function()cls() color(2) line() line(20, 20) check_eq(pget(20, 20), 0) color(3) line(5, 5) check_eq(pget(20, 20), 3) check_eq(pget(5, 5), 3) end)
    test_case("rect", function()cls()rect(1, 1, 126, 126, 6) check_eq(pget(0, 0), 0) check_eq(pget(1, 1), 6) check_eq(pget(127, 1), 0) check_eq(pget(126, 1), 6) check_eq(pget(1, 127), 0) check_eq(pget(1, 126), 6) check_eq(pget(127, 127), 0) check_eq(pget(126, 126), 6) check_eq(pget(2, 2), 0) check_eq(pget(64, 64), 0) check_eq(pget(125, 125), 0) end)
    test_case("rectfill", function()cls()rectfill(1, 1, 126, 126, 6) check_eq(pget(0, 0), 0) check_eq(pget(1, 1), 6) check_eq(pget(127, 1), 0) check_eq(pget(126, 1), 6) check_eq(pget(1, 127), 0) check_eq(pget(1, 126), 6) check_eq(pget(127, 127), 0) check_eq(pget(126, 126), 6) check_eq(pget(2, 2), 6) check_eq(pget(64, 64), 6) check_eq(pget(125, 125), 6) end)
    test_case("rrect", function()cls()rrect(1, 1, 126, 126, 20, 6) check_eq(pget(6, 6), 0) check_true(pget(7, 7) == 6 or pget(8, 7) == 6 or pget(7, 8) == 6) check_eq(pget(8, 8), 0) check_eq(pget(64, 64), 0) end)
    test_case("rrectfill", function()cls()rrectfill(1, 1, 126, 126, 20, 6) check_eq(pget(6, 6), 0) check_true(pget(7, 7) == 6 or pget(8, 7) == 6 or pget(7, 8) == 6) check_eq(pget(8, 8), 6) check_eq(pget(64, 64), 6) end)
    test_case("pal_draw", function()pal(9, 8) pset(0, 0, 9) pal(9, 9) check_eq(pget(0, 0), 8) end)
    test_case("pal_draw_sprite", function()spr(1, 0, 0) check_eq(pget(2, 4), 9) pal(9, 8) spr(1, 7, 60) pal(9, 9) check_eq(pget(9, 64), 8) end)
    test_case("pal_no_param", function()pal(9, 8) pal() pset(0, 0, 9) check_eq(pget(0, 0), 9) end)
    test_case("pal_display", function()pal(9, 8, 1) pset(0, 0, 9) pal(9, 9, 1) check_eq(pget(0, 0), 9) end)
    --test_case("pal_secondary", function() TODO end)
    test_case("pal_table", function()pal({[12]=9, [14]=8}) spr(1, 0, 0) check_eq(pget(0, 6), 9) check_eq(pget(4, 6), 8) end)
    test_case("palt", function()palt(8, true) spr(1, 0, 0) check_eq(pget(2, 4), 9) check_eq(pget(0, 4), 0) end)
    test_case("palt", function()palt(0b1100000000000000) spr(1, 0, 0) check_eq(pget(0, 0), 0) check_eq(pget(2, 0), 0) check_eq(pget(4, 0), 2) end)
    test_case("spr", function()spr(1, 0, 0) check_eq(pget(0, 0), 0) check_eq(pget(6, 0), 3) check_eq(pget(0, 6), 12) check_eq(pget(6, 6), 15) end)
    test_case("spr_tiled", function()spr(1, 0, 0, 2, 1) check_eq(pget(6, 0), 3) check_eq(pget(8, 0), 1) check_eq(pget(22, 0), 0) end)
    test_case("spr_hflip", function()spr(1, 0, 0, 1, 1, true) check_eq(pget(1, 0), 3) check_eq(pget(7, 0), 0) check_eq(pget(1, 6), 15) check_eq(pget(7, 6), 12) end)
    test_case("spr_vflip", function()spr(1, 0, 0, 1, 1, false, true) check_eq(pget(0, 1), 12) check_eq(pget(6, 1), 15) check_eq(pget(0, 7), 0) check_eq(pget(6, 7), 3) end)
    test_case("sspr", function()sspr(8, 0, 8, 8, 7, 3, 24, 16) check_eq(pget(7, 3), 0) check_eq(pget(7+6*3, 3), 3) check_eq(pget(7, 3+6*2), 12) check_eq(pget(7+6*3, 3+6*2), 15) end)
    test_case("sspr_tiled", function()sspr(8, 0, 16, 8, 7, 3, 48, 16) check_eq(pget(7+6*3, 3), 3) check_eq(pget(7+8*3, 3), 1) check_eq(pget(7+22*3, 3), 0)  end)
    test_case("sspr_hflip", function()sspr(8, 0, 8, 8, 7, 3, 24, 16, true) check_eq(pget(7+1*3, 3), 3) check_eq(pget(7+7*3, 3), 0) check_eq(pget(7+1*3, 3+6*2), 15) check_eq(pget(7+7*3, 3+6*2), 12) end)
    test_case("sspr_vflip", function()sspr(8, 0, 8, 8, 7, 3, 24, 16, false, true) check_eq(pget(7, 3+1*2), 12) check_eq(pget(7+6*3, 3+1*2), 15) check_eq(pget(7, 3+7*2), 0) check_eq(pget(7+6*3, 3+7*2), 3) end)
    --test_case("fillp", function()pal(3,12) circfill(64,64,20,3)  end)
    test_case("fillp", function()cls()fillp(0b1010101010100000)rectfill(1, 1, 10, 10, 0x4e) check_eq(pget(1, 1), 0xe) check_eq(pget(2, 1), 0x4) end)
    test_case("fillp_transparency", function()cls()fillp(0b1010101010100000.1)rectfill(1, 1, 10, 10, 0x4e) check_eq(pget(1, 1), 0xe) check_eq(pget(2, 1), 0) end)
    test_case("fillp_sprite", function()cls()for i=0,15 do pal(i, i+i*16, 2) end pal(2, 0x87, 2) fillp(0b1010101010100000.01)spr(1, 0, 0) check_eq(pget(0, 0), 0) check_eq(pget(2, 0), 1) check_eq(pget(4, 0), 0x8) check_eq(pget(5, 0), 0x7) end)
    test_case("fillp_global_2nd", function()cls()for i=0,15 do pal(i, i+i*16, 2) end pal(2, 0x87, 2) fillp(0b1010101010100000.001) rectfill(0, 0, 10, 10, 1) rectfill(20, 0, 30, 10, 2) check_eq(pget(0, 0), 1) check_eq(pget(20, 0), 8) check_eq(pget(21, 0), 7) end)
    test_case("fillp_color", function()cls()poke(0x5f34, 0x3)rectfill(1, 1, 10, 10, 0x104e.aaaa) check_eq(pget(1, 1), 0xe) check_eq(pget(2, 1), 0x4)  end)
    test_case("fillp_invert", function()cls()poke(0x5f34, 0x3)rectfill(1, 1, 10, 10, 0x184e.aaaa) check_eq(pget(1, 1), 0x4) check_eq(pget(2, 1), 0xe) end)
    test_case("fillp_symbol", function()cls()fillp(▒)rectfill(1, 1, 10, 10, 0x4e) check_eq(pget(1, 1), 0xe) check_eq(pget(2, 1), 0) end)
end

function test_table_functions()
    test_case("add", function()local t={}add(t,11)add(t,22)check_eq(#t,2)check_eq(t[2],22)end)
    test_case("del", function()local a={1,10,2,11,3,12}del(a,10)check_false(a[2]==10)end)
    test_case("deli", function()local a={"x","y","z"}v=deli(a,2)check_eq(#a,2)check_eq(v,"y")end)
    test_case("count", function()local a={1,2,2,3,2}check_eq(count(a),5)end)
    test_case("count_value", function()local a={1,2,2,3,2}check_eq(count(a,2),3)end)
    test_case("all", function()local t={11,12,13}add(t,14)local found=false for v in all(t) do if v==13 then found=true end end check_true(found) end)
    test_case("foreach", function() local s=0 foreach({1,2,3}, function(v) s=s+v end) check_eq(s,6) end)
    test_case("pairs", function()local t={["hello"]=3, [10]="blah"} t.blue = 5 local seen_blah=false local seen_3=false local seen_5=false local seen_other=false for k,v in pairs(t) do if k == 10 then seen_blah=(v=="blah") elseif k == "hello" then seen_3=(v == 3) elseif k=="blue" then seen_5=(v==5) else seen_other=true end end check_true(seen_blah) check_true(seen_3) check_true(seen_5) check_false(seen_other) end)
end

function test_input()
    --test_case("btn", function()btn([b], [pl]) end)
    --test_case("btnp", function()poke(0x5f5c, delay) -- set the initial delay before repeating. 255 means never repeat. poke(0x5f5d, delay) -- set the repeating delay.  end)
end

function test_audio()
    --test_case("sfx", function()sfx(3) -- play sfx 3 sfx(3,2) -- play sfx 3 on channel 2 sfx(3,-2) -- stop sfx 3 from playing on any channel sfx(-1,2) -- stop whatever is playing on channel 2 sfx(-2,2) -- release looping on channel 2 sfx(-1) -- stop all sounds on all channels sfx(-2) -- release looping on all channels  end)
    --test_case("music", function()music(0, 1000)  end)
end

function test_map()
    test_case("mget_mset_map", function() mset(0,0,42) check_eq(mget(0,0),42) end)
    --test_case("map", function()camera(pl.x - 64, pl.y - 64) map()  end)
    --test_case("tline", function()poke(0x5f3a, offset_x) poke(0x5f3b, offset_y)  end)
end

function test_memory()
    test_case("peek", function()pset(0, 0, 1) pset(1, 0, 2) pset(2, 0, 3) pset(3, 0, 4) local a, b = peek(0x6000, 2) check_eq(a, 0x21) check_eq(b, 0x43) end)
    test_case("peek_alt", function()pset(0, 0, 1) pset(1, 0, 2) check_eq(@0x6000, 0x21) end)
    test_case("poke", function()cls()poke(0x6000, 9) check_eq(pget(0, 0), 9)end)
    test_case("poke_ff", function()poke(0x6000, 0xff) check_eq(peek(0x6000), 0xff)end)
    test_case("peek2", function()poke(0x4300, 0xab) poke(0x4301, 0xcd) check_eq(peek2(0x4300), 0xcdab) end)
    test_case("peek2_alt", function()poke(0x4300, 0xab) poke(0x4301, 0xcd) check_eq(%0x4300, 0xcdab) end)
    test_case("poke2", function()poke2(0x4300, 0xcdab) check_eq(peek(0x4300), 0xab) check_eq(peek(0x4301), 0xcd) end)
    test_case("peek4", function()poke(0x4300, 0x12) poke(0x4301, 0x34) poke(0x4302, 0x56) poke(0x4303, 0x78) check_eq(peek4(0x4300), 0x7856.3412) end)
    test_case("peek4_alt", function()poke(0x4300, 0x12) poke(0x4301, 0x34) poke(0x4302, 0x56) poke(0x4303, 0x78) check_eq($0x4300, 0x7856.3412) end)
    test_case("poke4", function()poke4(0x4300, 0x7856.3412) check_eq(peek(0x4300), 0x12) check_eq(peek(0x4301), 0x34) check_eq(peek(0x4302), 0x56) check_eq(peek(0x4303), 0x78) end)
    test_case("memcpy", function()pset(0, 0, 7) pset(63, 0, 11) memcpy(0x4300, 0x6000, 0x20) check_eq(peek(0x4300), peek(0x6000)) check_eq(peek(0x431f), peek(0x601f)) end)
    test_case("reload", function()poke(5, 0) poke(6, 0) reload(0x0, 0x0, 0x20) check_eq(peek(5), 0x11) check_eq(peek(6), 0x22) end)
    --test_case("cstore", function()cstore(dest_addr, source_addr, len, [filename]) end)
    test_case("memset", function()memset(0x6000, 0xc8, 0x1000) check_eq(pget(0, 0), 8) check_eq(pget(1, 0), 0xc) end)
end

function test_math()
    test_case("max", function()check_eq(max(4, 9), 9) end)
    test_case("max_neg", function()check_eq(max(-2, -7), -2) end)
    test_case("max_pos_neg", function()check_eq(max(2, -7), 2) end)
    test_case("min", function()check_eq(min(4, 9), 4) end)
    test_case("min_neg", function()check_eq(min(-2, -7), -7) end)
    test_case("min_pos_neg", function()check_eq(min(2, -7), -7) end)
    test_case("mid", function()check_eq(mid(7, 5, 10), 7) end)
    test_case("flr", function()check_eq(flr(4.1), 4) end)
    test_case("flr_neg", function()check_eq(flr(-2.3), -3) end)
    test_case("ceil", function()check_eq(ceil(4.1), 5) end)
    test_case("ceil_neg", function()check_eq(ceil(-2.3), -2) end)
    test_case("cos", function()check_eq(cos(0), 1) end)
    test_case("sin", function()check_eq(sin(0.25), -1) end)
    test_case("atan2", function()check_eq(atan2(0,-1), 0.25) end)
    test_case("sqrt", function()check_eq(sqrt(9), 3) end)
    test_case("abs", function()check_eq(abs(-5), 5) end)
    test_case("rnd", function()local r=rnd(1) check_ge(r, 0) check_lt(r, 1) end)
    test_case("srand1", function()srand(123) local a=rnd(1) srand(123) local b=rnd(1) check_eq(a,b) end)
    test_case("srand2", function()srand(123) local a=rnd(1) srand(234) local b=rnd(1) check_ne(a,b) end)

    test_case("band", function()local x = 0b1010 local y=0b0110 check_eq(band(x, y), 0B0010) end)
    test_case("band_op", function()local x = 0b1010 local y=0b0110 check_eq(x & y, 0B0010) end)
    test_case("bor", function()check_eq(bor(0b1001, 0b1010), 0b1011) end)
    test_case("bor_op", function()check_eq(0b1001 | 0b1010, 0b1011) end)
    test_case("bxor", function()check_eq(bxor(0b101, 0b110), 0b011) end)
    test_case("bxor_op", function()check_eq(0b101 ^^ 0b110, 0b011) end)
    test_case("bnot", function()check_eq(bnot(0b110), 0b1111111111111001.1111111111111111) end)
    test_case("bnot_op", function()check_eq(~0b110, 0b1111111111111001.1111111111111111) end)
    test_case("shl", function()check_eq(shl(0b101, 3), 0b101000) end)
    test_case("shl_op", function()check_eq(0b101 << 3, 0b101000) end)
    test_case("shr1", function()check_eq(shr(0b1111111111101000, 3), 0b1111111111111101) end)
    test_case("shr2", function()check_eq(shr(0b0111111111101000, 3), 0b0000111111111101) end)
    test_case("shr_op", function()check_eq(0b1111111111101000 >> 3, 0b1111111111111101) end)
    test_case("lshr", function()check_eq(lshr(0b101000, 3), 0b000101) end)
    test_case("lshr_op", function()check_eq(0b101000 >>> 3, 0b000101) end)
    test_case("rotl", function()check_eq(rotl(0b10100, 2), 0b1010000) end)
    test_case("rotl_op", function()check_eq(0b10100 <<> 2, 0b1010000) end)
    test_case("rotr", function()check_eq(rotr(0b10100, 2), 0b00101) end)
    test_case("rotr_op", function()check_eq(0b10100 >>< 2, 0b00101) end)

    test_case("int_div", function()check_eq(9\2, 4) end)

end

function test_custom_menu_items()
    --test_case("menuitem", function()menuitem(3, "screenshake: off", function() screenshake = not screenshake menuitem(nil, "screenshake: "..(screenshake and "on" or "off")) return true -- don't close end )  end)
end

function test_strings_and_type_conversion()
    test_case("strings", function()local s = "the quick" check_eq(s, "the quick") s = 'brown fox' check_eq(s, "brown fox"); s = [[
jumps over
multiple lines
]] check_eq(s, "jumps over\nmultiple lines\n") end)
    test_case("string_length", function()local s="foo" check_eq(#s, 3) end)
    test_case("concatenation", function()check_eq("three "..4, "three 4") end)
    test_case("string_arithmetic", function()check_eq(2+"3", 5) end)
    test_case("tostr", function()check_eq(tostr(17), "17") check_eq(tostr(17,0x1), "0x0011.0000") check_eq(tostr(17,0x3), "0x00110000") check_eq(tostr(17,0x2), "1114112") end)
    test_case("tonum", function()check_eq(tonum("ff", 0x1), 255) check_eq(tonum("1114112", 0x2), 17) check_eq(tonum("1234abcd", 0x3), 0x1234.abcd) end)
    test_case("chr", function()check_eq(chr(64), "@") check_eq(chr(104,101,108,108,111), "hello") end)
    test_case("ord", function()check_eq(ord("@"), 64) check_eq(ord("123",2), 50) check_eq(ord("123",2,3), 50,51,52) end)
    test_case("sub", function()local s = "the quick brown fox" check_eq(sub(s,5,9), "quick") check_eq(sub(s,5), "quick brown fox") --[[ check_eq(sub(s,5,true), "q") --]] end)
    test_case("split", function()check_eq(split("1,2,3"), {1,2,3}) check_eq(split("one:two:3",":",false), {"one","two","3"}) check_eq(split("1,,2,"), {1,"",2,""}) end)
    test_case("type", function()check_eq(type(3), "number") check_eq(type("3"), "string") end)
end

function test_cartridge_data()
    test_case("cartdata", function()check_true(cartdata("zep_dark_forest_test")) end)
    test_case("dset_dget", function()dset(0, 123) check_eq(dget(0), 123) end)
    test_case("dset_to_mem", function()dset(0, 34) check_eq(peek4(0x5e00), 34) end)
    test_case("dget_from_mem", function()poke4(0x5e00, 234) check_eq(dget(0), 234) end)
end

function test_gpio()
    --test_case("gpio", function() TODO end)
    --test_case("serial", function() TODO end)
end

function test_metatables()
    test_case("metatables", function()local vec2d={ __add=function(a,b) return {x=(a.x+b.x), y=(a.y+b.y)} end } v1={x=2,y=9} setmetatable(v1, vec2d) v2={x=1,y=5} setmetatable(v2, vec2d) v3 = v1+v2 check_eq(v3.x..","..v3.y, "3,14") end)
    --test_case("setmetatable", function()setmetatable(tbl, m) end)
    --test_case("getmetatable", function()getmetatable(tbl) end)
    --test_case("rawset", function()rawset(tbl, key, value) end)
    --test_case("rawget", function()rawget(tbl, key) end)
    --test_case("rawequal", function()rawequal(tbl1,tbl2) end)
    --test_case("rawlen", function()rawlen(tbl) end)
end

function test_function_arguments()
    test_case("args_table", function()function foo(...) local args={...} check_eq(args, {1, 2, 3}) end foo(1, 2, 3) end)
    test_case("args_pass", function()function bar(...) local args={...} check_eq(args, {2, 3, 4}) end function foo(...) bar(select(2, ...)) end foo(1, 2, 3, 4) end)
end

function test_coroutines()
    test_case("coroutines", function()
        sum=0
        function count()
            for i=1,3 do
                sum += i
                yield()
            end
        end
        local cor = cocreate(count)
        check_ne(costatus(cor), 'dead')
        check_eq(sum, 0)
        coresume(cor)
        check_ne(costatus(cor), 'dead')
        check_eq(sum, 1)
        coresume(cor)
        check_ne(costatus(cor), 'dead')
        check_eq(sum, 3)
        coresume(cor)
        check_ne(costatus(cor), 'dead')
        check_eq(sum, 6)
        coresume(cor)
        check_eq(costatus(cor), 'dead')
        cor = nil
    end)
    --test_case("yield", function()function hey() print("doing something") yield() print("doing the next thing") yield() print("finished") end c = cocreate(hey) for i=1,3 do coresume(c) end  end)
    --test_case("cocreate", function()cocreate(f) end)
    --test_case("coresume", function()coresume(c, [p0, p1 ..]) end)
    --test_case("assert", function()assert(coresume(c)) end)
    --test_case("costatus", function()costatus(c) end)
    --test_case("yield", function()yield end)
end

function test_draw_state()
    -- 0x5f20-0x5f23 - clip rectangle
    test_case("clip_to_draw_state", function()clip(7, 11, 32, 40) check_eq(peek(0x5f20), 7) check_eq(peek(0x5f21), 11) check_eq(peek(0x5f22), 7+32) check_eq(peek(0x5f23), 11+40) end)
    test_case("clip_from_draw_state1", function()clip(0, 0, 128, 128) pset(0, 0, 1) poke(0x5f20, 20) poke(0x5f21, 20) poke(0x5f22, 30) poke(0x5f23, 30) pset(0, 0, 2) clip(0, 0, 128, 128) check_eq(pget(0, 0), 1)end)
    test_case("clip_from_draw_state2", function()clip(0, 0, 128, 128) pset(25, 25, 1) poke(0x5f20, 20) poke(0x5f21, 20) poke(0x5f22, 30) poke(0x5f23, 30) pset(25, 25, 2) check_eq(pget(25, 25), 2)end)
    -- 0x5f24 - cursor carriage return x position
    test_case("print_left_margin", function()cls()poke(0x5f24, 50)print("line") check_eq(peek(0x5f26), 50) end)
    test_case("print_left_margin_cursor", function()cls()cursor(50)print("line") check_eq(peek(0x5f26), 50) end)
    -- 0x5f25 - pen colour
    test_case("cursor_col_to_draw_state", function()cursor(34, 66, 11) check_eq(peek(0x5f25), 11)end)
    test_case("color_to_draw_state", function()color(3) check_eq(peek(0x5f25),3)end)
    test_case("color_from_draw_state", function()poke(0x5f25, 10) pset(17, 8) check_eq(pget(17, 8), 10)end)
    -- 0x5f26-0x5f27 - cursor position
    test_case("print_cursor_to_draw_state", function()cls()print("line") check_eq(peek(0x5f26), 0) check_eq(peek(0x5f27), 6) end)
    test_case("cursor_to_draw_state", function()cursor(77, 55) check_eq(peek(0x5f26), 77) check_eq(peek(0x5f27), 55) end)
    test_case("cursor_print_from_draw_state", function()poke(0x5f26, 80) poke(0x5f27, 50) color(11) print("hello, world!") check_eq(pget(80, 50), 11)end)
    -- 0x5f28-0x5f2b - camera position
    test_case("camera_to_draw_state", function()camera(-10, -10) check_eq(peek(0x5f28) | (peek(0x5f29) << 8), -10) check_eq(peek(0x5f2a) | (peek(0x5f2b) << 8), -10) end)
    test_case("camera_from_draw_state", function()pset(10, 10, 1) pset(0, 0, 2) poke(0x5f28, 10) poke(0x5f2a, 10) pset(10, 10, 3) camera(0, 0) check_eq(pget(0, 0), 3) check_eq(pget(10, 10), 1)end)
    --transformation mode: 0x5f2c
    -- 0x5f2d
    --   0x1 - enable mouse + keyboard
    --   0x2 - mouse buttons trigger btn(4)..btn(6)
    --   0x4 - pointer lock
    -- 0x5f31..0x5f33 - fill pattern
    -- 0x5f34 - color flags
    --    0x2 - the invert flag will be validated
    --    0x1 - accept fill pattern in colour value
    -- 0x5f35 - endpoint invalid
    -- 0x5f36
    --   0x80 - enable character wrap by default when printing
    --   0x40 - disable automatic scrolling caused by print() (with no x,y given)
    --   0x20 - turn off 0x808 audio lpf filter
    --   0x10 - enable sget, mget, pget default values
    --    0x8 - treat sprite 0 as opaque when drawn by map(), tline()
    --    0x4 - don't add newlines after print()
    --    0x2 - round circles to even
    test_case("print_no_scroll", function()poke(0x5f36, 0x40)cls()for i=1,20 do print("line") check_eq(peek(0x5f27), i*6) end print("line") check_eq(peek(0x5f27), 126) print("line") check_eq(peek(0x5f27), 132) end)
    -- 0x5f38-0x5f3b - tline width, height, x offset, y offset
    -- 0x5f3c..0x5f3f line endpoint
end

function test_hardware_state()
    -- 0x5f40-0x5f41 - audio channel effects
    -- 0x5f44 - random number generator state
    -- 0x5f4c-0x5f53 - controller states
    -- 0x5f54 - sprite sheet address
    test_case("gfx_0x60", function()poke(0x5f54, 0x60)sset(0, 0, 7)sset(1, 0, 0)check_eq(peek(0x6000), 7)end)
    -- 0x5f55 - screen address
    test_case("screen_0x0", function()poke(0x5f55, 0)pset(0, 0, 7)pset(1, 0, 0)check_eq(peek(0), 7)end)
    -- 0x5f56 - map address
    --   0x10-0x24 - map memory address mapped to 0xXX00
    --   0x80 - map memory address mapped to 0x8000
    --test_case("map_0x10", function()poke(0x5f56, 0x10)mset(0, 0, 7)check_eq(peek(0x1000), 7)end)
    --test_case("map_height1", function()poke(0X5f54, 0x60)poke(0x5f56, 0x10)mset(0, 63, 7)check_eq(mget(0, 63), 7)mset(0, 64, 7)check_eq(mget(0, 64), 0) end)
    --test_case("map_height2", function()poke(0X5f54, 0x60)poke(0x5f56, 0x20)mset(0, 31, 7)check_eq(mget(0, 31), 7)mset(0, 32, 7)check_eq(mget(0, 32), 0) end)
    --test_case("map_height3", function()poke(0X5f54, 0x60)poke(0x5f56, 0x2f)mset(0, 1, 7)check_eq(mget(0, 1), 7)mset(0, 2, 7)check_eq(mget(0, 2), 0) end)
    -- 0x5f57 - map width
    -- 0x5f58-0x5f5b - default print attributes
    --   0x5f5b - draw_y_offset
    test_case("print_attributes_solid_background", function()cls(2)poke(0x5f58, 1 | 0x10)print("OO")check_eq(pget(0, 0), 0)end)
    test_case("print_attributes_invert", function()cls(2)poke(0x5f58, 1 | 0x20)color(3)print("OO")check_eq(pget(0, 0), 3)end)
    test_case("print_attributes_char_h", function()cls()poke(0x5f59, 2 << 4)color(3)print("hello")check_eq(pget(0, 0), 3)check_eq(pget(0, 2),0)end)
    test_case("print_attributes_char_w", function()cls()poke(0x5f59, 2)color(3)print("eO")check_eq(pget(0, 0), 3)check_eq(pget(2, 0),0)end)
    test_case("print_attributes_tab_w", function()cls()poke(0x5f5a, 16 << 4)color(3)print("\te")check_eq(pget(0, 0), 0)check_eq(pget(16, 0),3)end)
    -- 0x5f59-0x5f5b - out of range return values for sget, mget, pget
    test_case("sget_out_of_bounds_value", function()poke(0x5f36, 0x10) poke(0x5f59, 21) check_eq(sget(-10, -10), 21) end)
    test_case("mget_out_of_bounds_value", function()poke(0x5f36, 0x10) poke(0x5f5a, 83) check_eq(mget(-10, -10), 83) end)
    test_case("pget_out_of_bounds_value", function()poke(0x5f36, 0x10) poke(0x5f5b, 4) check_eq(pget(-10, -10), 4)end)
    -- 0x5f5c - btnp initial repeat rate
    -- 0x5f5d - btnp continuous repeat rate
    -- 0x5f5e
    --   high bits - colour read mask
    --   low bits - colour write mask
    -- 0x5f60-0x5f6f - sprite fill patterns LUT
end

test_suite("test_framework", test_test_framework)
test_suite("comments", test_comments)
test_suite("types_and_assignment", test_types_and_assignment)
test_suite("conditionals", test_conditionals)
test_suite("loops", test_loops)
test_suite("functions_and_local_variables", test_functions_and_local_variables)
test_suite("tables", test_tables)
test_suite("pico_8_shorthand", test_pico_8_shorthand)
test_suite("test_pico_8_program_structure", test_pico_8_program_structure)
test_suite("include", test_include)
test_suite("quirks_of_pico_8", test_quirks_of_pico_8)
test_suite("graphics", test_graphics)
test_suite("table_functions", test_table_functions)
test_suite("input", test_input)
test_suite("map", test_map)
test_suite("memory", test_memory)
test_suite("math", test_math)
test_suite("custom_menu_items", test_custom_menu_items)
test_suite("strings_and_type_conversion", test_strings_and_type_conversion)
test_suite("cartridge_data", test_cartridge_data)
test_suite("gpio", test_gpio)
test_suite("metatables", test_metatables)
test_suite("function_arguments", test_function_arguments)
test_suite("coroutines", test_coroutines)
test_suite("draw_state", test_draw_state)
test_suite("hardware_state", test_hardware_state)
test_suite("system", test_system)
test_suite("audio", test_audio)

summary()
__gfx__
00000000001122331111111122222222333333330000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
000000000014283c1000000120000002333333330000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000445566771000000120000002333333330000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
000000004054687c1000000120000002333333330000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
000000008899aabb1000000120000002333333330000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
000000008094a8bc1000000120000002333333330000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000ccddeeff1000000120000002333333330000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000c0d4e8fc1111111122222222333333330000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__map__
0204040404040404040404040404040404040404040404040404040404040403000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0404040404040404040404040404040404040404040404040404040404040404000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0304040404040404040404040404040404040404040404040404040404040402000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__sfx__
000400002405024050240502405024050240502405024050240502405024050240502405024050240502405024050240502405024050240502405024050240502405024050240502405024050240502405024050
__music__
00 00424344

