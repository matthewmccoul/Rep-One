import json
import math
import copy
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, simpledialog
from dataclasses import dataclass
from typing import List, Optional, Tuple


@dataclass
class ClipboardData:
    width: int
    height: int
    cells: List[List[int]]


class MapModel:
    """Minimal RPG Maker MV map model.

    Supports the standard MV flattened `data` array layout:
    width * height * 6 layers.

    This editor currently only draws/edits the first four tile layers.
    Shadow, region, and events are preserved on load/save but not edited.
    """

    TOTAL_LAYERS = 6
    DRAWABLE_LAYERS = 4

    def __init__(self) -> None:
        self.raw: Optional[dict] = None
        self.width: int = 0
        self.height: int = 0
        self.data: List[int] = []
        self.path: Optional[str] = None

    def load(self, path: str) -> None:
        with open(path, "r", encoding="utf-8") as f:
            raw = json.load(f)

        width = raw["width"]
        height = raw["height"]
        data = raw["data"]
        expected = width * height * self.TOTAL_LAYERS

        if len(data) != expected:
            raise ValueError(
                f"Unexpected data length: got {len(data)}, expected {expected} "
                f"for a {width}x{height} MV map."
            )

        self.raw = raw
        self.width = width
        self.height = height
        self.data = list(data)
        self.path = path

    def save(self, path: Optional[str] = None) -> None:
        if self.raw is None:
            raise ValueError("No map loaded.")

        out = copy.deepcopy(self.raw)
        out["width"] = self.width
        out["height"] = self.height
        out["data"] = list(self.data)

        target = path or self.path
        if not target:
            raise ValueError("No save path available.")

        with open(target, "w", encoding="utf-8") as f:
            json.dump(out, f, separators=(",", ":"))

        self.path = target

    def index(self, x: int, y: int, layer: int) -> int:
        return layer * self.width * self.height + y * self.width + x

    def get(self, x: int, y: int, layer: int) -> int:
        return self.data[self.index(x, y, layer)]

    def set(self, x: int, y: int, layer: int, value: int) -> None:
        self.data[self.index(x, y, layer)] = int(value)

    def composite_tile_value(self, x: int, y: int) -> int:
        """Show the highest visible non-zero tile from the first 4 layers.

        If all four are zero, fall back to layer 0.
        """
        for layer in range(self.DRAWABLE_LAYERS - 1, -1, -1):
            value = self.get(x, y, layer)
            if value != 0:
                return value
        return self.get(x, y, 0)

    def snapshot(self) -> List[int]:
        return list(self.data)

    def restore_snapshot(self, snap: List[int]) -> None:
        if len(snap) != len(self.data):
            raise ValueError("Snapshot length mismatch.")
        self.data = list(snap)


class MapEditorApp:
    BASE_TILE_SIZE = 48
    MIN_ZOOM = 0.25
    MAX_ZOOM = 4.0

    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Simple RPG Maker MV Map Editor")
        self.root.geometry("1280x780")

        self.model = MapModel()

        self.zoom = 1.0
        self.show_grid = True
        self.current_tool = tk.StringVar(value="pencil")
        self.current_layer = tk.IntVar(value=0)
        self.current_tile_value = tk.StringVar(value="2816")
        self.status_var = tk.StringVar(value="No map loaded")

        self.undo_stack: List[List[int]] = []
        self.redo_stack: List[List[int]] = []
        self.clipboard: Optional[ClipboardData] = None

        self.drag_start: Optional[Tuple[int, int]] = None
        self.drag_current: Optional[Tuple[int, int]] = None
        self.preview_id: Optional[int] = None
        self.selection_anchor: Optional[Tuple[int, int]] = None

        self._build_ui()
        self._build_menus()
        self._bind_shortcuts()

    # ---------- UI ----------

    def _build_ui(self) -> None:
        outer = ttk.Frame(self.root)
        outer.pack(fill="both", expand=True)

        # Left toolbox
        left = ttk.Frame(outer, padding=10)
        left.pack(side="left", fill="y")

        ttk.Label(left, text="Tools", font=("TkDefaultFont", 11, "bold")).pack(anchor="w")
        ttk.Separator(left).pack(fill="x", pady=(4, 8))

        ttk.Label(left, text="Draw Tool").pack(anchor="w")
        tool_combo = ttk.Combobox(
            left,
            textvariable=self.current_tool,
            state="readonly",
            values=["pencil", "rectangle", "ellipse", "fill"],
            width=16,
        )
        tool_combo.pack(anchor="w", pady=(2, 10))
        tool_combo.bind("<<ComboboxSelected>>", lambda e: self._update_status())

        ttk.Label(left, text="Edit Layer").pack(anchor="w")
        layer_combo = ttk.Combobox(
            left,
            textvariable=self.current_layer,
            state="readonly",
            values=[0, 1, 2, 3],
            width=16,
        )
        layer_combo.pack(anchor="w", pady=(2, 10))

        ttk.Label(left, text="Tile Code").pack(anchor="w")
        tile_entry = ttk.Entry(left, textvariable=self.current_tile_value, width=18)
        tile_entry.pack(anchor="w", pady=(2, 4))

        ttk.Button(left, text="Use Selected Tile Code", command=self._use_clicked_tile_code).pack(
            anchor="w", pady=(0, 10)
        )

        ttk.Label(left, text="Quick Actions").pack(anchor="w")
        quick = ttk.Frame(left)
        quick.pack(anchor="w", pady=(4, 10))
        ttk.Button(quick, text="Undo", command=self.undo, width=10).grid(row=0, column=0, padx=(0, 4), pady=2)
        ttk.Button(quick, text="Redo", command=self.redo, width=10).grid(row=0, column=1, pady=2)
        ttk.Button(quick, text="Copy", command=self.copy_selection, width=10).grid(row=1, column=0, padx=(0, 4), pady=2)
        ttk.Button(quick, text="Paste", command=self.paste_at_prompt, width=10).grid(row=1, column=1, pady=2)

        ttk.Label(left, text="View").pack(anchor="w")
        view = ttk.Frame(left)
        view.pack(anchor="w", pady=(4, 10))
        ttk.Button(view, text="Zoom In", command=self.zoom_in, width=10).grid(row=0, column=0, padx=(0, 4), pady=2)
        ttk.Button(view, text="Zoom Out", command=self.zoom_out, width=10).grid(row=0, column=1, pady=2)
        ttk.Button(view, text="Reset Zoom", command=self.default_zoom, width=10).grid(row=1, column=0, padx=(0, 4), pady=2)
        ttk.Button(view, text="Grid On/Off", command=self.toggle_grid, width=10).grid(row=1, column=1, pady=2)

        help_text = (
            "Left click draws.\n"
            "Right click samples a visible tile code.\n"
            "Rectangle and ellipse drag from start to end.\n"
            "Copy/paste works on the current edit layer.\n"
            "Shadows, regions, and events are preserved but not edited."
        )
        ttk.Label(left, text=help_text, wraplength=220, justify="left").pack(anchor="w", pady=(10, 0))

        # Right canvas area
        right = ttk.Frame(outer, padding=(0, 10, 10, 10))
        right.pack(side="left", fill="both", expand=True)

        self.canvas = tk.Canvas(right, background="#dde6c8", highlightthickness=1, highlightbackground="#888")
        self.hbar = ttk.Scrollbar(right, orient="horizontal", command=self.canvas.xview)
        self.vbar = ttk.Scrollbar(right, orient="vertical", command=self.canvas.yview)
        self.canvas.configure(xscrollcommand=self.hbar.set, yscrollcommand=self.vbar.set)

        self.canvas.grid(row=0, column=0, sticky="nsew")
        self.vbar.grid(row=0, column=1, sticky="ns")
        self.hbar.grid(row=1, column=0, sticky="ew")

        right.rowconfigure(0, weight=1)
        right.columnconfigure(0, weight=1)

        status = ttk.Label(self.root, textvariable=self.status_var, anchor="w", relief="sunken")
        status.pack(side="bottom", fill="x")

        self.canvas.bind("<Button-1>", self.on_left_down)
        self.canvas.bind("<B1-Motion>", self.on_left_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_left_up)
        self.canvas.bind("<Button-3>", self.on_right_click)
        self.canvas.bind("<Motion>", self.on_mouse_move)
        self.canvas.bind("<MouseWheel>", self.on_mousewheel)

    def _build_menus(self) -> None:
        menubar = tk.Menu(self.root)

        file_menu = tk.Menu(menubar, tearoff=0)
        file_menu.add_command(label="Load", command=self.load_map)
        file_menu.add_command(label="Save", command=self.save_map)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.destroy)
        menubar.add_cascade(label="File", menu=file_menu)

        edit_menu = tk.Menu(menubar, tearoff=0)
        edit_menu.add_command(label="Undo", command=self.undo)
        edit_menu.add_command(label="Redo", command=self.redo)
        edit_menu.add_separator()
        edit_menu.add_command(label="Copy", command=self.copy_selection)
        edit_menu.add_command(label="Paste", command=self.paste_at_prompt)
        menubar.add_cascade(label="Edit", menu=edit_menu)

        draw_menu = tk.Menu(menubar, tearoff=0)
        draw_menu.add_command(label="Pencil", command=lambda: self.set_tool("pencil"))
        draw_menu.add_command(label="Rectangle", command=lambda: self.set_tool("rectangle"))
        draw_menu.add_command(label="Ellipse", command=lambda: self.set_tool("ellipse"))
        draw_menu.add_command(label="Fill", command=lambda: self.set_tool("fill"))
        menubar.add_cascade(label="Draw", menu=draw_menu)

        view_menu = tk.Menu(menubar, tearoff=0)
        view_menu.add_command(label="Zoom In", command=self.zoom_in)
        view_menu.add_command(label="Zoom Out", command=self.zoom_out)
        view_menu.add_command(label="Default Zoom", command=self.default_zoom)
        view_menu.add_command(label="Grid Lines On/Off", command=self.toggle_grid)
        menubar.add_cascade(label="View", menu=view_menu)

        self.root.config(menu=menubar)

    def _bind_shortcuts(self) -> None:
        self.root.bind("<Control-o>", lambda e: self.load_map())
        self.root.bind("<Control-s>", lambda e: self.save_map())
        self.root.bind("<Control-z>", lambda e: self.undo())
        self.root.bind("<Control-y>", lambda e: self.redo())
        self.root.bind("<Control-c>", lambda e: self.copy_selection())
        self.root.bind("<Control-v>", lambda e: self.paste_at_prompt())
        self.root.bind("+", lambda e: self.zoom_in())
        self.root.bind("-", lambda e: self.zoom_out())

    # ---------- Helpers ----------

    def tile_size(self) -> int:
        return max(12, int(self.BASE_TILE_SIZE * self.zoom))

    def has_map(self) -> bool:
        return self.model.raw is not None

    def get_draw_value(self) -> int:
        text = self.current_tile_value.get().strip()
        if not text:
            raise ValueError("Tile code is blank.")
        return int(text)

    def push_undo(self) -> None:
        if not self.has_map():
            return
        self.undo_stack.append(self.model.snapshot())
        if len(self.undo_stack) > 100:
            self.undo_stack.pop(0)
        self.redo_stack.clear()

    def set_tool(self, tool: str) -> None:
        self.current_tool.set(tool)
        self._update_status()

    def _update_status(self, extra: str = "") -> None:
        if not self.has_map():
            self.status_var.set("No map loaded")
            return
        msg = (
            f"{self.model.width}x{self.model.height} | "
            f"Tool: {self.current_tool.get()} | "
            f"Layer: {self.current_layer.get()} | "
            f"Tile: {self.current_tile_value.get()} | "
            f"Zoom: {self.zoom:.2f}x"
        )
        if extra:
            msg += f" | {extra}"
        self.status_var.set(msg)

    def canvas_to_tile(self, event_x: int, event_y: int) -> Optional[Tuple[int, int]]:
        if not self.has_map():
            return None
        x = int(self.canvas.canvasx(event_x) // self.tile_size())
        y = int(self.canvas.canvasy(event_y) // self.tile_size())
        if 0 <= x < self.model.width and 0 <= y < self.model.height:
            return x, y
        return None

    def rect_bounds(self, a: Tuple[int, int], b: Tuple[int, int]) -> Tuple[int, int, int, int]:
        x1 = min(a[0], b[0])
        y1 = min(a[1], b[1])
        x2 = max(a[0], b[0])
        y2 = max(a[1], b[1])
        return x1, y1, x2, y2

    def tile_center(self, x: int, y: int) -> Tuple[float, float]:
        ts = self.tile_size()
        return x * ts + ts / 2, y * ts + ts / 2

    # ---------- File actions ----------

    def load_map(self) -> None:
        path = filedialog.askopenfilename(
            title="Load RPG Maker MV map JSON",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
        )
        if not path:
            return
        try:
            self.model.load(path)
            self.undo_stack.clear()
            self.redo_stack.clear()
            self.selection_anchor = None
            self.drag_start = None
            self.drag_current = None
            self.render_map()
            self._update_status(f"Loaded {path}")
        except Exception as e:
            messagebox.showerror("Load Error", str(e))

    def save_map(self) -> None:
        if not self.has_map():
            messagebox.showinfo("Save", "No map loaded.")
            return

        path = filedialog.asksaveasfilename(
            title="Save map JSON",
            defaultextension=".json",
            initialfile=(self.model.path.split("/")[-1] if self.model.path else "Map.json"),
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
        )
        if not path:
            return
        try:
            self.model.save(path)
            self._update_status(f"Saved {path}")
        except Exception as e:
            messagebox.showerror("Save Error", str(e))

    # ---------- Edit actions ----------

    def undo(self) -> None:
        if not self.has_map() or not self.undo_stack:
            return
        self.redo_stack.append(self.model.snapshot())
        snap = self.undo_stack.pop()
        self.model.restore_snapshot(snap)
        self.render_map()
        self._update_status("Undo")

    def redo(self) -> None:
        if not self.has_map() or not self.redo_stack:
            return
        self.undo_stack.append(self.model.snapshot())
        snap = self.redo_stack.pop()
        self.model.restore_snapshot(snap)
        self.render_map()
        self._update_status("Redo")

    def copy_selection(self) -> None:
        if not self.has_map():
            return
        if self.selection_anchor is None or self.drag_current is None:
            messagebox.showinfo("Copy", "No current selection area. Drag a rectangle or ellipse first.")
            return

        x1, y1, x2, y2 = self.rect_bounds(self.selection_anchor, self.drag_current)
        layer = self.current_layer.get()
        cells = []
        for y in range(y1, y2 + 1):
            row = []
            for x in range(x1, x2 + 1):
                row.append(self.model.get(x, y, layer))
            cells.append(row)

        self.clipboard = ClipboardData(width=x2 - x1 + 1, height=y2 - y1 + 1, cells=cells)
        self._update_status(f"Copied {self.clipboard.width}x{self.clipboard.height} from layer {layer}")

    def paste_at_prompt(self) -> None:
        if not self.has_map():
            return
        if self.clipboard is None:
            messagebox.showinfo("Paste", "Clipboard is empty.")
            return

        x = simpledialog.askinteger("Paste", "Paste at tile X:", minvalue=0, maxvalue=max(0, self.model.width - 1))
        if x is None:
            return
        y = simpledialog.askinteger("Paste", "Paste at tile Y:", minvalue=0, maxvalue=max(0, self.model.height - 1))
        if y is None:
            return

        self.push_undo()
        layer = self.current_layer.get()
        for dy in range(self.clipboard.height):
            for dx in range(self.clipboard.width):
                tx = x + dx
                ty = y + dy
                if 0 <= tx < self.model.width and 0 <= ty < self.model.height:
                    self.model.set(tx, ty, layer, self.clipboard.cells[dy][dx])
        self.render_map()
        self._update_status(f"Pasted at {x},{y} on layer {layer}")

    # ---------- Drawing ----------

    def _use_clicked_tile_code(self) -> None:
        self._update_status("Tile code ready")

    def draw_point(self, x: int, y: int, value: int) -> None:
        self.model.set(x, y, self.current_layer.get(), value)

    def flood_fill(self, x: int, y: int, new_value: int) -> None:
        layer = self.current_layer.get()
        target = self.model.get(x, y, layer)
        if target == new_value:
            return

        stack = [(x, y)]
        seen = set()
        while stack:
            cx, cy = stack.pop()
            if (cx, cy) in seen:
                continue
            seen.add((cx, cy))
            if not (0 <= cx < self.model.width and 0 <= cy < self.model.height):
                continue
            if self.model.get(cx, cy, layer) != target:
                continue
            self.model.set(cx, cy, layer, new_value)
            stack.append((cx + 1, cy))
            stack.append((cx - 1, cy))
            stack.append((cx, cy + 1))
            stack.append((cx, cy - 1))

    def draw_rectangle(self, start: Tuple[int, int], end: Tuple[int, int], value: int) -> None:
        x1, y1, x2, y2 = self.rect_bounds(start, end)
        for y in range(y1, y2 + 1):
            for x in range(x1, x2 + 1):
                self.draw_point(x, y, value)

    def draw_ellipse(self, start: Tuple[int, int], end: Tuple[int, int], value: int) -> None:
        x1, y1, x2, y2 = self.rect_bounds(start, end)
        rx = max((x2 - x1 + 1) / 2.0, 0.5)
        ry = max((y2 - y1 + 1) / 2.0, 0.5)
        cx = x1 + rx - 0.5
        cy = y1 + ry - 0.5

        for y in range(y1, y2 + 1):
            for x in range(x1, x2 + 1):
                dx = (x - cx) / rx
                dy = (y - cy) / ry
                if dx * dx + dy * dy <= 1.0:
                    self.draw_point(x, y, value)

    # ---------- Canvas events ----------

    def on_left_down(self, event: tk.Event) -> None:
        tile = self.canvas_to_tile(event.x, event.y)
        if tile is None:
            return
        self.drag_start = tile
        self.drag_current = tile
        self.selection_anchor = tile

        tool = self.current_tool.get()
        try:
            value = self.get_draw_value()
        except Exception as e:
            messagebox.showerror("Tile Code Error", str(e))
            return

        if tool == "pencil":
            self.push_undo()
            self.draw_point(tile[0], tile[1], value)
            self.render_map()
        elif tool == "fill":
            self.push_undo()
            self.flood_fill(tile[0], tile[1], value)
            self.render_map()
        else:
            self.render_preview(tile, tile)

    def on_left_drag(self, event: tk.Event) -> None:
        tile = self.canvas_to_tile(event.x, event.y)
        if tile is None or self.drag_start is None:
            return

        tool = self.current_tool.get()
        self.drag_current = tile

        if tool == "pencil":
            try:
                value = self.get_draw_value()
            except Exception:
                return
            self.draw_point(tile[0], tile[1], value)
            self.render_map()
        elif tool in ("rectangle", "ellipse"):
            self.render_preview(self.drag_start, tile)

    def on_left_up(self, event: tk.Event) -> None:
        tile = self.canvas_to_tile(event.x, event.y)
        if tile is None or self.drag_start is None:
            self.clear_preview()
            self.drag_start = None
            self.drag_current = None
            return

        tool = self.current_tool.get()
        try:
            value = self.get_draw_value()
        except Exception as e:
            messagebox.showerror("Tile Code Error", str(e))
            self.clear_preview()
            self.drag_start = None
            self.drag_current = None
            return

        if tool in ("rectangle", "ellipse"):
            self.push_undo()
            if tool == "rectangle":
                self.draw_rectangle(self.drag_start, tile, value)
            else:
                self.draw_ellipse(self.drag_start, tile, value)
            self.render_map()

        self.clear_preview()
        self.drag_current = tile
        self._update_status(f"Tile {tile[0]},{tile[1]}")
        self.drag_start = None

    def on_right_click(self, event: tk.Event) -> None:
        tile = self.canvas_to_tile(event.x, event.y)
        if tile is None:
            return
        value = self.model.composite_tile_value(tile[0], tile[1])
        self.current_tile_value.set(str(value))
        self._update_status(f"Sampled tile code {value} at {tile[0]},{tile[1]}")

    def on_mouse_move(self, event: tk.Event) -> None:
        tile = self.canvas_to_tile(event.x, event.y)
        if tile is None or not self.has_map():
            return
        visible = self.model.composite_tile_value(tile[0], tile[1])
        current = self.model.get(tile[0], tile[1], self.current_layer.get())
        self._update_status(
            f"Hover {tile[0]},{tile[1]} | visible={visible} | layer{self.current_layer.get()}={current}"
        )

    def on_mousewheel(self, event: tk.Event) -> None:
        if event.state & 0x4:  # Ctrl pressed
            if event.delta > 0:
                self.zoom_in()
            else:
                self.zoom_out()

    # ---------- View ----------

    def zoom_in(self) -> None:
        self.zoom = min(self.MAX_ZOOM, round(self.zoom * 1.25, 4))
        self.render_map()
        self._update_status("Zoom in")

    def zoom_out(self) -> None:
        self.zoom = max(self.MIN_ZOOM, round(self.zoom / 1.25, 4))
        self.render_map()
        self._update_status("Zoom out")

    def default_zoom(self) -> None:
        self.zoom = 1.0
        self.render_map()
        self._update_status("Default zoom")

    def toggle_grid(self) -> None:
        self.show_grid = not self.show_grid
        self.render_map()
        self._update_status(f"Grid {'on' if self.show_grid else 'off'}")

    # ---------- Rendering ----------

    def clear_preview(self) -> None:
        if self.preview_id is not None:
            self.canvas.delete(self.preview_id)
            self.preview_id = None

    def render_preview(self, start: Tuple[int, int], end: Tuple[int, int]) -> None:
        self.clear_preview()
        ts = self.tile_size()
        x1, y1, x2, y2 = self.rect_bounds(start, end)
        self.preview_id = self.canvas.create_rectangle(
            x1 * ts,
            y1 * ts,
            (x2 + 1) * ts,
            (y2 + 1) * ts,
            outline="#d04b4b",
            width=2,
            dash=(6, 4),
        )

    def render_map(self) -> None:
        self.canvas.delete("all")
        self.clear_preview()

        if not self.has_map():
            self.canvas.create_text(40, 40, anchor="nw", text="Load a map JSON to begin.", font=("TkDefaultFont", 12))
            self.canvas.configure(scrollregion=(0, 0, 500, 300))
            return

        ts = self.tile_size()
        width_px = self.model.width * ts
        height_px = self.model.height * ts

        for y in range(self.model.height):
            for x in range(self.model.width):
                x1 = x * ts
                y1 = y * ts
                x2 = x1 + ts
                y2 = y1 + ts

                self.canvas.create_rectangle(x1, y1, x2, y2, fill="#c9dca2", outline="")

                value = self.model.composite_tile_value(x, y)
                font_size = max(7, min(12, int(ts / 5)))
                self.canvas.create_text(
                    x1 + ts / 2,
                    y1 + ts / 2,
                    text=str(value),
                    font=("TkDefaultFont", font_size),
                    fill="#1f1f1f",
                )

        if self.show_grid:
            grid_color = "#90a97d"
            for x in range(self.model.width + 1):
                xx = x * ts
                self.canvas.create_line(xx, 0, xx, height_px, fill=grid_color)
            for y in range(self.model.height + 1):
                yy = y * ts
                self.canvas.create_line(0, yy, width_px, yy, fill=grid_color)

        self.canvas.configure(scrollregion=(0, 0, width_px, height_px))


def main() -> None:
    root = tk.Tk()
    style = ttk.Style()
    try:
        style.theme_use("clam")
    except tk.TclError:
        pass
    app = MapEditorApp(root)
    app.render_map()
    root.mainloop()


if __name__ == "__main__":
    main()
