from PIL import Image, ImageDraw, ImageFont

src = r"C:\Users\user\.codex\generated_images\01a0c17e-d4fa-7bb1-bd08-ef32c8f7f3e6\exec-25f1439d-731d-42f6-960f-8f52318280e6.png"
out = r"D:\esp32實習\37_epaper_tag\midautumn_preview.png"
header_out = r"D:\esp32實習\37_epaper_tag\midautumn_bitmap.h"

im = Image.open(src).convert("RGB")
target_ratio = 296 / 128
src_ratio = im.width / im.height
if src_ratio > target_ratio:
    crop_w = int(im.height * target_ratio)
    left = (im.width - crop_w) // 2
    im = im.crop((left, 0, left + crop_w, im.height))
else:
    crop_h = int(im.width / target_ratio)
    top = (im.height - crop_h) // 2
    im = im.crop((0, top, im.width, top + crop_h))

large = im.resize((1184, 512), Image.Resampling.LANCZOS)
draw = ImageDraw.Draw(large)
font_path = r"C:\Windows\Fonts\NotoSansTC-VF.ttf"
font_big = ImageFont.truetype(font_path, 108)
font_small = ImageFont.truetype(font_path, 80)

# White left panel is intentionally kept clear for legible Chinese at e-paper size.
draw.text((55, 145), "中秋節", font=font_big, fill=(0, 0, 0), stroke_width=1)
draw.text((55, 260), "闔家平安", font=font_small, fill=(180, 0, 0), stroke_width=1)

small = large.resize((296, 128), Image.Resampling.LANCZOS)
px = small.load()
for y in range(128):
    for x in range(296):
        r, g, b = px[x, y]
        # Quantize to the only three ink colors of the panel.
        if r > 205 and g > 205 and b > 205:
            px[x, y] = (255, 255, 255)
        elif r > g * 1.25 and r > b * 1.25 and r > 90:
            px[x, y] = (190, 0, 0)
        else:
            px[x, y] = (0, 0, 0)
small.save(out)

def pack(predicate):
    data = []
    for y in range(128):
        for byte_x in range(0, 296, 8):
            value = 0
            for bit in range(8):
                if predicate(px[byte_x + bit, y]):
                    value |= 0x80 >> bit
            data.append(value)
    return data

black = pack(lambda p: p[0] < 40 and p[1] < 40 and p[2] < 40)
red = pack(lambda p: p[0] > 100 and p[0] > p[1] * 1.5 and p[0] > p[2] * 1.5)
with open(header_out, "w", encoding="ascii") as f:
    f.write("#pragma once\n#include <Arduino.h>\n#include <pgmspace.h>\n")
    f.write("constexpr int MIDAUTUMN_W = 296;\nconstexpr int MIDAUTUMN_H = 128;\n")
    for name, values in (("MIDAUTUMN_BLACK", black), ("MIDAUTUMN_RED", red)):
        f.write(f"const uint8_t {name}[] PROGMEM = {{\n")
        for i in range(0, len(values), 16):
            f.write("  " + ",".join(f"0x{v:02X}" for v in values[i:i+16]) + ",\n")
        f.write("};\n")
print(header_out)
print(out)
