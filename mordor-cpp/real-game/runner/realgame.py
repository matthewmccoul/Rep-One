"""Drive the real 1995 Mordor (shareware, under Wine) for rule tests: edit the save, load, act, read the result.
One Game per X display. Coordinates are for the 1024x768 main window."""
import os, re, subprocess, time
S = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad"
MDR = ["dotnet", f"{S}/tools/mdr/out/mdr.dll"]
ENV = dict(os.environ, PATH="/root/.dotnet:" + os.environ["PATH"], DOTNET_NOLOGO="1")
class Game:
    def __init__(self, display=":77", prefix=f"{S}/real/prefix", shots=f"{S}/real/shots"):
        self.d, self.prefix, self.shots = display, prefix, shots
        self.save = f"{prefix}/drive_c/MORDOR/DATA/MDATA4.MDR"
    def x(self, *a):
        return subprocess.run(["xdotool", *a], env=dict(ENV, DISPLAY=self.d), capture_output=True, text=True).stdout
    def click(self, x, y, wait=1.0): self.x("mousemove", str(x), str(y), "click", "1"); time.sleep(wait)
    def type(self, s): self.x("type", "--delay", "60", s)
    def shot(self, name):
        p = f"{self.shots}/{name}.png"
        subprocess.run(f"xwd -root -silent -display {self.d} | convert xwd:- {p}", shell=True); return p
    def edit(self, **fields):
        args = [f"{k.replace('__', '.').replace('_', '(', 1) if False else k}={v}" for k, v in fields.items()]
        subprocess.run(MDR + ["set", self.save, "2900", "3", "Character", *args], env=ENV, check=True)
    def set(self, pairs):  # pairs: {"Stats(0)": 10, "Pack(1).Id": 13, ...}
        subprocess.run(MDR + ["set", self.save, "2900", "3", "Character", *[f"{k}={v}" for k, v in pairs.items()]], env=ENV, check=True)
    def read(self):
        out = subprocess.run(MDR + ["dump", self.save, "2900", "3", "Character"], env=ENV, capture_output=True, text=True).stdout
        return dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
    # --- screens ---
    def city_to_menu(self):
        self.click(898, 282, 2); self.click(478, 417, 4)       # Exit, "Exit to Main Menu?" Yes
    def unload(self): self.click(511, 353, 3)                 # Unload Character
    def load_and_run(self, password="p"):
        self.click(511, 353, 2); self.click(425, 425, 2)       # Load Character, pick Probe (OK)
        self.click(510, 382, 0.5); self.type(password); self.click(425, 425, 3)
        self.click(511, 323, 6)                                # Run
    def messages(self, tag):
        p = self.shot(tag); c = p.replace(".png", "_msg.png")
        subprocess.run(["convert", p, "-crop", "595x95+420+636", "-resize", "300%", "-colorspace", "Gray", c])
        txt = subprocess.run(["tesseract", c, "-", "--psm", "6"], capture_output=True, text=True).stdout
        return [l.strip() for l in txt.splitlines() if l.strip()]
    # --- actions ---
    def equip_slot(self, row):
        self.click(690, 399 + 16 * (row - 1), 1); self.click(913, 534, 3)

def _wins(d, visible=False):
    out = subprocess.run(f"xdotool search {'--onlyvisible ' if visible else ''}--name . 2>/dev/null | while read w; do echo \"$w|$(xdotool getwindowname $w)\"; done", shell=True, env=dict(ENV, DISPLAY=d), capture_output=True, text=True).stdout
    return [l.split("|", 1) for l in out.splitlines() if "|" in l]
def start(g, log=None):
    """(Re)start Xvfb and the game on g.d; leave it at the main menu with the window at 1024x768."""
    subprocess.run(f"WINEPREFIX={g.prefix} wineserver -k ; pkill -f 'winevdm.*MORDOR' ; pkill -f 'wine.*MORDOR' ; pkill -f winedbg ; sleep 2", shell=True)
    if subprocess.run(["pgrep", "-f", f"Xvfb {g.d}"], capture_output=True).returncode != 0:
        subprocess.Popen(["Xvfb", g.d, "-screen", "0", "1024x768x24"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL); time.sleep(1)
    env = dict(ENV, DISPLAY=g.d, WINEPREFIX=g.prefix, WINEDEBUG="trace+msgbox")
    g.log = log or f"{g.shots}/../run_{g.d[1:]}.log"
    subprocess.Popen(["wine", "C:\\MORDOR\\MORDOR.EXE"], cwd=f"{g.prefix}/drive_c/MORDOR", env=env, stdout=open(g.log, "w"), stderr=subprocess.STDOUT, start_new_session=True)
    for _ in range(60):
        time.sleep(1); w = _wins(g.d)
        pv = [i for i, n in w if n == "Public Version"]
        if pv: g.x("key", "--window", pv[0], "Return"); break
    for _ in range(30):
        time.sleep(1); w = _wins(g.d); m = [i for i, n in w if "Limited Version" in n]
        if m: g.x("windowsize", m[0], "1024", "768"); g.x("windowmove", m[0], "0", "0"); time.sleep(2); return True
    return False
def crashed(g):
    """The game itself is gone: its main window or its process. Stray debugger windows from other Wine processes do not count."""
    alive = subprocess.run(["pgrep", "-f", "winevdm.*MORDOR"], capture_output=True).returncode == 0
    names = [n for _, n in _wins(g.d, visible=True)]
    # start() kills leftover debuggers, so a Program Error window here is from this game
    return not alive or "Program Error" in names or not any("Limited Version" in n for _, n in _wins(g.d))
