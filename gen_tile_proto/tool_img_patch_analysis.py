#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
from collections import deque
from PIL import Image
import matplotlib.pyplot as plt

#================================================================================================================================#
#=> - Class: PatchAnalyzer
#================================================================================================================================#

class PatchAnalyzer:

    def __loadImage (m):
        if os.path.exists(m.image_path):
            img = Image.open(m.image_path)
            img_rgb = img.convert('RGB')
            m.image_array = np.array(img_rgb)
        else:
            m.image_array = None

    def __analyzePatches (m):
        if m.image_array is None:
            return
        height, width = m.image_array.shape[:2]
        claimed = np.zeros((height, width), dtype=bool)
        patch_sizes = []
        pixel_count = 0
        for y in range(height):
            for x in range(width):
                if not claimed[y, x]:
                    target_color = tuple(m.image_array[y, x])
                    queue = deque()
                    queue.append((x, y))
                    claimed[y, x] = True
                    pixel_count = 1
                    while len(queue) > 0:
                        cx, cy = queue.popleft()
                        neighbors = [(cx - 1, cy), (cx + 1, cy), (cx, cy - 1), (cx, cy + 1)]
                        for nx, ny in neighbors:
                            if 0 <= nx < width and 0 <= ny < height:
                                if not claimed[ny, nx]:
                                    neighbor_color = tuple(m.image_array[ny, nx])
                                    if neighbor_color == target_color:
                                        claimed[ny, nx] = True
                                        queue.append((nx, ny))
                                        pixel_count += 1
                    patch_sizes.append((pixel_count, target_color))
                    pixel_count = 0
        patch_sizes.sort(key=lambda x: x[0])
        m.patch_sizes = patch_sizes
        print ("*** Found %d patches" %(len(patch_sizes)))
        m.__saveResults()
        m.__displayChart()

    def __saveResults (m):
        if m.image_path.endswith(".png"):
            save_path = m.image_path.replace(".png", "_patches.txt")
        elif m.image_path.endswith(".ppm"):
            save_path = m.image_path.replace(".ppm", "_patches.txt")
        else:
            raise ValueError("Unsupported image format: %s" %(m.image_path))
        with open(save_path, "w") as ptr:
            for size, color in m.patch_sizes:
                ptr.write("%d:%d:%d:%d\n" %(size, color[0], color[1], color[2]))

    def __displayChart (m):
        if len(m.patch_sizes) == 0:
            return
        sizes = [size for size, color in m.patch_sizes]
        plt.figure(figsize=(12, 6))
        plt.bar(range(len(sizes)), sizes)
        plt.xlabel("Patch Index")
        plt.ylabel("Pixel Count")
        plt.title("Patch Size Distribution")
        plt.tight_layout()
        plt.show()

    def __init__ (m, image_path):
        m.image_path = image_path
        m.image_array = None
        m.patch_sizes = []
        m.__loadImage()
        m.__analyzePatches()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print ("*** Error: Usage: %s <image_path>" %(sys.argv[0]))
        sys.exit(1)
    image_path = sys.argv[1]
    
    analyzer = PatchAnalyzer(image_path)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
