#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

from PIL import Image, ImageFilter

#================================================================================================================================#
#=> - Functions -
#================================================================================================================================#

def applyGaussianBlur (image_path, blur_radius=2):
    if not os.path.exists(image_path):
        print ("*** Error: Image file not found: %s" %(image_path))
        return
    img = Image.open(image_path)
    img_rgb = img.convert('RGB')
    blurred = img_rgb.filter(ImageFilter.GaussianBlur(radius=blur_radius))
    output_path = image_path.replace(".png", "_blurred.png").replace(".ppm", "_blurred.png").replace(".jpg", "_blurred.png")
    if output_path == image_path:
        base, ext = os.path.splitext(image_path)
        output_path = base + "_blurred" + ext
    blurred.save(output_path)
    print ("*** Saved blurred image to %s" %(output_path))

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print ("*** Error: Usage: %s <image_path>" %(sys.argv[0]))
        sys.exit(1)
    applyGaussianBlur (sys.argv[1], blur_radius=1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
