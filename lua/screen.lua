------
-- Lua settings
------
Settings = {}

HD = false
if SCREEN_WIDTH >= 800 then
	HD = true
end

GSCALE = 1.0

if SCREEN_HEIGHT >= 800 then
	GSCALE = 2.0
end
-- print('HD',SCREEN_WIDTH,SCREEN_HEIGHT,HD)

X0 = 80
Y0 = 54
X1 = 636
Y1 = 520


if HD then
X0 = 10
Y0 = 10
X1 = SCREEN_WIDTH-10
Y1 = SCREEN_HEIGHT-10
end

background = 0x808080
--SCREEN_HEIGHT / 576

if true then
 TEXT_COLOR = 0xffe0e080
 DIGITS_COLOR = 0xff70b050
 SEARCH_COLOR = 0xffaaaaff
 FORMAT_COLOR = 0xffffffaa
 RESULT_COLOR = 0xff20c020
 SPECTRUM_COLOR0 = 0xff00aaee
 SPECTRUM_COLOR1 = 0xff111155
else
 TEXT_COLOR = 0xff000000
 DIGITS_COLOR = 0xff202080
 RESULT_COLOR = 0xff202040
 SEARCH_COLOR = 0xffffaaaa
 FORMAT_COLOR = 0xffffffaa
 SPECTRUM_COLOR0 = 0xff000000
 SPECTRUM_COLOR1 = 0xff404040
 Settings.background = 0x888888
 Settings.stars = 0
end

Settings.top_left = { X0, Y0 }
Settings.down_right = { X1, Y1 }


scale = 3.0 * GSCALE
Settings.main_title = { X0, Y0, scale, TEXT_COLOR }
Settings.main_composer = { X0, Y0+25*scale, scale*0.6, TEXT_COLOR }
Settings.main_format = { X0, Y0+42*scale, scale*0.25, TEXT_COLOR }


SY = Settings.main_format[2] + 32 * GSCALE
Settings.song_field = { X0, SY, GSCALE, DIGITS_COLOR }
Settings.time_field = { X0 + 130 * GSCALE, SY, GSCALE, DIGITS_COLOR }
Settings.length_field = { X0 + 220 * GSCALE, SY, GSCALE, DIGITS_COLOR }

Settings.xinfo_field = { X0 - 4, SY + 35 * GSCALE, GSCALE * 0.75, 0xffffffff }

Settings.favicon = { X0 + 330 * GSCALE, SY - GSCALE*25, 8*8 * GSCALE, 8*6 * GSCALE }


if HD then
  Settings.scroll = { Y1 - 200, 4.0, 4, "data/Bello.otf" }
  Settings.spectrum = { X0, Y1, 32, 100.0, SPECTRUM_COLOR0, SPECTRUM_COLOR1 }
else
  Settings.scroll = { Y1 - 100, 2.0, 4, "data/Bello.otf" }
  Settings.spectrum = { X0-50, Y1+40, 26, 64.0, SPECTRUM_COLOR0, SPECTRUM_COLOR1 }
end

x = SCREEN_WIDTH - 300 * GSCALE
y = Settings.scroll[1] - 80 * GSCALE

scale = 1.2 * GSCALE
Settings.next_field = { x, y-14, 0.5, 0xff444477 }

Settings.next_title = { x, y, scale, TEXT_COLOR }
Settings.next_composer = { x, y+26*scale, scale*0.6, TEXT_COLOR }
Settings.next_format = { x, y+44*scale, scale*0.3, TEXT_COLOR }

y = Settings.scroll[1] - 70 * GSCALE

scale = 80.0
Settings.exit_title = { -3200, Y0, scale, 0 }
Settings.exit_composer = { -3200, Y0+25*scale, scale*0.6, 0 }
Settings.exit_format = { -3200, Y0+45*scale, scale*0.3, 0 }

x = SCREEN_WIDTH+10
y = 340
scale = 1.0
Settings.enter_title = { x, y, scale, TEXT_COLOR }
Settings.enter_composer = { x, y+25*scale, scale*0.6, TEXT_COLOR }
Settings.enter_format = { x, y+45*scale, scale*0.3, TEXT_COLOR }

Settings.search_field = { X0, Y0, 1.0, SEARCH_COLOR }
Settings.top_status = { X0, Y0, 1.0, FORMAT_COLOR }

LSCALE = 0.8

Settings.result_field = { X0, Y0+30, LSCALE, RESULT_COLOR }
Settings.result_lines = (Y1-Y0)/(30*LSCALE)

Settings.font = "data/Neutra.otf"
Settings.list_font = "data/Neutra.otf"
------
-- print("Lua parsing done")
