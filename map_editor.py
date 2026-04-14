import json
import math
import copy
import os
from dataclasses import dataclass
from typing import List, Optional, Tuple

from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.widget import Widget
from kivy.uix.scrollview import ScrollView
from kivy.uix.label import Label
from kivy.uix.button import Button
from kivy.uix.spinner import Spinner
from kivy.uix.textinput import TextInput
from kivy.uix.popup import Popup
from kivy.uix.filechooser import FileChooserListView
from kivy.uix.dropdown import DropDown
from kivy.graphics import Color, Rectangle, RoundedRectangle, Line, InstructionGroup
from kivy.core.text import Label as CoreLabel
from kivy.metrics import dp


# ---------- Theme ----------

_C_BAR   = (0.13, 0.13, 0.17, 1)
_C_BTN   = (0.22, 0.22, 0.28, 1)
_C_BTN_D = (0.34, 0.34, 0.46, 1)
_C_ACT   = (0.12, 0.26, 0.46, 1)
_C_ACT_D = (0.18, 0.38, 0.62, 1)


class FlatButton(Button):
    def __init__(self, col=_C_BTN, col_down=_C_BTN_D, **kwargs):
        super().__init__(**kwargs)
        self.background_normal = ''
        self.background_down   = ''
        self.background_color  = (0, 0, 0, 0)
        self._col_up   = col
        self._col_down = col_down
        with self.canvas.before:
            self._rc = Color(*col)
            self._rr = RoundedRectangle(pos=self.pos, size=self.size, radius=[dp(5)])
        self.bind(pos=self._upd, size=self._upd, state=self._upd_state)

    def _upd(self, *_):
        self._rr.pos  = self.pos
        self._rr.size = self.size

    def _upd_state(self, *_):
        self._rc.rgba = self._col_down if self.state == 'down' else self._col_up


def _tinted(layout, color=_C_BAR):
    with layout.canvas.before:
        Color(*color)
        rect = Rectangle(pos=layout.pos, size=layout.size)
    layout.bind(
        pos =lambda *_: setattr(rect, 'pos',  layout.pos),
        size=lambda *_: setattr(rect, 'size', layout.size),
    )
    return layout


# ---------- Data ----------

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


# ---------- Dialogs ----------

class AlertPopup(Popup):
    @classmethod
    def show(cls, title, message):
        inst = cls(title=title, size_hint=(0.8, 0.4))
        layout = BoxLayout(orientation='vertical', padding=dp(12), spacing=dp(8))
        layout.add_widget(Label(text=message, color=(0.9, 0.9, 0.9, 1)))
        ok = FlatButton(text='OK', col=_C_ACT, col_down=_C_ACT_D,
                        size_hint_y=None, height=dp(44))
        ok.bind(on_release=inst.dismiss)
        layout.add_widget(ok)
        inst.content = layout
        inst.open()


class FileDialog(Popup):
    def __init__(self, mode='load', on_select=None, **kwargs):
        super().__init__(**kwargs)
        self._mode     = mode
        self._on_select = on_select
        self.title     = 'Load Map' if mode == 'load' else 'Save Map'
        self.size_hint = (0.95, 0.9)

        # Best default: Downloads folder
        for candidate in ('/sdcard/Download', '/storage/emulated/0/Download',
                          '/sdcard', '/storage/emulated/0', '/'):
            if os.path.isdir(candidate):
                start = candidate
                break

        layout = BoxLayout(orientation='vertical', spacing=dp(4), padding=dp(4))

        # Path bar — shows current dir, lets user type a path manually
        path_row = BoxLayout(size_hint_y=None, height=dp(44), spacing=dp(4))
        self._path_input = TextInput(
            text=start, multiline=False,
            background_color=(0.18, 0.18, 0.22, 1),
            foreground_color=(0.9, 0.9, 0.9, 1),
        )
        go_btn = FlatButton(text='Go', col=_C_ACT, col_down=_C_ACT_D,
                            size_hint_x=None, width=dp(60))
        go_btn.bind(on_release=self._go)
        path_row.add_widget(self._path_input)
        path_row.add_widget(go_btn)
        layout.add_widget(path_row)

        # Quick-jump buttons
        quick = BoxLayout(size_hint_y=None, height=dp(40), spacing=dp(4))
        for label, path in [('Downloads', '/sdcard/Download'),
                             ('/sdcard',   '/sdcard'),
                             ('/storage',  '/storage/emulated/0'),
                             ('/',         '/')]:
            if os.path.isdir(path):
                b = FlatButton(text=label, col=_C_BTN, col_down=_C_BTN_D)
                b.bind(on_release=lambda btn, p=path: self._nav(p))
                quick.add_widget(b)
        layout.add_widget(quick)

        # File browser
        self._chooser = FileChooserListView(path=start)
        self._chooser.bind(path=lambda i, v: setattr(self._path_input, 'text', v))
        layout.add_widget(self._chooser)

        if mode == 'save':
            self._filename_input = TextInput(
                text='Map.json', multiline=False,
                background_color=(0.18, 0.18, 0.22, 1),
                foreground_color=(0.9, 0.9, 0.9, 1),
                size_hint_y=None, height=dp(44),
            )
            layout.add_widget(self._filename_input)

        btn_row = BoxLayout(size_hint_y=None, height=dp(50), spacing=dp(4))
        ok  = FlatButton(text='Select' if mode == 'load' else 'Save',
                         col=_C_ACT, col_down=_C_ACT_D)
        can = FlatButton(text='Cancel')
        ok.bind(on_release=self._do_select)
        can.bind(on_release=self.dismiss)
        btn_row.add_widget(ok)
        btn_row.add_widget(can)
        layout.add_widget(btn_row)
        self.content = layout

    def _nav(self, path):
        if os.path.isdir(path):
            self._chooser.path = path

    def _go(self, *_):
        p = self._path_input.text.strip()
        if os.path.isfile(p):
            self._chooser.selection = [p]
        elif os.path.isdir(p):
            self._nav(p)

    def _do_select(self, *_):
        if self._mode == 'load':
            sel = self._chooser.selection
            if not sel:
                return
            path = sel[0]
        else:
            fn = self._filename_input.text.strip()
            if not fn:
                return
            path = os.path.join(self._chooser.path, fn)
        self.dismiss()
        if self._on_select:
            self._on_select(path)


class PasteDialog(Popup):
    def __init__(self, model, clipboard, on_confirm=None, **kwargs):
        super().__init__(title='Paste', size_hint=(0.7, 0.5), **kwargs)
        self._on_confirm = on_confirm
        self._model      = model

        layout = BoxLayout(orientation='vertical', padding=dp(12), spacing=dp(8))
        layout.add_widget(Label(
            text=f'Clipboard {clipboard.width}x{clipboard.height} — paste at:',
            color=(0.85, 0.85, 0.85, 1), size_hint_y=None, height=dp(36),
        ))

        for attr, label in [('_x_input', 'Tile X'), ('_y_input', 'Tile Y')]:
            row = BoxLayout(size_hint_y=None, height=dp(44), spacing=dp(8))
            row.add_widget(Label(text=label, size_hint_x=None, width=dp(70),
                                 color=(0.8, 0.8, 0.8, 1)))
            inp = TextInput(text='0', multiline=False, input_filter='int',
                            background_color=(0.18, 0.18, 0.22, 1),
                            foreground_color=(0.9, 0.9, 0.9, 1))
            setattr(self, attr, inp)
            row.add_widget(inp)
            layout.add_widget(row)

        btn_row = BoxLayout(size_hint_y=None, height=dp(50), spacing=dp(4))
        ok  = FlatButton(text='Paste',  col=_C_ACT, col_down=_C_ACT_D)
        can = FlatButton(text='Cancel')
        ok.bind(on_release=self._do_paste)
        can.bind(on_release=self.dismiss)
        btn_row.add_widget(ok)
        btn_row.add_widget(can)
        layout.add_widget(btn_row)
        self.content = layout

    def _do_paste(self, *_):
        try:
            x, y = int(self._x_input.text), int(self._y_input.text)
        except ValueError:
            AlertPopup.show('Error', 'X and Y must be integers.')
            return
        m = self._model
        if not (0 <= x < m.width and 0 <= y < m.height):
            AlertPopup.show('Error', f'Out of range (0–{m.width-1}, 0–{m.height-1}).')
            return
        self.dismiss()
        if self._on_confirm:
            self._on_confirm(x, y)


# ---------- Map canvas widget ----------

class MapWidget(Widget):
    def __init__(self, root_layout, **kwargs):
        super().__init__(**kwargs)
        self.root_layout = root_layout
        self.size_hint = (None, None)
        self.size = (100, 100)
        self._label_cache = {}

        self.tile_group = InstructionGroup()
        self.preview_group = InstructionGroup()
        self.canvas.add(self.tile_group)
        self.canvas.add(self.preview_group)

        self.bind(size=self._on_size_change, pos=self._on_size_change)

    def _on_size_change(self, *args):
        self.update_canvas()

    def _pos_to_tile(self, wx, wy):
        rl = self.root_layout
        if not rl.has_map():
            return None
        ts = rl.tile_size()
        tx = int(wx / ts)
        ty = rl.model.height - 1 - int(wy / ts)
        if 0 <= tx < rl.model.width and 0 <= ty < rl.model.height:
            return tx, ty
        return None

    def _get_label_texture(self, value, font_size):
        key = (value, font_size)
        if key not in self._label_cache:
            lbl = CoreLabel(text=str(value), font_size=font_size)
            lbl.refresh()
            self._label_cache[key] = lbl.texture
        return self._label_cache[key]

    def update_canvas(self, *args):
        rl = self.root_layout
        self.tile_group.clear()

        if self.width == 0 or self.height == 0:
            return
        if not rl.has_map():
            return

        model = rl.model
        ts = rl.tile_size()
        map_px_w = model.width * ts
        map_px_h = model.height * ts

        self.size = (map_px_w, map_px_h)

        sv = rl.scroll_view
        sv_w = max(sv.width, 1)
        sv_h = max(sv.height, 1)
        scroll_x_px = sv.scroll_x * max(0, map_px_w - sv_w)
        scroll_y_px = sv.scroll_y * max(0, map_px_h - sv_h)

        vp_left = scroll_x_px
        vp_bottom = scroll_y_px
        vp_right = vp_left + sv_w
        vp_top = vp_bottom + sv_h

        x_min = max(0, int(vp_left / ts))
        x_max = min(model.width - 1, int(vp_right / ts) + 1)
        map_y_min = max(0, model.height - 1 - int(vp_top / ts) - 1)
        map_y_max = min(model.height - 1, model.height - 1 - int(vp_bottom / ts) + 1)

        font_size = max(7, min(12, int(ts / 5)))

        # Tile backgrounds
        self.tile_group.add(Color(0.788, 0.862, 0.635, 1))
        for map_y in range(map_y_min, map_y_max + 1):
            for map_x in range(x_min, x_max + 1):
                px = map_x * ts
                py = (model.height - 1 - map_y) * ts
                self.tile_group.add(Rectangle(pos=(px, py), size=(ts, ts)))

        # Tile value labels
        self.tile_group.add(Color(0.122, 0.122, 0.122, 1))
        for map_y in range(map_y_min, map_y_max + 1):
            for map_x in range(x_min, x_max + 1):
                val = model.composite_tile_value(map_x, map_y)
                tex = self._get_label_texture(val, font_size)
                px = map_x * ts
                py = (model.height - 1 - map_y) * ts
                self.tile_group.add(Rectangle(
                    texture=tex,
                    pos=(px + ts / 2 - tex.width / 2, py + ts / 2 - tex.height / 2),
                    size=(tex.width, tex.height)
                ))

        # Grid lines
        if rl.show_grid:
            self.tile_group.add(Color(0.565, 0.663, 0.490, 1))
            for gx in range(x_min, x_max + 2):
                px = gx * ts
                self.tile_group.add(Line(
                    points=[px, map_y_min * ts, px, (map_y_max + 1) * ts], width=1
                ))
            for map_y in range(map_y_min, map_y_max + 2):
                py = (model.height - 1 - map_y) * ts
                self.tile_group.add(Line(
                    points=[x_min * ts, py, (x_max + 1) * ts, py], width=1
                ))

    def render_preview(self, start, end):
        self.preview_group.clear()
        rl = self.root_layout
        ts = rl.tile_size()
        model = rl.model
        x1, y1, x2, y2 = rl.rect_bounds(start, end)
        px1 = x1 * ts
        py1 = (model.height - 1 - y2) * ts
        pw = (x2 - x1 + 1) * ts
        ph = (y2 - y1 + 1) * ts
        self.preview_group.add(Color(0.816, 0.294, 0.294, 1))
        self.preview_group.add(Line(rectangle=(px1, py1, pw, ph), dash_offset=0, dash_length=6, width=2))

    def clear_preview(self):
        self.preview_group.clear()

    def on_touch_down(self, touch):
        if not self.collide_point(*touch.pos):
            return False
        rl = self.root_layout
        if rl.current_tool == 'pan':
            return False
        touch.grab(self)
        wx, wy = self.to_local(*touch.pos)
        tile = self._pos_to_tile(wx, wy)
        if tile is None:
            return True
        rl.drag_start = tile
        rl.drag_current = tile
        rl.selection_anchor = tile
        tool = rl.current_tool

        if tool == 'sample':
            val = rl.model.composite_tile_value(tile[0], tile[1])
            rl.current_tile_value = str(val)
            rl.tile_code_input.text = str(val)
            rl._update_status(f'Sampled {val} at {tile[0]},{tile[1]}')
            rl.current_tool = rl._prev_tool
            rl.tool_spinner.text = rl._prev_tool
            return True

        try:
            value = rl.get_draw_value()
        except ValueError as e:
            AlertPopup.show('Tile Code Error', str(e))
            return True

        if tool == 'pencil':
            rl.push_undo()
            rl.draw_point(tile[0], tile[1], value)
            self.update_canvas()
        elif tool == 'fill':
            rl.push_undo()
            rl.flood_fill(tile[0], tile[1], value)
            self.update_canvas()
        else:
            self.render_preview(tile, tile)
        return True

    def on_touch_move(self, touch):
        if touch.grab_current is not self:
            return False
        rl = self.root_layout
        wx, wy = self.to_local(*touch.pos)
        tile = self._pos_to_tile(wx, wy)
        if tile is None or rl.drag_start is None:
            return True
        rl.drag_current = tile
        tool = rl.current_tool
        if tool == 'pencil':
            try:
                value = rl.get_draw_value()
            except ValueError:
                return True
            rl.draw_point(tile[0], tile[1], value)
            self.update_canvas()
        elif tool in ('rectangle', 'ellipse'):
            self.render_preview(rl.drag_start, tile)
        return True

    def on_touch_up(self, touch):
        if touch.grab_current is not self:
            return False
        touch.ungrab(self)
        rl = self.root_layout
        wx, wy = self.to_local(*touch.pos)
        tile = self._pos_to_tile(wx, wy)
        if tile is None or rl.drag_start is None:
            self.clear_preview()
            rl.drag_start = None
            rl.drag_current = None
            return True
        tool = rl.current_tool
        try:
            value = rl.get_draw_value()
        except ValueError as e:
            AlertPopup.show('Tile Code Error', str(e))
            self.clear_preview()
            rl.drag_start = None
            rl.drag_current = None
            return True
        if tool in ('rectangle', 'ellipse'):
            rl.push_undo()
            if tool == 'rectangle':
                rl.draw_rectangle(rl.drag_start, tile, value)
            else:
                rl.draw_ellipse(rl.drag_start, tile, value)
            self.update_canvas()
        self.clear_preview()
        rl.drag_current = tile
        rl._update_status(f'Tile {tile[0]},{tile[1]}')
        rl.drag_start = None
        return True


# ---------- Layout ----------

class RootLayout(BoxLayout):
    BASE_TILE_SIZE = 48
    MIN_ZOOM = 0.25
    MAX_ZOOM = 4.0

    def __init__(self, **kwargs):
        super().__init__(orientation='vertical', **kwargs)

        self.model = MapModel()
        self.zoom = 1.0
        self.show_grid = True
        self.current_tool = 'pencil'
        self._prev_tool = 'pencil'
        self.current_layer = 0
        self.current_tile_value = '2816'
        self.undo_stack: List[List[int]] = []
        self.redo_stack: List[List[int]] = []
        self.clipboard: Optional[ClipboardData] = None
        self.drag_start: Optional[Tuple[int, int]] = None
        self.drag_current: Optional[Tuple[int, int]] = None
        self.selection_anchor: Optional[Tuple[int, int]] = None

        self._build_ui()

    def _build_ui(self):
        self.add_widget(self._build_top_bar())

        middle = BoxLayout(orientation='horizontal')
        middle.add_widget(self._build_left_bar())

        self.scroll_view = ScrollView(do_scroll_x=True, do_scroll_y=True)
        self.map_widget = MapWidget(root_layout=self)
        self.scroll_view.add_widget(self.map_widget)
        self.scroll_view.bind(
            scroll_x=self.map_widget.update_canvas,
            scroll_y=self.map_widget.update_canvas,
        )
        middle.add_widget(self.scroll_view)
        self.add_widget(middle)

        self.status_label = Label(
            text='No map loaded',
            size_hint_y=None, height=dp(24),
            halign='left', valign='middle',
        )
        self.status_label.bind(size=self.status_label.setter('text_size'))
        self.add_widget(self.status_label)

    def _build_top_bar(self):
        bar = _tinted(BoxLayout(
            orientation='horizontal', size_hint_y=None, height=dp(48), spacing=dp(3), padding=dp(3),
        ))

        def _dd_btn(label, items, width=dp(72)):
            dd = DropDown(auto_width=False, width=dp(160))
            for txt, cb in items:
                b = FlatButton(text=txt, col=_C_BTN, col_down=_C_BTN_D,
                               size_hint_y=None, height=dp(44))
                b.bind(on_release=lambda btn, c=cb, d=dd: (c(), d.dismiss()))
                dd.add_widget(b)
            btn = FlatButton(text=label, col=_C_ACT, col_down=_C_ACT_D,
                             size_hint_x=None, width=width)
            btn.bind(on_release=lambda b: dd.open(b))
            return btn

        bar.add_widget(_dd_btn('File', [
            ('Load', self.load_map),
            ('Save', self.save_map),
            ('Exit', lambda: App.get_running_app().stop()),
        ]))
        bar.add_widget(_dd_btn('Edit', [
            ('Copy',  self.copy_selection),
            ('Paste', self.paste_at_prompt),
        ]))

        self.tool_spinner = Spinner(
            text='pencil',
            values=['pencil', 'rectangle', 'ellipse', 'fill', 'pan', 'sample'],
            size_hint_x=None, width=dp(120),
            background_normal='', background_color=_C_BTN,
        )
        self.tool_spinner.bind(text=self._on_tool_change)
        bar.add_widget(self.tool_spinner)

        self.layer_spinner = Spinner(
            text='0', values=['0', '1', '2', '3'],
            size_hint_x=None, width=dp(62),
            background_normal='', background_color=_C_BTN,
        )
        self.layer_spinner.bind(text=self._on_layer_change)
        bar.add_widget(self.layer_spinner)

        self.tile_code_input = TextInput(
            text='2816', multiline=False, input_filter='int',
            size_hint_x=None, width=dp(90),
            background_color=(0.18, 0.18, 0.23, 1),
            foreground_color=(0.95, 0.95, 0.95, 1),
            cursor_color=(0.5, 0.75, 1.0, 1),
        )
        self.tile_code_input.bind(text=self._on_tile_code_change)
        bar.add_widget(self.tile_code_input)

        bar.add_widget(Label())  # spacer — status text could go here later
        return bar

    def _build_left_bar(self):
        bar = _tinted(BoxLayout(
            orientation='vertical', size_hint_x=None, width=dp(62),
            spacing=dp(3), padding=dp(3),
        ))
        for text, cmd in [
            ('Undo',  self.undo),
            ('Redo',  self.redo),
            ('+',     self.zoom_in),
            ('-',     self.zoom_out),
            ('Grid',  self.toggle_grid),
        ]:
            b = FlatButton(text=text)
            b.bind(on_release=lambda btn, c=cmd: c())
            bar.add_widget(b)

        more_dd = DropDown(auto_width=False, width=dp(160))
        for lbl, cb in [('Reset Zoom', self.default_zoom),
                        ('Sample',     lambda: self.set_tool('sample'))]:
            b = FlatButton(text=lbl, col=_C_BTN, col_down=_C_BTN_D,
                           size_hint_y=None, height=dp(44))
            b.bind(on_release=lambda btn, c=cb, d=more_dd: (c(), d.dismiss()))
            more_dd.add_widget(b)
        more = FlatButton(text='More')
        more.bind(on_release=lambda b: more_dd.open(b))
        bar.add_widget(more)
        return bar

    # ---------- Spinner callbacks ----------

    def _on_tool_change(self, spinner, text):
        if text != 'sample':
            self._prev_tool = text
        self.current_tool = text
        self._update_status()

    def _on_layer_change(self, spinner, text):
        self.current_layer = int(text)

    def _on_tile_code_change(self, ti, text):
        self.current_tile_value = text

    # ---------- Helpers ----------

    def tile_size(self):
        return max(12, int(self.BASE_TILE_SIZE * self.zoom))

    def has_map(self):
        return self.model.raw is not None

    def get_draw_value(self):
        text = self.current_tile_value.strip()
        if not text:
            raise ValueError('Tile code is blank.')
        return int(text)

    def push_undo(self):
        if not self.has_map():
            return
        self.undo_stack.append(self.model.snapshot())
        if len(self.undo_stack) > 100:
            self.undo_stack.pop(0)
        self.redo_stack.clear()

    def set_tool(self, tool):
        if tool != 'sample':
            self._prev_tool = tool
        self.current_tool = tool
        self.tool_spinner.text = tool
        self._update_status()

    def rect_bounds(self, a, b):
        return min(a[0], b[0]), min(a[1], b[1]), max(a[0], b[0]), max(a[1], b[1])

    def _update_status(self, extra=''):
        if not self.has_map():
            self.status_label.text = 'No map loaded'
            return
        msg = (f'{self.model.width}x{self.model.height} | '
               f'Tool: {self.current_tool} | Layer: {self.current_layer} | '
               f'Tile: {self.current_tile_value} | Zoom: {self.zoom:.2f}x')
        if extra:
            msg += f' | {extra}'
        self.status_label.text = msg

    # ---------- File ----------

    def load_map(self):
        def _on_selected(path):
            try:
                self.model.load(path)
                self.undo_stack.clear()
                self.redo_stack.clear()
                self.selection_anchor = None
                self.drag_start = None
                self.drag_current = None
                self.map_widget.update_canvas()
                self._update_status(f'Loaded {path}')
            except Exception as e:
                AlertPopup.show('Load Error', str(e))
        FileDialog(mode='load', on_select=_on_selected).open()

    def save_map(self):
        if not self.has_map():
            AlertPopup.show('Save', 'No map loaded.')
            return
        def _on_selected(path):
            try:
                self.model.save(path)
                self._update_status(f'Saved {path}')
            except Exception as e:
                AlertPopup.show('Save Error', str(e))
        FileDialog(mode='save', on_select=_on_selected).open()

    # ---------- Edit ----------

    def undo(self):
        if not self.has_map() or not self.undo_stack:
            return
        self.redo_stack.append(self.model.snapshot())
        self.model.restore_snapshot(self.undo_stack.pop())
        self.map_widget.update_canvas()
        self._update_status('Undo')

    def redo(self):
        if not self.has_map() or not self.redo_stack:
            return
        self.undo_stack.append(self.model.snapshot())
        self.model.restore_snapshot(self.redo_stack.pop())
        self.map_widget.update_canvas()
        self._update_status('Redo')

    def copy_selection(self):
        if not self.has_map():
            return
        if self.selection_anchor is None or self.drag_current is None:
            AlertPopup.show('Copy', 'No selection. Drag a rectangle or ellipse first.')
            return
        x1, y1, x2, y2 = self.rect_bounds(self.selection_anchor, self.drag_current)
        layer = self.current_layer
        cells = []
        for y in range(y1, y2 + 1):
            row = [self.model.get(x, y, layer) for x in range(x1, x2 + 1)]
            cells.append(row)
        self.clipboard = ClipboardData(width=x2 - x1 + 1, height=y2 - y1 + 1, cells=cells)
        self._update_status(f'Copied {self.clipboard.width}x{self.clipboard.height} from layer {layer}')

    def paste_at_prompt(self):
        if not self.has_map():
            return
        if self.clipboard is None:
            AlertPopup.show('Paste', 'Clipboard is empty.')
            return
        def _do(x, y):
            self.push_undo()
            layer = self.current_layer
            for dy in range(self.clipboard.height):
                for dx in range(self.clipboard.width):
                    tx, ty = x + dx, y + dy
                    if 0 <= tx < self.model.width and 0 <= ty < self.model.height:
                        self.model.set(tx, ty, layer, self.clipboard.cells[dy][dx])
            self.map_widget.update_canvas()
            self._update_status(f'Pasted at {x},{y} on layer {layer}')
        PasteDialog(self.model, self.clipboard, on_confirm=_do).open()

    # ---------- Drawing ----------

    def draw_point(self, x, y, value):
        self.model.set(x, y, self.current_layer, value)

    def flood_fill(self, x, y, new_value):
        layer = self.current_layer
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
            stack.extend([(cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)])

    def draw_rectangle(self, start, end, value):
        x1, y1, x2, y2 = self.rect_bounds(start, end)
        for y in range(y1, y2 + 1):
            for x in range(x1, x2 + 1):
                self.draw_point(x, y, value)

    def draw_ellipse(self, start, end, value):
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

    # ---------- View ----------

    def zoom_in(self):
        self.zoom = min(self.MAX_ZOOM, round(self.zoom * 1.25, 4))
        self.map_widget._label_cache.clear()
        self.map_widget.update_canvas()
        self._update_status('Zoom in')

    def zoom_out(self):
        self.zoom = max(self.MIN_ZOOM, round(self.zoom / 1.25, 4))
        self.map_widget._label_cache.clear()
        self.map_widget.update_canvas()
        self._update_status('Zoom out')

    def default_zoom(self):
        self.zoom = 1.0
        self.map_widget._label_cache.clear()
        self.map_widget.update_canvas()
        self._update_status('Default zoom')

    def toggle_grid(self):
        self.show_grid = not self.show_grid
        self.map_widget.update_canvas()
        self._update_status(f"Grid {'on' if self.show_grid else 'off'}")


# ---------- App ----------

class MapEditorApp(App):
    def build(self):
        return RootLayout()

    def on_start(self):
        try:
            from jnius import autoclass
            PythonActivity = autoclass('org.kivy.android.PythonActivity')
            ActivityInfo = autoclass('android.content.pm.ActivityInfo')
            PythonActivity.mActivity.setRequestedOrientation(0)  # SCREEN_ORIENTATION_LANDSCAPE
        except Exception:
            pass
        self.root.map_widget.update_canvas()


def main():
    MapEditorApp().run()


if __name__ == '__main__':
    main()
