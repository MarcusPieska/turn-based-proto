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

lib.SetBuilt.argtypes = [c_int]
lib.SetBuilt.restype = None

lib.ClearBuilt.argtypes = [c_int]
lib.ClearBuilt.restype = None

lib.ToggleResearched.argtypes = [c_int]
lib.ToggleResearched.restype = None

lib.SetResearched.argtypes = [c_int]
lib.SetResearched.restype = None

lib.ClearResearched.argtypes = [c_int]
lib.ClearResearched.restype = None

lib.ToggleUnlocked.argtypes = [c_int]
lib.ToggleUnlocked.restype = None

lib.SetUnlocked.argtypes = [c_int]
lib.SetUnlocked.restype = None

lib.ClearUnlocked.argtypes = [c_int]
lib.ClearUnlocked.restype = None

lib.IsBuilt.argtypes = [c_int]
lib.IsBuilt.restype = c_int

lib.IsResearched.argtypes = [c_int]
lib.IsResearched.restype = c_int

lib.IsUnlocked.argtypes = [c_int]
lib.IsUnlocked.restype = c_int

lib.IsBuildable.argtypes = [c_int]
lib.IsBuildable.restype = c_int

lib.ValidateAndCount.argtypes = [ctypes.c_char_p]
lib.ValidateAndCount.restype = c_int

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def print_bit_array(researched_flags, unlocked_flags, built_flags, count):
    print("Researched flags:")
    bits_str = ""
    for i in range(count):
        bits_str += "1" if researched_flags[i] else "0"
        if (i + 1) % 10 == 0:
            bits_str += " "
        if (i + 1) % 50 == 0:
            print(bits_str)
            bits_str = ""
    if bits_str:
        print(bits_str)
    
    print("\nUnlocked flags:")
    bits_str = ""
    for i in range(count):
        bits_str += "1" if unlocked_flags[i] else "0"
        if (i + 1) % 10 == 0:
            bits_str += " "
        if (i + 1) % 50 == 0:
            print(bits_str)
            bits_str = ""
    if bits_str:
        print(bits_str)
    
    print("\nBuilt flags:")
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
        researched_flags = []
        unlocked_flags = []
        built_flags = []
        for i in range(count):
            researched_flags.append(lib.IsResearched(i) == 1)
            unlocked_flags.append(lib.IsUnlocked(i) == 1)
            built_flags.append(lib.IsBuilt(i) == 1)
        print_bit_array(researched_flags, unlocked_flags, built_flags, count)
        m.refresh_buildings()

    def __toggle_researched(m, index):
        lib.ToggleResearched(index)
        m.refresh_buildings()

    def __toggle_unlocked(m, index):
        lib.ToggleUnlocked(index)
        m.refresh_buildings()

    def __toggle_built(m, index):
        lib.ToggleBuilt(index)
        m.refresh_buildings()

    def refresh_buildings(m):
        count = lib.GetBuildingCount()
        for i in range(count):
            data = BuildingDataC()
            lib.GetBuilding(i, ctypes.byref(data))
            name = data.name.decode('utf-8')
            
            researched = lib.IsResearched(i) == 1
            unlocked = lib.IsUnlocked(i) == 1
            built = lib.IsBuilt(i) == 1
            buildable = lib.IsBuildable(i) == 1
            
            if i < len(m.building_rows):
                row = m.building_rows[i]
                row["name_label"].config(text=name)
                row["researched_light"].config(bg="green" if researched else "red")
                row["unlocked_light"].config(bg="green" if unlocked else "red")
                row["built_light"].config(bg="green" if built else "red")
                status = " [BUILDABLE]" if buildable else ""
                row["name_label"].config(text=name + status)
            else:
                row_frm = tk.Frame(m.scroll_frame)
                row_frm.pack(fill=tk.X, padx=5, pady=2)
                
                name_label = tk.Label(row_frm, text=name, width=35, anchor="w")
                name_label.pack(side=tk.LEFT, padx=5)
                
                btn = tk.Button(row_frm, text="Toggle Researched", command=lambda idx=i: m.__toggle_researched(idx), width=18)
                btn.pack(side=tk.LEFT, padx=2)
                
                btn = tk.Button(row_frm, text="Toggle Unlocked", command=lambda idx=i: m.__toggle_unlocked(idx), width=18)
                btn.pack(side=tk.LEFT, padx=2)
                
                btn = tk.Button(row_frm, text="Toggle Built", command=lambda idx=i: m.__toggle_built(idx), width=15)
                btn.pack(side=tk.LEFT, padx=2)
                
                researched_light = tk.Label(row_frm, text="R", width=3, bg="red")
                researched_light.pack(side=tk.LEFT, padx=2)
                
                unlocked_light = tk.Label(row_frm, text="U", width=3, bg="red")
                unlocked_light.pack(side=tk.LEFT, padx=2)
                
                built_light = tk.Label(row_frm, text="B", width=3, bg="red")
                built_light.pack(side=tk.LEFT, padx=2)
                
                m.building_rows.append({
                    "name_label": name_label,
                    "researched_light": researched_light,
                    "unlocked_light": unlocked_light,
                    "built_light": built_light,
                })
        
        while len(m.building_rows) > count:
            m.building_rows[-1]["name_label"].master.destroy()
            m.building_rows.pop()

    def __init__(m, root):
        m.root = root
        m.building_rows = []
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        load_data_btn = tk.Button(m.top_frame, text="Load Building Data", command=m.__load_building_data, width=18)
        load_data_btn.pack(side=tk.LEFT, padx=5)
        
        save_btn = tk.Button(m.top_frame, text="Save", command=m.__save_building_vector, width=15)
        save_btn.pack(side=tk.LEFT, padx=5)
        
        load_btn = tk.Button(m.top_frame, text="Load", command=m.__load_building_vector, width=15)
        load_btn.pack(side=tk.LEFT, padx=5)
        
        m.canvas = tk.Canvas(root)
        m.scrollbar = tk.Scrollbar(root, orient="vertical", command=m.canvas.yview)
        m.scroll_frame = tk.Frame(m.canvas)
        
        m.scroll_frame.bind("<Configure>", lambda e: m.canvas.configure(scrollregion=m.canvas.bbox("all")))
        
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
    root.geometry("1200x800")
    gui = BuildingVectorGUI(root)
    root.mainloop()
    lib.DestroyBuildingVector()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
