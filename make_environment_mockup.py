from PIL import Image, ImageDraw, ImageFont

W, H = 1184, 512
im = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(im)
black = (0, 0, 0)
red = (190, 0, 0)
white = (255, 255, 255)
font_path = r"C:\Windows\Fonts\NotoSansTC-VF.ttf"
font_title = ImageFont.truetype(font_path, 48)
font_label = ImageFont.truetype(font_path, 40)
font_value = ImageFont.truetype(font_path, 92)
font_unit = ImageFont.truetype(font_path, 46)

# Header bar.
d.rectangle((0, 0, W, 76), fill=red)
d.text((35, 12), "環境監測", font=font_title, fill=white)
d.text((945, 18), "ESP32", font=font_label, fill=white)

# Three equal measurement cards.
card_w = W // 3
for i in (1, 2):
    x = i * card_w
    d.line((x, 76, x, H), fill=black, width=4)

def thermometer(cx, cy):
    d.ellipse((cx - 25, cy + 28, cx + 25, cy + 78), fill=red, outline=black, width=8)
    d.rounded_rectangle((cx - 15, cy - 58, cx + 15, cy + 55), radius=15, fill=white, outline=black, width=8)
    d.rectangle((cx - 8, cy - 15, cx + 8, cy + 55), fill=red)
    d.line((cx + 28, cy + 10, cx + 55, cy + 10), fill=black, width=6)
    d.line((cx + 34, cy + 35, cx + 55, cy + 35), fill=black, width=6)

def droplet(cx, cy):
    d.polygon([(cx, cy - 65), (cx - 42, cy - 5), (cx - 42, cy + 28), (cx - 25, cy + 55),
               (cx, cy + 68), (cx + 25, cy + 55), (cx + 42, cy + 28), (cx + 42, cy - 5)], fill=red, outline=black)
    d.ellipse((cx - 13, cy - 20, cx + 2, cy + 12), fill=white)

def sun(cx, cy):
    d.ellipse((cx - 35, cy - 35, cx + 35, cy + 35), fill=red, outline=black, width=7)
    for dx, dy in ((0,-70),(0,70),(-70,0),(70,0),(-50,-50),(50,-50),(-50,50),(50,50)):
        d.line((cx + dx * .65, cy + dy * .65, cx + dx, cy + dy), fill=black, width=8)

centers = [card_w // 2, card_w + card_w // 2, 2 * card_w + card_w // 2]
thermometer(centers[0], 188)
droplet(centers[1], 188)
sun(centers[2], 188)

labels = [("溫度", "25.6", "°C"), ("濕度", "62", "%"), ("亮度", "78", "%")]
for cx, (label, value, unit) in zip(centers, labels):
    tw = d.textbbox((0, 0), label, font=font_label)[2]
    d.text((cx - tw // 2, 285), label, font=font_label, fill=black)
    value_w = d.textbbox((0, 0), value, font=font_value)[2]
    unit_w = d.textbbox((0, 0), unit, font=font_unit)[2]
    gap = 10
    total = value_w + gap + unit_w
    d.text((cx - total // 2, 352), value, font=font_value, fill=black)
    d.text((cx - total // 2 + value_w + gap, 375), unit, font=font_unit, fill=red)

out = r"D:\esp32實習\38_epaper_moon\environment_mockup.png"
im.resize((296, 128), Image.Resampling.LANCZOS).save(out)
print(out)
