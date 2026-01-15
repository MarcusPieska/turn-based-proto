#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk

#================================================================================================================================#
#=> - DialogGetInts class -
#================================================================================================================================#

class DialogGetInts:

    def __setup_ui (m):
        main_frame = tk.Frame(m.dialog, padx=10, pady=10)
        main_frame.pack()
        m.frames = []
        m.entries = []
        m.sliders = []
        m.labels = []
        m.updating = False
        for i, (min_val, default_val, max_val) in enumerate(m.int_triplets):
            if default_val < min_val:
                default_val = min_val
            elif default_val > max_val:
                default_val = max_val
            default_val = int(default_val)
            frame = tk.Frame(main_frame)
            frame.pack(pady=5, fill=tk.X)
            m.frames.append(frame)
            entry = tk.Entry(frame, width=10)
            entry.pack(side=tk.LEFT, padx=5)
            entry.insert(0, str(default_val))
            entry.bind("<KeyRelease>", lambda e, idx=i: m.__on_entry_change(idx))
            m.entries.append(entry)
            slider = tk.Scale(frame, from_=min_val, to=max_val, orient=tk.HORIZONTAL, length=200)
            slider.set(default_val)
            slider.pack(side=tk.LEFT, padx=5)
            slider.configure(command=lambda val, idx=i: m.__on_slider_change(idx, val))
            m.sliders.append(slider)
            label = tk.Label(frame, text=str(default_val), width=10)
            label.pack(side=tk.LEFT, padx=5)
            m.labels.append(label)
        btn_ok = tk.Button(main_frame, text="OK", width=10, command=m.__on_ok)
        btn_ok.pack(pady=10)

    def __on_entry_change (m, idx):
        if m.updating:
            return
        try:
            val_str = m.entries[idx].get()
            val = int(val_str)
            min_val, default_val, max_val = m.int_triplets[idx]
            if val < min_val:
                val = min_val
            elif val > max_val:
                val = max_val
            m.values[idx] = int(val)
            m.updating = True
            m.sliders[idx].set(val)
            m.labels[idx].config(text=str(val))
            m.entries[idx].delete(0, tk.END)
            m.entries[idx].insert(0, str(val))
            m.updating = False
        except (ValueError, TypeError):
            pass

    def __on_slider_change (m, idx, val_str):
        if m.updating:
            return
        try:
            val = int(float(val_str))
            m.values[idx] = int(val)
            m.updating = True
            m.entries[idx].delete(0, tk.END)
            m.entries[idx].insert(0, str(val))
            m.labels[idx].config(text=str(val))
            m.updating = False
        except (ValueError, TypeError):
            pass

    def __on_ok (m):
        m.dialog.destroy()

    def __on_close (m):
        m.dialog.destroy()

    def __init__ (m, parent, int_triplets):
        m.parent = parent
        m.int_triplets = int_triplets
        m.values = []
        for min_val, default_val, max_val in int_triplets:
            if default_val < min_val:
                default_val = min_val
            elif default_val > max_val:
                default_val = max_val
            m.values.append(int(default_val))
        m.dialog = tk.Toplevel(parent)
        m.dialog.title("Get Integers")
        m.dialog.transient(parent)
        m.dialog.grab_set()
        m.dialog.protocol("WM_DELETE_WINDOW", m.__on_close)
        m.__setup_ui()
        m.dialog.wait_window()

    def get_values (m):
        return [int(v) for v in m.values]

#================================================================================================================================#
#=> - End -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    dialog = DialogGetInts(root, [(100, 100, 200), (100, 100, 200)])
    values = dialog.get_values()
    print(values)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
