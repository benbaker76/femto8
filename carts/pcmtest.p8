pico-8 cartridge // http://www.pico-8.com
version 43
__lua__
-- pcm audio test

function _init()
    phase = 0
    tone = 0
    frame_count = 0
end

function _update()
    if stat(108) < 768 then
        frame_count += 1
        if frame_count >= 30 then
            frame_count = 0
            tone = 1 - tone
        end

        for i=0,255 do
            local sample = sin(phase) * 127 + 128
            if tone == 0 then
                -- 220 hz, a3
                phase += 220 / 5512.5
            else
                -- 440 hz, a4
                phase += 440 / 5512.5
            end

            if phase > 1 then
                phase -= 1
            end

            poke(0x8000 + i, sample)
        end

        serial(0x808, 0x8000, 256)
    end
end

function _draw()
    cls()

    print("pcm audio test", 10, 10, 7)
    print("alternating tones", 10, 20, 6)

    local tone_name = tone == 0 and "220 hz (a3)" or "440 hz (a4)"
    print("tone: " .. tone_name, 10, 35, 11)

    local buffered = stat(108)
    local app_buf = stat(109)
    print("buffered: " .. buffered, 10, 50, 12)
    print("app buffer: " .. app_buf, 10, 60, 13)

    local bar_width = buffered / 2048 * 100
    rectfill(10, 80, 10 + bar_width, 90, 8)
    rect(10, 80, 110, 90, 7)
    print("buffer fullness", 10, 70, 7)

    print("phase: " .. flr(phase * 100) / 100, 10, 100, 14)
end
