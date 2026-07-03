# src/selector_gui.py
import tkinter as tk
from tkinter import ttk


def select_files_interactively(files_list: list[dict]) -> list[dict]:
    """Opens a Tkinter GUI with checkboxes for easy file selection.

    Falls back to original list if GUI initialization is not possible.
    """
    try:
        root = tk.Tk()
    except Exception as e:
        print(
            f"\n[Warning] Unable to initialize GUI (headless/no display). "
            f"Saving all files as active. Error: {e}"
        )
        return files_list

    root.title("AI Code Serializer - Select Files")
    root.geometry("650x550")

    # Responsive grid layout
    root.rowconfigure(0, weight=1)
    root.columnconfigure(0, weight=1)

    # Main container
    main_frame = ttk.Frame(root, padding="15")
    main_frame.grid(row=0, column=0, sticky="nsew")
    main_frame.rowconfigure(1, weight=1)
    main_frame.columnconfigure(0, weight=1)

    # Header label
    header = ttk.Label(
        main_frame,
        text="Check the files you want to pass to the AI model:",
        font=("Arial", 11, "bold"),
    )
    header.grid(row=0, column=0, pady=(0, 10), sticky="w")

    # Scrollable Frame Construction
    canvas = tk.Canvas(main_frame, highlightthickness=0)
    scrollbar = ttk.Scrollbar(
        main_frame, orient="vertical", command=canvas.yview
    )
    scrollable_frame = ttk.Frame(canvas)

    scrollable_frame.bind(
        "<Configure>",
        lambda e: canvas.configure(scrollregion=canvas.bbox("all")),
    )

    canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
    canvas.configure(yscrollcommand=scrollbar.set)

    canvas.grid(row=1, column=0, sticky="nsew")
    scrollbar.grid(row=1, column=1, sticky="ns")

    # Mouse Wheel Scrolling Bindings
    def _on_mousewheel(event):
        # Supports Windows, macOS, and Linux scrolling behaviors
        if event.num == 4:
            canvas.yview_scroll(-1, "units")
        elif event.num == 5:
            canvas.yview_scroll(1, "units")
        else:
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")

    canvas.bind_all("<MouseWheel>", _on_mousewheel)
    canvas.bind_all("<Button-4>", _on_mousewheel)
    canvas.bind_all("<Button-5>", _on_mousewheel)

    # Draw file checkboxes
    checkbox_vars = []
    for file_entry in files_list:
        var = tk.BooleanVar(value=file_entry["include"])
        checkbox_vars.append((file_entry, var))

        cb = ttk.Checkbutton(
            scrollable_frame, text=file_entry["relative_path"], variable=var
        )
        cb.pack(anchor="w", pady=3, padx=5)

    # Footer Buttons Layout
    btn_frame = ttk.Frame(main_frame, padding="10")
    btn_frame.grid(row=2, column=0, columnspan=2, sticky="ew")

    def select_all():
        for _, var in checkbox_vars:
            var.set(True)

    def select_none():
        for _, var in checkbox_vars:
            var.set(False)

    def confirm():
        for entry, var in checkbox_vars:
            entry["include"] = var.get()
        root.destroy()

    btn_all = ttk.Button(btn_frame, text="Select All", command=select_all)
    btn_all.pack(side="left", padx=5)

    btn_none = ttk.Button(btn_frame, text="Select None", command=select_none)
    btn_none.pack(side="left", padx=5)

    btn_confirm = ttk.Button(btn_frame, text="Save & Close", command=confirm)
    btn_confirm.pack(side="right", padx=5)

    def on_closing():
        # Keep existing choices if window is closed via the X button
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_closing)
    root.mainloop()

    return files_list