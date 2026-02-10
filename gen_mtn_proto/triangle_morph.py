#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
import math
from PIL import Image

#================================================================================================================================#
#=> - Class: TriangleMorpher
#================================================================================================================================#

class TriangleMorpher:

    def __init__ (m):
        pass

    def morphTriangle (m, source_image, source_triangle, pt_base1, pt_base2, pt_peak, output_width, output_height):
        if len(source_image.shape) == 3:
            is_rgb = True
            num_channels = source_image.shape[2]
        else:
            is_rgb = False
            num_channels = 1
        
        tri_adj = (pt_base1, pt_base2, pt_peak)
        
        if is_rgb:
            if num_channels == 4:
                image = np.zeros((output_height, output_width, 4), dtype=np.uint8)
            else:
                image = np.zeros((output_height, output_width, 3), dtype=np.uint8)
        else:
            image = np.zeros((output_height, output_width), dtype=np.uint8)
        
        v1_s, v2_s, v3_s = source_triangle
        v1_d, v2_d, v3_d = tri_adj
        
        min_x = int(max(0, min(v[0] for v in tri_adj)))
        max_x = int(min(output_width - 1, max(v[0] for v in tri_adj)))
        min_y = int(max(0, min(v[1] for v in tri_adj)))
        max_y = int(min(output_height - 1, max(v[1] for v in tri_adj)))
        
        for y in range(min_y, max_y + 1):
            for x in range(min_x, max_x + 1):
                if m.__pointInTriangle(x, y, v1_d, v2_d, v3_d):
                    bary = m.__barycentric(x, y, v1_d, v2_d, v3_d)
                    if bary[0] >= 0 and bary[1] >= 0 and bary[2] >= 0:
                        src_x = int(bary[0] * v1_s[0] + bary[1] * v2_s[0] + bary[2] * v3_s[0])
                        src_y = int(bary[0] * v1_s[1] + bary[1] * v2_s[1] + bary[2] * v3_s[1])
                        
                        if 0 <= src_x < source_image.shape[1] and 0 <= src_y < source_image.shape[0]:
                            if is_rgb:
                                image[y, x] = source_image[src_y, src_x]
                            else:
                                image[y, x] = source_image[src_y, src_x]
        
        return image

    def __barycentric (m, x, y, v1, v2, v3):
        v0x, v0y = v2[0] - v1[0], v2[1] - v1[1]
        v1x, v1y = v3[0] - v1[0], v3[1] - v1[1]
        v2x, v2y = x - v1[0], y - v1[1]
        dot00 = v0x * v0x + v0y * v0y
        dot01 = v0x * v1x + v0y * v1y
        dot02 = v0x * v2x + v0y * v2y
        dot11 = v1x * v1x + v1y * v1y
        dot12 = v1x * v2x + v1y * v2y
        inv_denom = 1 / (dot00 * dot11 - dot01 * dot01)
        u = (dot11 * dot02 - dot01 * dot12) * inv_denom
        v = (dot00 * dot12 - dot01 * dot02) * inv_denom
        return (1 - u - v, u, v)

    def __pointInTriangle (m, x, y, v1, v2, v3):
        def sign(p1, p2, p3):
            return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1])
        d1 = sign((x, y), v1, v2)
        d2 = sign((x, y), v2, v3)
        d3 = sign((x, y), v3, v1)
        has_neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
        has_pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
        return not (has_neg and has_pos)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
