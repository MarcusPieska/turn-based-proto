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

class BuildingDataC(Structure):
    _fields_ = [
        ("name", c_char * 256),
        ("cost", c_int),
        ("effect", c_char * 256),
        ("exists", c_int),
    ]

try:
    lib = CDLL("./libbuilding_vector.so")
except OSError:
    try:
        lib = CDLL("./building_vector.dll")
    except OSError:
        lib = CDLL("./libbuilding_vector.dylib")

lib.InitBuildingData.argtypes = [ctypes.c_char_p]
lib.InitBuildingData.restype = None

lib.DestroyBuildingVector.argtypes = []
lib.DestroyBuildingVector.restype = None

lib.GetBuildingCount.argtypes = []
lib.GetBuildingCount.restype = c_int

lib.GetBuilding.argtypes = [c_int, POINTER(BuildingDataC)]
lib.GetBuilding.restype = None

lib.SaveBuildingVector.argtypes = [ctypes.c_char_p]
lib.SaveBuildingVector.restype = None

lib.LoadBuildingVector.argtypes = [ctypes.c_char_p]
lib.LoadBuildingVector.restype = None

lib.ToggleBuilt.argtypes = [c_int]
lib.ToggleBuilt.restype = None

lib.SetAvailable.argtypes = [c_int]
lib.SetAvailable.restype = None

lib.ValidateAndCount.argtypes = [ctypes.c_char_p]
lib.ValidateAndCount.restype = c_int

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def print_bit_array(built_flags, available_flags, count):
    print("Built flags:")
    bits_str = ""
    for i in range(count):
        bits_str += "1" if built_flags[i] else "0"
        if (i + 1) % 10 == 0:
            bits_str += " "
        if (i + 1) % 50 == 0:
            print(bits_str)
            bits_str = ""
    if bits_str:
        print(bits_str)
    
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
#=> - Class: BuildingVectorGUI -
#================================================================================================================================#

class BuildingVectorGUI:

    def __load_building_data(m):
        filename = "../game_config.buildings"
        lib.InitBuildingData(filename.encode('utf-8'))
        m.refresh_buildings()

    def __save_building_vector(m):
        filename = "building_vector_save.temp"
        lib.SaveBuildingVector(filename.encode('utf-8'))
        print("Building vector saved to %s" % filename)

    def __load_building_vector(m):
        filename = "building_vector_save.temp"
        lib.LoadBuildingVector(filename.encode('utf-8'))
        count = lib.GetBuildingCount()
        built_flags = []
        available_flags = []
        for i in range(count):
            data = BuildingDataC()
            lib.GetBuilding(i, ctypes.byref(data))
            built_flags.append(data.exists == 1)
            if i < len(m.available_states):
                available_flags.append(m.available_states[i])
            else:
                available_flags.append(False)
        print_bit_array(built_flags, available_flags, count)
        m.refresh_buildings()

    def __toggle_available(m, index):
        lib.SetAvailable(index)
        if index < len(m.available_states):
            m.available_states[index] = not m.available_states[index]
        m.refresh_buildings()

    def __toggle_built(m, index):
        lib.ToggleBuilt(index)
        m.refresh_buildings()

    def refresh_buildings(m):
        count = lib.GetBuildingCount()
        built_flags = []
        for i in range(count):
            data = BuildingDataC()
            lib.GetBuilding(i, ctypes.byref(data))
            name = data.name.decode('utf-8')
            exists = data.exists == 1
            built_flags.append(exists)
            
            if i < len(m.building_rows):
                row = m.building_rows[i]
                row["name_label"].config(text=name)
                row["built_light"].config(bg="green" if exists else "red")
                if i < len(m.available_states):
                    row["available_light"].config(bg="green" if m.available_states[i] else "red")
            else:
                row_frame = tk.Frame(m.scroll_frame)
                row_frame.pack(fill=tk.X, padx=5, pady=2)
                
                name_label = tk.Label(row_frame, text=name, width=30, anchor="w")
                name_label.pack(side=tk.LEFT, padx=5)
                
                available_btn = tk.Button(row_frame, text="Toggle Available", 
                                        command=lambda idx=i: m.__toggle_available(idx), width=15)
                available_btn.pack(side=tk.LEFT, padx=5)
                
                built_btn = tk.Button(row_frame, text="Toggle Built", 
                                     command=lambda idx=i: m.__toggle_built(idx), width=15)
                built_btn.pack(side=tk.LEFT, padx=5)
                
                available_light = tk.Label(row_frame, text="A", width=3, bg="red")
                available_light.pack(side=tk.LEFT, padx=5)
                
                built_light = tk.Label(row_frame, text="B", width=3, bg="red")
                built_light.pack(side=tk.LEFT, padx=5)
                
                m.building_rows.append({
                    "name_label": name_label,
                    "available_light": available_light,
                    "built_light": built_light,
                })
                if i >= len(m.available_states):
                    m.available_states.append(False)
        
        while len(m.building_rows) > count:
            m.building_rows[-1]["name_label"].master.destroy()
            m.building_rows.pop()
            if len(m.available_states) > count:
                m.available_states.pop()

    def __init__(m, root):
        m.root = root
        m.building_rows = []
        m.available_states = []
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        load_data_btn = tk.Button(m.top_frame, text="Load Building Data", 
                                  command=m.__load_building_data, width=18)
        load_data_btn.pack(side=tk.LEFT, padx=5)
        
        save_btn = tk.Button(m.top_frame, text="Save", 
                            command=m.__save_building_vector, width=15)
        save_btn.pack(side=tk.LEFT, padx=5)
        
        load_btn = tk.Button(m.top_frame, text="Load", 
                            command=m.__load_building_vector, width=15)
        load_btn.pack(side=tk.LEFT, padx=5)
        
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
        
        m.__load_building_data()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Building Vector Tester")
    root.geometry("800x600")
    gui = BuildingVectorGUI(root)
    root.mainloop()
    lib.DestroyBuildingVector()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
