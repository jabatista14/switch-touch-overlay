"""Render the HB App Store icon and banner from simple vector-like shapes."""

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


OUT = Path(__file__).parent
CYAN = (45, 224, 236)
ORANGE = (255, 159, 70)


def background(size):
    image = Image.new("RGB", size)
    pixels = image.load()
    for y in range(size[1]):
        for x in range(size[0]):
            t = x / max(1, size[0] - 1)
            pixels[x, y] = (
                int(8 + 14 * t),
                int(20 + 22 * t),
                int(36 + 34 * t),
            )
    return image


def switch_mark(image, box):
    x0, y0, x1, y1 = box
    width, height = x1 - x0, y1 - y0
    unit = min(width / 266, height / 150)
    w, h = 266 * unit, 150 * unit
    x, y = x0 + (width - w) / 2, y0 + (height - h) / 2
    d = ImageDraw.Draw(image)
    r = lambda v: round(v * unit)
    p = lambda xx, yy: (round(x + xx * unit), round(y + yy * unit))

    # Console shell and screen. The two Joy-Cons are deliberately unlike
    # Nintendo's logo, keeping the artwork descriptive rather than branded.
    d.rounded_rectangle((*p(0, 0), *p(266, 150)), radius=r(29), fill=(39, 59, 77))
    d.rounded_rectangle((*p(34, 7), *p(232, 143)), radius=r(13), fill=(5, 13, 25))
    d.rounded_rectangle((*p(43, 16), *p(223, 134)), radius=r(6), fill=(23, 53, 69))
    d.ellipse((*p(10, 32), *p(26, 48)), fill=CYAN)
    d.ellipse((*p(12, 105), *p(24, 117)), fill=(88, 119, 133))
    d.ellipse((*p(241, 34), *p(253, 46)), fill=(88, 119, 133))
    d.ellipse((*p(239, 104), *p(255, 120)), fill=ORANGE)

    # A clear virtual finger target, with a solid cursor instead of tiny text.
    cx, cy = p(135, 75)
    d.ellipse((cx - r(37), cy - r(37), cx + r(37), cy + r(37)),
              outline=(39, 166, 179), width=max(2, r(3)))
    d.ellipse((cx - r(23), cy - r(23), cx + r(23), cy + r(23)),
              outline=CYAN, width=max(3, r(5)))
    d.ellipse((cx - r(8), cy - r(8), cx + r(8), cy + r(8)), fill=ORANGE)
    d.line((*p(178, 109), *p(192, 123)), fill=CYAN, width=max(3, r(5)))
    d.polygon([p(168, 99), p(184, 106), p(174, 115)], fill=CYAN)


def font(size, bold=False):
    path = "C:/Windows/Fonts/segoeuib.ttf" if bold else "C:/Windows/Fonts/segoeui.ttf"
    return ImageFont.truetype(path, size)


def render():
    icon = background((256, 150))
    switch_mark(icon, (14, 22, 242, 128))
    icon.save(OUT / "icon.png", optimize=True)

    banner = background((848, 208))
    switch_mark(banner, (27, 27, 330, 181))
    d = ImageDraw.Draw(banner)
    d.text((347, 42), "TOUCH CURSOR", font=font(49, bold=True), fill=(235, 249, 253))
    d.line((349, 113, 802, 113), fill=CYAN, width=3)
    d.text((348, 133), "Joystick-to-touch for a damaged screen", font=font(23),
           fill=(168, 209, 221))
    banner.save(OUT / "banner.png", optimize=True)


if __name__ == "__main__":
    render()
