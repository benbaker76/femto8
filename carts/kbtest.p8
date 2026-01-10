pico-8 cartridge // http://www.pico-8.com
version 16
__lua__
-- keyboard test
function _init()
    -- enable keyboard devkit mode
    poke(0x5f2d, 1)
    cls()
    y = 0
    keys = {}
end

function _update()
    -- read all keypresses this frame
    while stat(30) do
        local key = stat(31)
        add(keys, {k=key, f=0})
        -- keep only last 10 keys
        if #keys > 10 then
            deli(keys, 1)
        end
    end

    -- age keys
    for k in all(keys) do
        k.f += 1
    end
end

function _draw()
    cls()

    -- title
    print("keyboard test", 2, 2, 7)
    print("press keys", 2, 8, 6)

    -- show key history (left side, compact)
    local ypos = 16
    local key_count = 0
    for k in all(keys) do
        if key_count >= 10 then break end
        local col = 7
        -- fade old keys
        if k.f > 60 then
            col = 1
        elseif k.f > 30 then
            col = 5
        elseif k.f > 15 then
            col = 6
        end

        -- convert special keys to readable
        local keystr = k.k
        if k.k == "\b" then
            keystr = "[bksp]"
        elseif k.k == "\r" then
            keystr = "[enter]"
        elseif k.k == "\t" then
            keystr = "[tab]"
        elseif k.k == "\27" or k.k == "\x1b" then
            keystr = "[esc]"
        elseif ord(k.k) < 32 then
            keystr = "[c-"..ord(k.k).."]"
        end

        print(keystr, 2, ypos, col)
        ypos += 6
        key_count += 1
    end

    -- show stat(30) indicator
    if stat(30) then
        circfill(120, 3, 3, 8)
    else
        circ(120, 3, 3, 5)
    end

    -- raw keyboard grid (stat(28))
    print("raw scancodes:", 2, 80, 6)
    for y=0,5 do
        for x=0,15 do
            local sc = x + y * 16
            local px = 2 + x * 8
            local py = 88 + y * 6
            if stat(28, sc) then
                circfill(px, py, 2, 8)
            else
                circ(px, py, 1, 1)
            end
        end
    end
end
