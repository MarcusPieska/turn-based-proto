#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

from PIL import Image

#================================================================================================================================#
#=> - Functions -
#================================================================================================================================#

def convertPngToPpm (image_path):
    if not os.path.exists(image_path):
        print ("*** Error: Image file not found: %s" %(image_path))
        return
    img = Image.open(image_path)
    img_rgb = img.convert('RGB')
    width, height = img_rgb.size
    output_path = image_path.replace(".png", ".ppm")
    if output_path == image_path:
        base, ext = os.path.splitext(image_path)
        output_path = base + ".ppm"
    with open(output_path, "wb") as f:
        f.write(b"P6\n")
        f.write(("%d %d\n" %(width, height)).encode())
        f.write(b"255\n")
        for y in range(height):
            for x in range(width):
                r, g, b = img_rgb.getpixel((x, y))
                f.write(bytes([r, g, b]))
    print ("*** Saved PPM image to %s" %(output_path))

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print ("*** Error: Usage: %s <image_path>" %(sys.argv[0]))
        sys.exit(1)
    convertPngToPpm (sys.argv[1])

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
