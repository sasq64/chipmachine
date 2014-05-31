------
-- Lua settings
------

HD = false
if Config.screen_width >= 1200 then
	HD = true
end

print('HD',Config.screen_width,Config.screen_height,HD)

X0 = 80
Y0 = 54
X1 = 636
Y1 = 520

if HD then
X0 = 10
Y0 = 10
X1 = Config.screen_width-10
Y1 = Config.screen_height-10
end


TEXT_COLOR = 0xffe0e080
DIGITS_COLOR = 0xffb0b0a0


Settings.top_left = { X0, Y0 }
Settings.down_right = { X1, Y1 }

scale = 3.0
Settings.main_title = { X0, Y0, scale, TEXT_COLOR }
Settings.main_composer = { X0, Y0+25*scale, scale*0.6, TEXT_COLOR }
Settings.main_format = { X0, Y0+45*scale, scale*0.3, TEXT_COLOR }

Settings.time_field = { X0, Y0 + 200, 1.0, DIGITS_COLOR }
Settings.length_field = { X0 + 100, Y0 + 200, 1.0, DIGITS_COLOR }
Settings.song_field = { X0 + 220, Y0 + 200, 1.0, DIGITS_COLOR }

scale = 1.2
x = 440
y = 340
Settings.next_field = { x, y-14, 0.5, 0xff444477 }
Settings.next_title = { x, y, scale, TEXT_COLOR }
Settings.next_composer = { x, y+25*scale, scale*0.6, TEXT_COLOR }
Settings.next_format = { x, y+45*scale, scale*0.3, TEXT_COLOR }

scale = 80.0
Settings.prev_title = { -3200, Y0, scale, 0 }
Settings.prev_composer = { -3200, Y0+25*scale, scale*0.6, 0 }
Settings.prev_format = { -3200, Y0+45*scale, scale*0.3, 0 }

if HD then
  Settings.spectrum = { X0, Y1, 32, 24.0 }
else
  Settings.spectrum = { X0-50, Y1+40, 26, 16.0 }
end

Settings.search_field = { X0, Y0, 1.0, 0xffaaaaff }

Settings.result_field = { X0, Y0+30, 0.8 }
Settings.result_lines = (Y1-Y0)/23

Settings.font = "data/Neutra.otf"
------
print("Lua parsing done")
