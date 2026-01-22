#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
from PIL import Image

#================================================================================================================================#
#=> - Class: HeightDataAdvAlpha
#================================================================================================================================#

class HeightDataAdvAlpha:

    MTN_BASE_GREEN = (19, 102, 41)

    def __derive_alpha (m, gray, lim1, lim2):
        new_alpha = np.zeros_like(gray, dtype=np.float32)
        mask_low = gray <= lim1
        mask_mid = (gray > lim1) & (gray < lim2)
        mask_high = gray >= lim2
        new_alpha[mask_low] = 0.0
        new_alpha[mask_mid] = ((gray[mask_mid].astype(np.float32) - lim1) / (lim2 - lim1)) * 255.0
        new_alpha[mask_high] = 255.0
        new_alpha = new_alpha.astype(np.uint8)
        return new_alpha

    def __shift_to_green (m, img_array, lim1, lim2):
        fac_r, fac_g, fac_b = 0.2, 1.0, 0.4
        if len(img_array.shape) == 2:
            gray = img_array
        else:
            gray = img_array[:, :, 0]
        result = img_array.copy().astype(np.float32)
        mask_low = gray < lim1
        mask_mid = (gray >= lim1) & (gray < lim2)
        if len(img_array.shape) == 2:
            result[mask_low] = result[mask_low] * fac_r
            if np.any(mask_mid):
                r_factor_mid = fac_r + ((gray[mask_mid].astype(np.float32) - lim1) / (lim2 - lim1)) * (1.0 - fac_r)
                result[mask_mid] = result[mask_mid] * r_factor_mid
        else:
            result[mask_low, 0] = result[mask_low, 0] * fac_r
            result[mask_low, 2] = result[mask_low, 2] * fac_b
            if np.any(mask_mid):
                r_factor_mid = fac_r + ((gray[mask_mid].astype(np.float32) - lim1) / (lim2 - lim1)) * (1.0 - fac_r)
                b_factor_mid = fac_b + ((gray[mask_mid].astype(np.float32) - lim1) / (lim2 - lim1)) * (1.0 - fac_b)
                result[mask_mid, 0] = result[mask_mid, 0] * r_factor_mid
                result[mask_mid, 2] = result[mask_mid, 2] * b_factor_mid
        m.rgb = result.astype(np.uint8)

    def __combine_alpha (m):
        m.combined_alpha = np.minimum(m.orig_alpha.astype(np.float32), m.new_alpha.astype(np.float32)).astype(np.uint8)

    def __convert_to_brown (m):
        h, w = m.gray.shape
        m.rgb = np.zeros((h, w, 3), dtype=np.uint8)
        m.rgb[:, :, 0] = m.gray
        m.rgb[:, :, 1] = (m.gray.astype(np.float32) / 1.37).astype(np.uint8)
        m.rgb[:, :, 2] = (m.gray.astype(np.float32) / 2.13).astype(np.uint8)

    def __get_result (m):
        h, w = m.rgb.shape[:2]
        rgba = np.zeros((h, w, 4), dtype=np.uint8)
        rgba[:, :, 0] = m.rgb[:, :, 0]
        rgba[:, :, 1] = m.rgb[:, :, 1]
        rgba[:, :, 2] = m.rgb[:, :, 2]
        rgba[:, :, 3] = m.combined_alpha
        return rgba

    def __save_image (m, output_path):
        rgba = m.__get_result()
        img = Image.fromarray(rgba, mode='RGBA')
        img.save(output_path)

    def __init__ (m, save_path, image_path=None, lim1=0, lim2=50):
        image_paths = []
        if image_path is not None:
            image_paths.append(image_path)
        else:
            for file in os.listdir(save_path):
                file_name = file.split("/")[-1]
                if "_adv_alpha" in file_name:
                    continue
                if file_name.endswith(".png") and "pv" in file_name and file_name.startswith("raw_mtn_hd"):
                    image_paths.append(os.path.join(save_path, file))

        for image_path in image_paths:
            img_array = np.array(Image.open(image_path))
            m.gray = img_array[:, :, 0]
            m.orig_alpha = img_array[:, :, 3]
            m.new_alpha = m.__derive_alpha(m.gray, 0, 20)
            m.__combine_alpha()
            m.__convert_to_brown()
            #m.__shift_to_green(m.rgb, 4, 100)
            m.__save_image(image_path.replace(".png", "_adv_alpha1.png"))

            m.new_alpha = m.__derive_alpha(m.gray, 30, 50)
            m.__combine_alpha()
            m.__convert_to_brown()
            #m.__shift_to_green(m.rgb, 34, 70)
            m.__save_image(image_path.replace(".png", "_adv_alpha2.png"))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#

if __name__ == "__main__":
    save_path = "../../img-content/"
    filename = "raw_mtn_hd006_pv3.png"
    adv_alpha = HeightDataAdvAlpha(save_path)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
