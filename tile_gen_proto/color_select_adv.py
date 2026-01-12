#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import math
import tkinter as tk
from PIL import Image, ImageTk

#================================================================================================================================#
#=> - Class: RgbColorWheel
#================================================================================================================================#

class RgbColorWheel:

    def __buildUi (m):
        m.main_frame = tk.Frame(m.root)
        m.main_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)

        m.canvas_frame = tk.Frame(m.main_frame)
        m.canvas_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)

        m.canvas = tk.Canvas(m.canvas_frame, width=m.wheel_size, height=m.wheel_size, bg="black")
        m.canvas.pack(side=tk.LEFT, padx=5, pady=5)
        m.canvas.bind("<Button-1>", m.__onWheelClick)

        m.hwb_size = 600
        m.hwb_canvas = tk.Canvas(m.canvas_frame, width=m.hwb_size, height=m.hwb_size, bg="black")
        m.hwb_canvas.pack(side=tk.LEFT, padx=5, pady=5)
        m.hwb_canvas.bind("<Button-1>", m.__onHwbClick)

        m.info_frame = tk.Frame(m.main_frame)
        m.info_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        m.color_preview = tk.Label(m.info_frame, text="      ", bg="#ffffff", width=30, height=6, relief=tk.SUNKEN)
        m.color_preview.pack(side=tk.LEFT, padx=5)

        m.rgb_entry = tk.Entry(m.info_frame, width=20)
        m.rgb_entry.pack(side=tk.LEFT, padx=5)
        m.rgb_entry.insert(0, "(255, 255, 255)")
        m.rgb_entry.config(state="readonly")

        m.hex_label = tk.Label(m.info_frame, text="#FFFFFF")
        m.hex_label.pack(side=tk.LEFT, padx=5)

        m.brightness_frame = tk.Frame(m.main_frame)
        m.brightness_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        m.brightness_label = tk.Label(m.brightness_frame, text="Brightness:")
        m.brightness_label.pack(side=tk.LEFT, padx=5)

        m.brightness_var = tk.IntVar(value=128)
        m.brightness_scale = tk.Scale(m.brightness_frame, from_=0, to=255, orient=tk.HORIZONTAL, variable=m.brightness_var, command=m.__onBrightnessChange, length=400)
        m.brightness_scale.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)

        all_wheels_exist = True
        for b in range(256):
            if not os.path.exists(m.__getWheelImagePath(b)):
                all_wheels_exist = False
                break
        
        if not all_wheels_exist:
            m.__generateAllWheels()

        all_hwb_exist = True
        for h in range(360):
            if not os.path.exists(m.__getHwbImagePath(h)):
                all_hwb_exist = False
                break
        
        if not all_hwb_exist:
            m.__generateAllHwb()
        
        m.__drawColorWheel()
        m.__drawHwbSquare()

    def __generateWheelImage (m, brightness):
        cx = m.wheel_size // 2
        cy = m.wheel_size // 2
        total_pixels = 0
        for y in range(m.wheel_size):
            for x in range(m.wheel_size):
                dx = x - cx
                dy = y - cy
                dist = math.hypot(dx, dy)
                if dist <= m.radius:
                    total_pixels += 1
        
        img = Image.new("RGB", (m.wheel_size, m.wheel_size), "black")
        pixels = img.load()
        wheel_brightness = brightness / 255.0
        drawn_pixels = 0
        
        for y in range(m.wheel_size):
            for x in range(m.wheel_size):
                dx = x - cx
                dy = y - cy
                dist = math.hypot(dx, dy)
                
                if dist <= m.radius:
                    angle = math.degrees(math.atan2(-dy, dx))
                    if angle < 0:
                        angle += 360
                    hue = angle
                    saturation = min(1.0, dist / m.radius) if m.radius > 0 else 0.0
                    r, g, b = m.__hslToRgb(hue, saturation, wheel_brightness)
                    pixels[x, y] = (r, g, b)
                    drawn_pixels += 1
                    print(f"\rB:{brightness} Drawing pixels: {drawn_pixels}/{total_pixels}", end="", flush=True)
        
        print()
        return img

    def __generateAllWheels (m):
        wheel_dir = "../../color-wheels"
        if not os.path.exists(wheel_dir):
            os.makedirs(wheel_dir)
        
        for brightness in range(256):
            image_path = f"{wheel_dir}/hs{brightness}.png"
            if not os.path.exists(image_path):
                img = m.__generateWheelImage(brightness)
                img.save(image_path)

    def __getWheelImagePath (m, brightness):
        return f"../../color-wheels/hs{brightness}.png"

    def __drawColorWheel (m, brightness=None):
        if brightness is None:
            brightness = m.brightness_var.get()
        
        m.canvas.delete("all")
        image_path = m.__getWheelImagePath(brightness)
        
        if not os.path.exists(image_path):
            m.__generateAllWheels()
        
        img = Image.open(image_path)
        photo = ImageTk.PhotoImage(img)
        m.canvas.create_image(m.wheel_size // 2, m.wheel_size // 2, image=photo, anchor=tk.CENTER)
        m.canvas.image = photo

    def __hslToRgb (m, h, s, l):
        h = h % 360
        s = max(0.0, min(1.0, s))
        l = max(0.0, min(1.0, l))
        c = (1 - abs(2 * l - 1)) * s
        x = c * (1 - abs((h / 60) % 2 - 1))
        m_val = l - c / 2
        if 0 <= h < 60:
            r, g, b = c, x, 0
        elif 60 <= h < 120:
            r, g, b = x, c, 0
        elif 120 <= h < 180:
            r, g, b = 0, c, x
        elif 180 <= h < 240:
            r, g, b = 0, x, c
        elif 240 <= h < 300:
            r, g, b = x, 0, c
        else:
            r, g, b = c, 0, x
        r = int((r + m_val) * 255)
        g = int((g + m_val) * 255)
        b = int((b + m_val) * 255)
        return (r, g, b)

    def __hwbToRgb (m, h, w, b):
        pure_r, pure_g, pure_b = m.__hslToRgb(h, 1.0, 0.5)
        pure_r = pure_r / 255.0
        pure_g = pure_g / 255.0
        pure_b = pure_b / 255.0
        
        r = pure_r + w * (1.0 - pure_r)
        g = pure_g + w * (1.0 - pure_g)
        b_val = pure_b + w * (1.0 - pure_b)
        
        r = r * (1.0 - b)
        g = g * (1.0 - b)
        b_val = b_val * (1.0 - b)
        
        return (int(r * 255), int(g * 255), int(b_val * 255))

    def __getHwbImagePath (m, hue_index):
        wheel_dir = "../../color-wheels"
        return f"{wheel_dir}/sb{hue_index}.png"

    def __generateHwbImage (m, hue_index):
        size = m.hwb_size
        total_pixels = size * size
        img = Image.new("RGB", (size, size), "black")
        pixels = img.load()
        drawn_pixels = 0
        
        for y in range(size):
            for x in range(size):
                whiteness = x / size
                blackness = y / size
                r, g, b = m.__hwbToRgb(hue_index, whiteness, blackness)
                pixels[x, y] = (r, g, b)
                drawn_pixels += 1
                print(f"\rH:{hue_index} Drawing pixels: {drawn_pixels}/{total_pixels}", end="", flush=True)
        
        print()
        return img

    def __generateAllHwb (m):
        for h in range(360):
            image_path = m.__getHwbImagePath(h)
            wheel_dir = os.path.dirname(image_path)
            if not os.path.exists(wheel_dir):
                os.makedirs(wheel_dir)
            if not os.path.exists(image_path):
                img = m.__generateHwbImage(h)
                img.save(image_path)

    def __drawHwbSquare (m):
        m.hwb_canvas.delete("all")
        hue_index = int(round(m.hue)) % 360
        image_path = m.__getHwbImagePath(hue_index)

        wheel_dir = os.path.dirname(image_path)
        if not os.path.exists(wheel_dir):
            os.makedirs(wheel_dir)
        
        if not os.path.exists(image_path):
            m.__generateAllHwb()
        img = Image.open(image_path)
        
        photo = ImageTk.PhotoImage(img)
        m.hwb_canvas.create_image(m.hwb_size // 2, m.hwb_size // 2, image=photo, anchor=tk.CENTER)
        m.hwb_canvas.image = photo
        
        if hasattr(m, 'whiteness') and hasattr(m, 'blackness'):
            marker_x = int(m.whiteness * m.hwb_size)
            marker_y = int(m.blackness * m.hwb_size)
            marker_size = 8
            m.hwb_canvas.create_oval(marker_x - marker_size, marker_y - marker_size, marker_x + marker_size, marker_y + marker_size, outline="white", width=2, fill="black", tags="marker")

    def __onWheelClick (m, event):
        cx = m.wheel_size // 2
        cy = m.wheel_size // 2
        dx = event.x - cx
        dy = event.y - cy
        dist = math.hypot(dx, dy)
        if dist > m.radius:
            return

        angle = math.degrees(math.atan2(-dy, dx))
        if angle < 0:
            angle += 360
        m.hue = angle

        if m.radius == 0:
            m.saturation = 1.0
        else:
            m.saturation = min(1.0, dist / m.radius)

        m.canvas.delete("marker")
        marker_size = 8
        m.canvas.create_oval(event.x - marker_size, event.y - marker_size, event.x + marker_size, event.y + marker_size, outline="white", width=2, fill="black", tags="marker")

        m.__drawHwbSquare()
        m.__updateColor()

    def __onHwbClick (m, event):
        if event.x < 0 or event.x >= m.hwb_size or event.y < 0 or event.y >= m.hwb_size:
            return
        
        m.whiteness = event.x / m.hwb_size
        m.blackness = event.y / m.hwb_size
        m.selected_color = m.__hwbToRgb(m.hue, m.whiteness, m.blackness)
        
        m.hwb_canvas.delete("marker")
        marker_size = 8
        m.hwb_canvas.create_oval(event.x - marker_size, event.y - marker_size, event.x + marker_size, event.y + marker_size, outline="white", width=2, fill="black", tags="marker")
        
        m.__updateLabels()

    def __onBrightnessChange (m, *args):
        brightness = m.brightness_var.get()
        m.__drawColorWheel(brightness)
        m.__updateColor()

    def __updateColor (m):
        brightness = m.brightness_var.get() / 255.0
        m.selected_color = m.__hslToRgb(m.hue, m.saturation, brightness)
        m.__updateLabels()

    def __updateLabels (m):
        r, g, b = m.selected_color
        color_hex = "#%02x%02x%02x" % (r, g, b)
        m.color_preview.config(bg=color_hex)
        m.rgb_entry.config(state="normal")
        m.rgb_entry.delete(0, tk.END)
        m.rgb_entry.insert(0, f"({r}, {g}, {b})")
        m.rgb_entry.config(state="readonly")
        m.hex_label.config(text=color_hex.upper())

    def __init__ (m, root, wheel_size=600):
        m.root = root
        m.wheel_size = wheel_size
        m.radius = wheel_size // 2
        m.hue = 0.0
        m.saturation = 0.0
        m.whiteness = 0.0
        m.blackness = 0.0
        m.selected_color = (255, 255, 255)
        m.__buildUi()
        m.__updateColor()
    
    def getColor (m):
        return m.selected_color

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk ()
    root.title ("RGB Color Wheel")
    wheel = RgbColorWheel (root, wheel_size=600)
    root.mainloop ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#