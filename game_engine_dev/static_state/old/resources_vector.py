#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import tkinter as tk
import ctypes
from ctypes import Structure, c_char, c_int, POINTER, CDLL

#================================================================================================================================#
#=> - Ctypes setup -
#================================================================================================================================#

class ResourceDataC(Structure):
    _fields_ = [
        ("name", c_char * 256),
        ("food", c_int),
        ("shields", c_int),
        ("income", c_int),
    ]

try:
    lib = CDLL("./libresources_vector.so")
except OSError:
    try:
        lib = CDLL("./resources_vector.dll")
    except OSError:
        lib = CDLL("./libresources_vector.dylib")

lib.InitResourceData.argtypes = [ctypes.c_char_p]
lib.InitResourceData.restype = None

lib.DestroyResourceVector.argtypes = []
lib.DestroyResourceVector.restype = None

lib.GetResourceCount.argtypes = []
lib.GetResourceCount.restype = c_int

lib.GetResource.argtypes = [c_int, POINTER(ResourceDataC)]
lib.GetResource.restype = None

lib.SetAvailable.argtypes = [c_int]
lib.SetAvailable.restype = None

lib.ValidateAndCount.argtypes = [ctypes.c_char_p]
lib.ValidateAndCount.restype = c_int

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def print_bit_array(available_flags, count):
    print("\nAvailable flags:")
    bits_str = ""
    for i in range(count):
        bits_str += "1" if available_flags[i] else "0"
        if (i + 1) % 10 == 0:
            bits_str += " "
        if (i + 1) % 50 == 0:
            print(bits_str)
            bits_str = ""
    if bits_str:
        print(bits_str)
    print()

#================================================================================================================================#
#=> - Class: ResourceVectorGUI -
#================================================================================================================================#

class ResourceVectorGUI:

    def __load_resource_data(m):
        filename = "../game_config.resources"
        lib.InitResourceData(filename.encode('utf-8'))
        m.refresh_resources()

    def __toggle_available(m, index):
        lib.SetAvailable(index)
        if index < len(m.available_states):
            m.available_states[index] = not m.available_states[index]
        m.refresh_resources()

    def refresh_resources(m):
        count = lib.GetResourceCount()
        
        while len(m.available_states) < count:
            m.available_states.append(False)
        while len(m.available_states) > count:
            m.available_states.pop()

        for i in range(count):
            data = ResourceDataC()
            lib.GetResource(i, ctypes.byref(data))
            name = data.name.decode('utf-8')
            
            if i < len(m.resource_rows):
                row = m.resource_rows[i]
                row["name_label"].config(text=name)
                row["stats_label"].config(text="F:%d S:%d I:%d" % (data.food, data.shields, data.income))
                
                is_available = m.available_states[i]
                row["available_light"].config(bg="green" if is_available else "red")
            else:
                row_frame = tk.Frame(m.scroll_frame)
                row_frame.pack(fill=tk.X, padx=5, pady=2)
                
                name_label = tk.Label(row_frame, text=name, width=30, anchor="w")
                name_label.pack(side=tk.LEFT, padx=5)
                
                stats_label = tk.Label(row_frame, text="F:%d S:%d I:%d" % (data.food, data.shields, data.income), width=15)
                stats_label.pack(side=tk.LEFT, padx=5)
                
                available_btn = tk.Button(row_frame, text="Toggle Available", 
                                        command=lambda idx=i: m.__toggle_available(idx), width=15)
                available_btn.pack(side=tk.LEFT, padx=5)
                
                is_available = m.available_states[i]
                available_light = tk.Label(row_frame, text="A", width=3, bg="green" if is_available else "red")
                available_light.pack(side=tk.LEFT, padx=5)
                
                m.resource_rows.append({
                    "name_label": name_label,
                    "stats_label": stats_label,
                    "available_light": available_light,
                })
        
        while len(m.resource_rows) > count:
            m.resource_rows[-1]["name_label"].master.destroy()
            m.resource_rows.pop()

    def __init__(m, root):
        m.root = root
        m.resource_rows = []
        m.available_states = []
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        load_data_btn = tk.Button(m.top_frame, text="Load Resource Data", 
                                  command=m.__load_resource_data, width=18)
        load_data_btn.pack(side=tk.LEFT, padx=5)
        
        m.canvas = tk.Canvas(root)
        m.scrollbar = tk.Scrollbar(root, orient="vertical", command=m.canvas.yview)
        m.scroll_frame = tk.Frame(m.canvas)
        
        m.scroll_frame.bind(
            "<Configure>",
            lambda e: m.canvas.configure(scrollregion=m.canvas.bbox("all"))
        )
        
        m.canvas.create_window((0, 0), window=m.scroll_frame, anchor="nw")
        m.canvas.configure(yscrollcommand=m.scrollbar.set)
        
        m.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.scrollbar.pack(side=tk.RIGHT, fill=tk.Y, padx=5, pady=5)
        
        m.__load_resource_data()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Resource Vector Tester")
    root.geometry("900x600")
    gui = ResourceVectorGUI(root)
    root.mainloop()
    lib.DestroyResourceVector()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
