# src/selector_gui.py
import tkinter as tk
from tkinter import ttk
from pathlib import Path
import config

CHECKED_SYM = "☑ "
UNCHECKED_SYM = "☐ "


class TreeNode:
    """Represents a node in the directory tree structure."""

    def __init__(
        self,
        name: str,
        relative_path: str,
        is_dir: bool,
        checked: bool = True,
        parent=None,
    ):
        self.name = name
        self.relative_path = relative_path
        self.is_dir = is_dir
        self.checked = checked
        self.parent = parent
        self.children = {}
        self.tree_item_id = None


def build_tree_from_flat_list(files_list: list[dict]) -> TreeNode:
    """Builds a nested TreeNode tree from flat manifest relative paths."""
    root_node = TreeNode(name="root", relative_path="", is_dir=True)

    for file_entry in files_list:
        # Normalize the incoming path to POSIX format (forward slashes)
        rel_path_raw = file_entry["relative_path"]
        rel_path = Path(rel_path_raw).as_posix()
        checked = file_entry["include"]

        parts = Path(rel_path).parts
        current = root_node
        accumulated_path = []

        for i, part in enumerate(parts):
            accumulated_path.append(part)
            part_path = "/".join(accumulated_path)
            is_last = i == len(parts) - 1

            if part not in current.children:
                is_dir = not is_last
                current.children[part] = TreeNode(
                    name=part,
                    relative_path=part_path,
                    is_dir=is_dir,
                    checked=checked if is_last else True,
                    parent=current,
                )
            current = current.children[part]

    align_parent_states(root_node)
    return root_node


def align_parent_states(node: TreeNode) -> bool:
    """Traverses down and assigns parent checkboxes based on children selection states."""
    if not node.is_dir:
        return node.checked

    if not node.children:
        return node.checked

    # Folder is marked fully checked only if all nested children are checked
    all_checked = all(
        align_parent_states(child) for child in node.children.values()
    )
    node.checked = all_checked
    return all_checked


def get_checked_status(node: TreeNode, result_dict: dict):
    """Recursively crawls the tree to collect file inclusion statuses."""
    if not node.is_dir:
        result_dict[node.relative_path] = node.checked
    for child in node.children.values():
        get_checked_status(child, result_dict)


def set_checked_descendants(tree: ttk.Treeview, node: TreeNode, state: bool):
    """Recursively updates checkbox visual and state values downward."""
    node.checked = state
    sym = CHECKED_SYM if state else UNCHECKED_SYM
    tree.item(node.tree_item_id, text=f"{sym}{node.name}")

    for child in node.children.values():
        set_checked_descendants(tree, child, state)


def update_parent_state_upward(tree: ttk.Treeview, parent_node: TreeNode):
    """Recursively updates directory checkboxes upward."""
    if not parent_node or parent_node.name == "root":
        return

    # Checked only if all immediate children are checked
    all_checked = all(
        child.checked for child in parent_node.children.values()
    )

    if parent_node.checked != all_checked:
        parent_node.checked = all_checked
        sym = CHECKED_SYM if all_checked else UNCHECKED_SYM
        tree.item(parent_node.tree_item_id, text=f"{sym}{parent_node.name}")

    # Walk up to the next parent directory
    update_parent_state_upward(tree, parent_node.parent)


def select_files_interactively(files_list: list[dict]) -> list[dict]:
    """Launches an interactive directory tree layout for selecting files."""
    try:
        root = tk.Tk()
    except Exception as e:
        print(
            f"\n[Warning] Unable to initialize GUI (headless/no display). "
            f"Using manifest file as-is. Error: {e}"
        )
        return files_list

    # --- ADDED STYLING CONFIGURATION ---
    style = ttk.Style()
    style.theme_use("clam")  # Ensures style modifications apply cleanly

    # Calculate proportional row height to prevent text clipping
    row_height = int(config.GUI_FONT_SIZE_TREE * 2.2)

    # Configure global Treeview and Button styles using config values
    style.configure(
        "Treeview",
        font=(config.GUI_FONT_FAMILY, config.GUI_FONT_SIZE_TREE),
        rowheight=row_height,
    )
    style.configure(
        "TButton", font=(config.GUI_FONT_FAMILY, config.GUI_FONT_SIZE_BASE)
    )
    # -----------------------------------

    # Change default resolution slightly to handle larger fonts comfortably
    root.title("AI Code Serializer - File Tree Selection")
    root.geometry("800x650")

    root.rowconfigure(0, weight=1)
    root.columnconfigure(0, weight=1)

    main_frame = ttk.Frame(root, padding="15")
    main_frame.grid(row=0, column=0, sticky="nsew")
    main_frame.rowconfigure(1, weight=1)
    main_frame.columnconfigure(0, weight=1)

    # --- UPDATED HEADER WITH CONFIG FONTS ---
    header = ttk.Label(
        main_frame,
        text="Interactive File Tree Selection:\n"
        "- Click folders or files to toggle their inclusion.\n"
        "- Toggling a folder applies the change to all items inside it.\n"
        "- Click (▶/▼) next to folders to fold or unfold directories.",
        font=(config.GUI_FONT_FAMILY, config.GUI_FONT_SIZE_BASE, "bold"),
        justify="left",
    )
    header.grid(row=0, column=0, pady=(0, 15), sticky="w")
    # ----------------------------------------

    # Frame containing the Treeview and Scrollbars
    tree_frame = ttk.Frame(main_frame)
    tree_frame.grid(row=1, column=0, sticky="nsew")
    tree_frame.rowconfigure(0, weight=1)
    tree_frame.columnconfigure(0, weight=1)

    tree = ttk.Treeview(tree_frame, selectmode="none", show="tree")
    tree.column("#0", width=550, minwidth=250, stretch=True)

    vsb = ttk.Scrollbar(tree_frame, orient="vertical", command=tree.yview)
    hsb = ttk.Scrollbar(tree_frame, orient="horizontal", command=tree.xview)
    tree.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)

    tree.grid(row=0, column=0, sticky="nsew")
    vsb.grid(row=0, column=1, sticky="ns")
    hsb.grid(row=1, column=0, sticky="ew")

    # Generate tree nodes representation
    root_node = build_tree_from_flat_list(files_list)
    item_to_node = {}

    def populate_gui_tree(parent_gui_id: str, node: TreeNode):
        sym = CHECKED_SYM if node.checked else UNCHECKED_SYM
        gui_id = tree.insert(
            parent_gui_id, "end", text=f"{sym}{node.name}", open=True
        )
        node.tree_item_id = gui_id
        item_to_node[gui_id] = node

        # Sort: list folders first, then files alphabetically
        sorted_children = sorted(
            node.children.items(),
            key=lambda x: (not x[1].is_dir, x[0].lower()),
        )
        for _, child_node in sorted_children:
            populate_gui_tree(gui_id, child_node)

    # Populate top-level paths under root
    for _, node in sorted(
        root_node.children.items(),
        key=lambda x: (not x[1].is_dir, x[0].lower()),
    ):
        populate_gui_tree("", node)

    # Define custom mouse clicks mapping on nodes
    def on_click(event):
            item_id = tree.identify_row(event.y)
            element = tree.identify_element(event.x, event.y)
            
            # Use lowercase and check if "indicator" is in the element name 
            # to support "Treeitem.indicator", "tree.indicator", and other theme variations.
            if item_id and "indicator" not in element.lower():
                node = item_to_node.get(item_id)
                if node:
                    new_state = not node.checked
                    # Propagate down
                    set_checked_descendants(tree, node, new_state)
                    # Propagate up
                    update_parent_state_upward(tree, node.parent)

    tree.bind("<Button-1>", on_click)

    # Global selections & confirmation buttons
    btn_frame = ttk.Frame(main_frame, padding="10")
    btn_frame.grid(row=2, column=0, sticky="ew")

    def select_all():
        for node in root_node.children.values():
            set_checked_descendants(tree, node, True)
            update_parent_state_upward(tree, node.parent)

    def select_none():
        for node in root_node.children.values():
            set_checked_descendants(tree, node, False)
            update_parent_state_upward(tree, node.parent)

    def confirm():
            # Export status from memory tree to flat selection list
            status_map = {}
            get_checked_status(root_node, status_map)
            for file_entry in files_list:
                # Normalize key to match the POSIX format stored in the status map
                rel_path = Path(file_entry["relative_path"]).as_posix()
                if rel_path in status_map:
                    file_entry["include"] = status_map[rel_path]
            root.destroy()

    btn_all = ttk.Button(btn_frame, text="Select All", command=select_all)
    btn_all.pack(side="left", padx=5)

    btn_none = ttk.Button(btn_frame, text="Select None", command=select_none)
    btn_none.pack(side="left", padx=5)

    btn_confirm = ttk.Button(btn_frame, text="Save & Close", command=confirm)
    btn_confirm.pack(side="right", padx=5)

    def on_closing():
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_closing)
    root.mainloop()

    return files_list