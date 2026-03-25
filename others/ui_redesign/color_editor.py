"""
GLI Color Scheme Editor (PyQt5)
Reads CSS custom properties from prototype.html :root block.
Two-tab system: Base colors (always editable) + Advanced (derived, auto-computed but overridable).
Live antialiased preview with glow outlines on changed elements.
Modified entries are marked with a dot indicator.
"""

import colorsys
import json
import re
import sys
from pathlib import Path

from PyQt5.QtCore import Qt, QRectF, QPointF
from PyQt5.QtGui import (
    QColor, QPainter, QPen, QBrush, QFont, QFontMetrics,
    QLinearGradient, QPainterPath,
)
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QLineEdit, QPushButton, QScrollArea,
    QFrame, QSplitter, QFileDialog, QMessageBox, QColorDialog,
    QSizePolicy, QCheckBox, QTabWidget,
)

# ── Color math utilities ─────────────────────────────────────────────────

def hex_to_rgb(h):
    h = h.lstrip('#')
    return tuple(int(h[i:i+2], 16) for i in (0, 2, 4))

def rgb_to_hex(r, g, b):
    return f"#{int(r):02x}{int(g):02x}{int(b):02x}"

def lighten(hex_color, pct):
    r, g, b = hex_to_rgb(hex_color)
    r = r + (255 - r) * pct
    g = g + (255 - g) * pct
    b = b + (255 - b) * pct
    return rgb_to_hex(r, g, b)

def darken(hex_color, pct):
    r, g, b = hex_to_rgb(hex_color)
    r = r * (1 - pct)
    g = g * (1 - pct)
    b = b * (1 - pct)
    return rgb_to_hex(r, g, b)

def add_rgb(hex_color, dr, dg, db):
    r, g, b = hex_to_rgb(hex_color)
    return rgb_to_hex(
        max(0, min(255, r + dr)),
        max(0, min(255, g + dg)),
        max(0, min(255, b + db)),
    )

def desaturate(hex_color, amount):
    r, g, b = hex_to_rgb(hex_color)
    h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
    s *= (1 - amount)
    r2, g2, b2 = colorsys.hsv_to_rgb(h, s, v)
    return rgb_to_hex(r2 * 255, g2 * 255, b2 * 255)

def mix(hex_a, hex_b, ratio):
    ra, ga, ba = hex_to_rgb(hex_a)
    rb, gb, bb = hex_to_rgb(hex_b)
    r = ra * (1 - ratio) + rb * ratio
    g = ga * (1 - ratio) + gb * ratio
    b = ba * (1 - ratio) + bb * ratio
    return rgb_to_hex(r, g, b)

def c6(hex_color):
    """Strip alpha for QColor (keep 6-digit)."""
    return hex_color[:7] if len(hex_color) > 7 else hex_color

# ── Base color defaults ──────────────────────────────────────────────────

BASE_INTERFACE = {
    "--bg-base":       "#080C12",
    "--bg-raised":     "#0C1018",
    "--bg-elevated":   "#111820",
    "--bg-header":     "#161E2C",
    "--bg-header-end": "#0E1620",
    "--bg-deepest":    "#060A10",
    "--bg-active":     "#243040",
    "--border-subtle":  "#1A2030",
    "--border-default": "#1E2838",
    "--border-strong":  "#2A3848",
    "--text-primary":   "#C8D8E8",
    "--text-secondary": "#9AAABB",
    "--text-muted":     "#7A8898",
    "--text-dim":       "#4A5868",
    "--text-ghost":     "#2A3848",
    "--accent":       "#F0A030",
    "--accent-muted": "#C87820",
    "--positive":  "#2ECC71",
    "--negative":  "#E03030",
    "--info":      "#5090D0",
    "--warning":   "#C0A040",
    "--crosshair":   "#3CB371",
    "--map-terrain": "#14222E",
}

BASE_DETAILS = {
    "--tcu-distance": "#F0A030",
    "--tcu-depth":  "#30B060",
    "--tcu-laser": "#3070D0",
    "--tcu-mcp":   "#9040C0",
    # Lens arc sliders
    "--lens-zoom":    "#CC3820",
    "--lens-focus":   "#E07828",
    "--lens-iris":    "#2898B8",
    "--slider-track": "#111820",
    "--slider-thumb": "#F0A030",
    # PTZ
    "--btn-icon":     "#5A6878",
    "--joy-handle":   "#C87820",
    "--joy-dir":      "#5A3A18",
    "--ptz-stop":     "#2A2010",
    "--bar-hist":  "#205040",
    "--con-misc":  "#2A4050",
    "--laser-off":   "#3A7A3A",
    "--laser-armed": "#1A3A1A",
}

# ── Derivation ───────────────────────────────────────────────────────────

def compute_derived(base, mode="dark"):
    """Compute all derived colors from base colors + theme mode."""
    d = {}
    bg = base["--bg-base"]
    is_dark = (mode == "dark")

    # Surface derived
    d["--track-bg"]   = add_rgb(bg, 3, 3, 3) if is_dark else add_rgb(bg, -8, -8, -8)
    d["--shadow"]     = "#00000055"
    d["--dot-off"]    = add_rgb(base["--bg-active"], 15, 10, 5) if is_dark else add_rgb(base["--bg-active"], -10, -8, -5)
    d["--fp-hdr-end"] = mix(base["--bg-header"], base["--bg-elevated"], 0.5)

    # Scene gradients
    d["--scene-mid"]      = add_rgb(bg, 4, 8, 14) if is_dark else add_rgb(bg, -4, -6, -8)
    d["--scene-bright"]   = add_rgb(bg, 6, 12, 22) if is_dark else add_rgb(bg, -6, -8, -12)
    d["--scene-side"]     = add_rgb(bg, 6, 12, 19) if is_dark else add_rgb(bg, -5, -8, -10)
    d["--scene-side-alt"] = add_rgb(bg, 8, 12, 14) if is_dark else add_rgb(bg, -6, -8, -10)

    # Readout
    d["--readout"]     = darken(desaturate(base["--positive"], 0.45), 0.25) if is_dark else darken(desaturate(base["--positive"], 0.35), 0.35)
    d["--readout-dim"] = darken(desaturate(base["--positive"], 0.55), 0.45) if is_dark else darken(desaturate(base["--positive"], 0.45), 0.55)

    # Semantic backgrounds
    d["--positive-bg"]     = mix(bg, base["--positive"], 0.08) if is_dark else mix("#FFFFFF", base["--positive"], 0.06)
    d["--positive-border"] = mix(bg, base["--positive"], 0.25) if is_dark else mix("#FFFFFF", base["--positive"], 0.20)
    d["--negative-bg"]     = mix(bg, base["--negative"], 0.10) if is_dark else mix("#FFFFFF", base["--negative"], 0.06)
    d["--negative-border"] = mix(bg, base["--negative"], 0.28) if is_dark else mix("#FFFFFF", base["--negative"], 0.18)
    d["--negative-dim"]    = mix(base["--negative"], bg, 0.45) if is_dark else darken(base["--negative"], 0.20)
    d["--info-bg"]         = mix(bg, base["--info"], 0.08) if is_dark else mix("#FFFFFF", base["--info"], 0.06)
    d["--info-border"]     = mix(bg, base["--info"], 0.22) if is_dark else mix("#FFFFFF", base["--info"], 0.18)
    d["--warning-bg"]      = mix(bg, base["--warning"], 0.10) if is_dark else mix("#FFFFFF", base["--warning"], 0.06)
    d["--warning-border"]  = mix(bg, base["--warning"], 0.28) if is_dark else mix("#FFFFFF", base["--warning"], 0.18)

    # Hover states
    hover_add = 5 if is_dark else -5
    d["--positive-hover-bg"]     = add_rgb(d["--positive-bg"], hover_add, hover_add * 2, hover_add) if is_dark else add_rgb(d["--positive-bg"], -8, -12, -8)
    d["--positive-hover-text"]   = lighten(base["--positive"], 0.40) if is_dark else darken(base["--positive"], 0.15)
    d["--positive-hover-border"] = add_rgb(d["--positive-border"], 16, 32, 16) if is_dark else add_rgb(d["--positive-border"], -10, -15, -10)
    d["--negative-hover-bg"]     = add_rgb(d["--negative-bg"], hover_add * 2, hover_add, hover_add) if is_dark else add_rgb(d["--negative-bg"], -12, -8, -8)
    d["--negative-hover-text"]   = lighten(base["--negative"], 0.30) if is_dark else darken(base["--negative"], 0.15)
    d["--negative-hover-border"] = add_rgb(d["--negative-border"], 32, 8, 8) if is_dark else add_rgb(d["--negative-border"], -15, -8, -8)
    d["--negative-hover-bg-soft"] = add_rgb(d["--negative-bg"], hover_add, hover_add, hover_add) if is_dark else add_rgb(d["--negative-bg"], -5, -5, -5)
    d["--info-hover-bg"]     = add_rgb(d["--info-bg"], hover_add, hover_add, hover_add * 2) if is_dark else add_rgb(d["--info-bg"], -8, -8, -12)
    d["--info-hover-text"]   = lighten(base["--info"], 0.35) if is_dark else darken(base["--info"], 0.15)
    d["--info-hover-border"] = add_rgb(d["--info-border"], 16, 16, 32) if is_dark else add_rgb(d["--info-border"], -10, -10, -15)
    d["--warning-hover-bg"]     = add_rgb(d["--warning-bg"], hover_add, hover_add, hover_add) if is_dark else add_rgb(d["--warning-bg"], -8, -8, -8)
    d["--warning-hover-text"]   = lighten(base["--warning"], 0.30) if is_dark else darken(base["--warning"], 0.15)
    d["--warning-hover-border"] = add_rgb(d["--warning-border"], 24, 24, 8) if is_dark else add_rgb(d["--warning-border"], -12, -12, -8)

    # Laser / fire derived
    d["--laser-off-bg"]     = mix(bg, base["--positive"], 0.04) if is_dark else mix("#FFFFFF", base["--positive"], 0.03)
    d["--laser-off-text"]   = base["--laser-off"]
    d["--laser-armed-bg"]   = base["--laser-armed"]
    d["--fire-active-text"] = lighten(base["--negative"], 0.45) if is_dark else darken(base["--negative"], 0.20)
    d["--fire-stripe-alt"]  = add_rgb(d["--negative-bg"], 0, 4, 4) if is_dark else add_rgb(d["--negative-bg"], 0, -4, -4)
    d["--fire-active-bg-a"] = add_rgb(d["--negative-bg"], 16, 0, 0) if is_dark else add_rgb(d["--negative-bg"], -10, 0, 0)
    d["--fire-active-bg-b"] = add_rgb(d["--negative-bg"], 32, 4, 4) if is_dark else add_rgb(d["--negative-bg"], -16, -4, -4)

    # Unlock bar derived
    d["--unlock-text"]          = darken(desaturate(base["--positive"], 0.40), 0.55) if is_dark else darken(base["--positive"], 0.40)
    d["--unlock-bg-a"]          = mix(bg, base["--positive"], 0.03) if is_dark else mix("#FFFFFF", base["--positive"], 0.02)
    d["--unlock-locked-text"]   = darken(desaturate(base["--warning"], 0.20), 0.30) if is_dark else darken(base["--warning"], 0.25)
    d["--unlock-locked-border"] = darken(desaturate(base["--warning"], 0.30), 0.55) if is_dark else darken(base["--warning"], 0.45)
    d["--unlock-locked-bg-a"]   = mix(bg, base["--warning"], 0.03) if is_dark else mix("#FFFFFF", base["--warning"], 0.02)
    d["--unlock-locked-bg-b"]   = mix(bg, base["--warning"], 0.06) if is_dark else mix("#FFFFFF", base["--warning"], 0.04)

    # TCU bar gradient starts
    d["--tcu-distance-start"] = darken(base["--tcu-distance"], 0.35) if is_dark else darken(base["--tcu-distance"], 0.15)
    d["--tcu-depth-start"]    = darken(base["--tcu-depth"], 0.35) if is_dark else darken(base["--tcu-depth"], 0.15)
    d["--tcu-laser-start"]    = darken(base["--tcu-laser"], 0.40) if is_dark else darken(base["--tcu-laser"], 0.15)
    d["--tcu-mcp-start"]      = darken(base["--tcu-mcp"], 0.40) if is_dark else darken(base["--tcu-mcp"], 0.15)

    # --- Alpha surface variants (8-digit hex) ---
    raised = base.get("--bg-raised", "#0C1018")
    d["--bg-raised-93"]   = raised + "EE"   # 93% opacity — widgets, panels, overlays
    d["--bg-raised-80"]   = raised + "CC"   # 80% opacity — panel toggle bar
    d["--shadow-deep"]    = "#00000099"      # deep shadow for floating panels

    # --- Alpha accent variants ---
    accent = base.get("--accent", "#F0A030")
    accent_m = base.get("--accent-muted", "#C87820")
    d["--accent-glow-20"] = accent + "33"   # faintest border glow (panel toggle on)
    d["--accent-glow-27"] = accent + "44"   # focus border glow (guru input, console tab)
    d["--accent-glow-33"] = accent + "55"   # active border (fp-ico, mt-btn, dir-btn stop)
    d["--accent-glow-40"] = accent + "66"   # histogram peak bar
    d["--accent-glow-60"] = accent + "99"   # bounding box border
    d["--accent-glow-80"] = accent + "CC"   # bounding box label BG
    d["--accent-muted-glow-33"] = accent_m + "55"  # active icon border
    d["--accent-stop-33"] = accent + "55"   # PTZ stop button text

    # --- Alpha state variants ---
    pos = base.get("--positive", "#2ECC71")
    neg = base.get("--negative", "#E03030")
    cross = base.get("--crosshair", "#3CB371")
    joy_h = base.get("--joy-handle", "#C87820")
    d["--positive-glow"]      = pos + "88"   # status dot glow
    d["--negative-glow"]      = neg + "88"   # alert dot glow
    d["--negative-glow-40"]   = neg + "66"   # map pin shadow
    d["--negative-rec-glow"]  = "#E0505044"  # record button glow
    d["--crosshair-20"]       = cross + "33" # crosshair lines
    d["--crosshair-33"]       = cross + "55" # crosshair circle
    d["--joy-handle-glow"]    = joy_h + "55" # joystick handle shadow

    return d

# ── Categories ────────────────────────────────────────────────────────────

BASE_CATEGORIES = [
    # ── Interface ──
    ("Surfaces", [
        ("--bg-base",       "Main background, display area"),
        ("--bg-raised",     "Panels, widgets, overlays"),
        ("--bg-elevated",   "Active btn BG, slider track"),
        ("--bg-header",     "Header gradient start"),
        ("--bg-header-end", "Header gradient end, Normal btn BG"),
        ("--bg-deepest",    "Deepest recessed BG, console log"),
        ("--bg-active",     "Active press / hover state"),
    ]),
    ("Borders", [
        ("--border-subtle",  "Internal borders, console tabs"),
        ("--border-default", "Structural borders, Normal/Active btn border"),
        ("--border-strong",  "Hover borders"),
    ]),
    ("Text", [
        ("--text-primary",   "Bright text, headings"),
        ("--text-secondary", "Default body text"),
        ("--text-muted",     "Hover / combo text, Normal btn text"),
        ("--text-dim",       "Labels, bar labels, icon buttons"),
        ("--text-ghost",     "Faintest text, inactive tabs"),
    ]),
    ("Accent", [
        ("--accent",       "Primary accent, Active btn text, tabs"),
        ("--accent-muted", "Secondary accent, joystick handle"),
    ]),
    ("State Colors", [
        ("--positive",  "Connected / start / confirm (green)"),
        ("--negative",  "Error / stop / alert (red)"),
        ("--info",      "Orientation toggle (blue)"),
        ("--warning",   "Resolution toggle (gold)"),
    ]),
    ("Overlay", [
        ("--crosshair",   "Crosshair overlay on display"),
        ("--map-terrain", "Map terrain line color"),
    ]),
    # ── Component Details ──
    ("TCU Timing Bars", [
        ("--tcu-distance", "DISTANCE bar (orange)"),
        ("--tcu-depth",    "DEPTH / gate width bar (green)"),
        ("--tcu-laser",    "LASER width bar (blue)"),
        ("--tcu-mcp",      "MCP bar (purple)"),
    ]),
    ("Lens Controls", [
        ("--lens-zoom",    "Zoom arc slider (red)"),
        ("--lens-focus",   "Focus arc slider (yellow)"),
        ("--lens-iris",    "Iris arc slider (cyan)"),
        ("--slider-track", "Slider track background"),
        ("--slider-thumb", "Slider thumb color"),
    ]),
    ("PTZ", [
        ("--btn-icon",     "Dir/lens +/- button icon color"),
        ("--joy-handle",   "Joystick handle ring color"),
        ("--joy-dir",      "Joystick direction labels (N/S/E/W)"),
        ("--ptz-stop",     "PTZ stop button border tint"),
    ]),
    ("Console", [
        ("--bar-hist",  "Histogram bar fill"),
        ("--con-misc",  "Misc console text"),
    ]),
    ("Laser Controls", [
        ("--laser-off",   "LASER ON button off-state text"),
        ("--laser-armed", "LASER ON button armed-state BG"),
    ]),
]

DERIVED_CATEGORIES = [
    ("Surface Derived", ["--track-bg", "--shadow", "--dot-off", "--fp-hdr-end",
        "--scene-mid", "--scene-bright", "--scene-side", "--scene-side-alt"]),
    ("Readout (from Positive)", ["--readout", "--readout-dim"]),
    ("Positive Derived", ["--positive-bg", "--positive-border",
        "--positive-hover-bg", "--positive-hover-text", "--positive-hover-border"]),
    ("Negative Derived", ["--negative-bg", "--negative-border", "--negative-dim",
        "--negative-hover-bg", "--negative-hover-text", "--negative-hover-border",
        "--negative-hover-bg-soft"]),
    ("Info Derived", ["--info-bg", "--info-border",
        "--info-hover-bg", "--info-hover-text", "--info-hover-border"]),
    ("Warning Derived", ["--warning-bg", "--warning-border",
        "--warning-hover-bg", "--warning-hover-text", "--warning-hover-border"]),
    ("Laser / Fire Derived", ["--laser-off-bg", "--laser-off-text", "--laser-armed-bg",
        "--fire-active-text", "--fire-stripe-alt",
        "--fire-active-bg-a", "--fire-active-bg-b"]),
    ("Unlock Bar Derived", ["--unlock-text", "--unlock-bg-a",
        "--unlock-locked-text", "--unlock-locked-border",
        "--unlock-locked-bg-a", "--unlock-locked-bg-b"]),
    ("TCU Bar Gradient Starts", ["--tcu-distance-start",
        "--tcu-depth-start", "--tcu-laser-start", "--tcu-mcp-start"]),
    ("Alpha Surfaces", [
        "--bg-raised-93", "--bg-raised-80",
        "--shadow-deep",
    ]),
    ("Alpha Accents", [
        "--accent-glow-20", "--accent-glow-27", "--accent-glow-33",
        "--accent-glow-40", "--accent-glow-60", "--accent-glow-80",
        "--accent-muted-glow-33",
        "--accent-stop-33",
    ]),
    ("Alpha States", [
        "--positive-glow",
        "--negative-glow", "--negative-glow-40", "--negative-rec-glow",
        "--crosshair-20", "--crosshair-33",
        "--joy-handle-glow",
    ]),
]

BASE_VARS = [v for _, vl in BASE_CATEGORIES for v, _ in vl]
DERIVED_VARS = [v for _, vl in DERIVED_CATEGORIES for v in vl]
ALL_VARS = BASE_VARS + DERIVED_VARS

# ── HTML parsing ──────────────────────────────────────────────────────────

ROOT_RE = re.compile(r':root\s*\{([^}]+)\}', re.DOTALL)
VAR_RE = re.compile(r'(--[\w-]+)\s*:\s*(#[0-9a-fA-F]{3,8})\s*;')
HEX_OK = re.compile(r'^#[0-9a-fA-F]{6,8}$')


def parse_root(text):
    m = ROOT_RE.search(text)
    return dict(VAR_RE.findall(m.group(1))) if m else {}


def update_root(text, variables):
    m = ROOT_RE.search(text)
    if not m:
        return text
    block = m.group(0)
    for name, val in variables.items():
        block = re.sub(
            rf'({re.escape(name)}\s*:\s*)#[0-9a-fA-F]{{3,8}}(\s*;)',
            rf'\g<1>{val}\2', block)
    return text[:m.start()] + block + text[m.end():]


# ── Preview widget ────────────────────────────────────────────────────────

class PreviewWidget(QWidget):
    """Antialiased sample display with glow outlines on changed vars."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.variables = {}
        self.defaults = {}
        self.show_glow = True
        self.setMinimumSize(400, 500)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

    def set_data(self, variables, defaults):
        self.variables = variables
        self.defaults = defaults
        self.update()

    def _changed(self, var):
        return self.variables.get(var) != self.defaults.get(var)

    def _c(self, var, fallback="#000000"):
        return QColor(c6(self.variables.get(var, fallback)))

    def _glow_pen(self, *var_names):
        if self.show_glow and any(self._changed(v) for v in var_names):
            return QPen(QColor("#FFDD44"), 2, Qt.DashLine)
        return None

    def _draw_glow(self, p, rect, *var_names):
        glow = self._glow_pen(*var_names)
        if glow:
            p.setPen(glow)
            p.setBrush(Qt.NoBrush)
            p.drawRoundedRect(rect.adjusted(-2, -2, 2, 2), 4, 4)

    def paintEvent(self, event):
        if not self.variables:
            return
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing, True)
        p.setRenderHint(QPainter.TextAntialiasing, True)

        W = self.width()

        mono = QFont("Consolas", 9)
        mono_b = QFont("Consolas", 9, QFont.Bold)
        ui = QFont("Segoe UI", 9)
        ui_sm = QFont("Segoe UI", 8)
        ui_sm_b = QFont("Segoe UI", 8, QFont.Bold)

        x0 = 10
        cw = W - 20
        y = 10
        pad = 10
        row_h = 22
        base_sz = 9
        sm_sz = 8
        fm = QFontMetrics(mono)

        # Background fill
        p.fillRect(self.rect(), self._c("--bg-base"))

        # Section label helper
        def section_label(text, yy):
            p.setFont(ui_sm_b)
            p.setPen(self._c("--text-dim"))
            p.drawText(QPointF(x0, yy + sm_sz + 2), text)
            return yy + sm_sz + 18

        # Title bar
        tb_h = row_h + 6
        tb_rect = QRectF(x0, y, cw, tb_h)
        grad = QLinearGradient(tb_rect.topLeft(), tb_rect.bottomLeft())
        grad.setColorAt(0, self._c("--bg-header"))
        grad.setColorAt(1, self._c("--bg-header-end"))
        p.setPen(QPen(self._c("--border-default")))
        p.setBrush(QBrush(grad))
        p.drawRoundedRect(tb_rect, 3, 3)
        self._draw_glow(p, tb_rect, "--bg-header", "--bg-header-end", "--border-default")

        # Status dots
        dot_r = 4
        dot_y = y + tb_h / 2
        for i, var in enumerate(["--positive", "--accent", "--negative"]):
            cx = x0 + 14 + i * 16
            p.setPen(Qt.NoPen)
            p.setBrush(QBrush(self._c(var)))
            p.drawEllipse(QPointF(cx, dot_y), dot_r, dot_r)
            if self.show_glow and self._changed(var):
                p.setPen(QPen(QColor("#FFDD44"), 1, Qt.DashLine))
                p.setBrush(Qt.NoBrush)
                p.drawEllipse(QPointF(cx, dot_y), dot_r + 3, dot_r + 3)

        # Mode tabs
        tab_x = x0 + 70
        tab_w = 52
        for i, label in enumerate(["SETUP", "MONITOR", "SCAN"]):
            tx = tab_x + i * (tab_w + 4)
            is_on = (i == 1)
            tr = QRectF(tx, y + 4, tab_w, tb_h - 8)
            if is_on:
                p.setPen(QPen(self._c("--border-default")))
                p.setBrush(QBrush(self._c("--bg-elevated")))
                p.drawRoundedRect(tr, 2, 2)
            p.setFont(ui_sm_b)
            p.setPen(self._c("--accent") if is_on else self._c("--text-ghost"))
            p.drawText(tr, Qt.AlignCenter, label)

        # Time
        p.setFont(mono)
        p.setPen(self._c("--readout-dim"))
        p.drawText(QRectF(x0 + cw - 80, y, 72, tb_h),
                   Qt.AlignVCenter | Qt.AlignRight, "3:00:00")

        y += tb_h + pad

        # Main display
        disp_h = 120
        disp_rect = QRectF(x0, y, cw, disp_h)
        p.setPen(QPen(self._c("--border-default")))
        p.setBrush(QBrush(self._c("--bg-base")))
        p.drawRoundedRect(disp_rect, 3, 3)

        # Crosshair
        ch_col = self._c("--crosshair")
        ch_col.setAlpha(80)
        cx_mid = disp_rect.center().x()
        cy_mid = disp_rect.center().y()
        p.setPen(QPen(ch_col, 1, Qt.DashLine))
        p.drawLine(QPointF(x0 + 4, cy_mid), QPointF(x0 + cw - 4, cy_mid))
        p.drawLine(QPointF(cx_mid, y + 4), QPointF(cx_mid, y + disp_h - 4))
        ch_col.setAlpha(120)
        p.setPen(QPen(ch_col, 1))
        p.setBrush(Qt.NoBrush)
        p.drawEllipse(QPointF(cx_mid, cy_mid), 12, 12)
        if self.show_glow and self._changed("--crosshair"):
            p.setPen(QPen(QColor("#FFDD44"), 1.5, Qt.DashLine))
            p.drawEllipse(QPointF(cx_mid, cy_mid), 15, 15)

        # FPS overlay
        p.setFont(mono)
        tx = x0 + 8
        ty = y + base_sz + 6
        p.setPen(self._c("--readout-dim"))
        p.drawText(QPointF(tx, ty), "FPS:")
        p.setFont(mono_b)
        p.setPen(self._c("--readout"))
        p.drawText(QPointF(tx + fm.horizontalAdvance("FPS: "), ty), "60")
        p.setFont(mono)
        p.setPen(self._c("--readout-dim"))
        p.drawText(QPointF(tx, ty + base_sz + 4), "GAIN:")
        p.setFont(mono_b)
        p.setPen(self._c("--readout"))
        p.drawText(QPointF(tx + fm.horizontalAdvance("GAIN: "), ty + base_sz + 4), "12 dB")

        # Bounding box
        bx = x0 + int(cw * 0.2)
        bby = y + 20
        bw = int(cw * 0.12)
        bh = 60
        accent_a = self._c("--accent")
        accent_a.setAlpha(160)
        p.setPen(QPen(accent_a, 1, Qt.DashLine))
        p.setBrush(Qt.NoBrush)
        p.drawRect(QRectF(bx, bby, bw, bh))
        lbl_h = sm_sz + 3
        p.setPen(Qt.NoPen)
        p.setBrush(QBrush(self._c("--accent")))
        p.drawRect(QRectF(bx, bby - lbl_h, min(bw, 55), lbl_h))
        p.setFont(ui_sm_b)
        p.setPen(QColor("#000000"))
        p.drawText(QRectF(bx + 2, bby - lbl_h, 55, lbl_h), Qt.AlignVCenter, "TARGET")

        y += disp_h + pad

        # Widget panels (TCU & Console side by side)
        panel_gap = 8
        panel_w = (cw - panel_gap) // 2
        panel_h = 80
        hdr_h = 20

        for panel_idx, (title, px) in enumerate([
            ("TCU & LASER", x0),
            ("CONSOLE DATA", x0 + panel_w + panel_gap),
        ]):
            pr = QRectF(px, y, panel_w, panel_h)
            p.setPen(QPen(self._c("--border-default")))
            p.setBrush(QBrush(self._c("--bg-raised")))
            p.drawRoundedRect(pr, 3, 3)
            self._draw_glow(p, pr, "--bg-raised", "--border-default")

            # Header gradient
            hg = QLinearGradient(QPointF(px, y), QPointF(px, y + hdr_h))
            hg.setColorAt(0, self._c("--bg-header"))
            hg.setColorAt(1, self._c("--bg-header-end"))
            hp = QPainterPath()
            hp.addRoundedRect(QRectF(px, y, panel_w, hdr_h + 2), 3, 3)
            hp2 = QPainterPath()
            hp2.addRect(QRectF(px, y + hdr_h - 2, panel_w, 4))
            hp = hp.united(hp2)
            p.setPen(Qt.NoPen)
            p.setBrush(QBrush(hg))
            p.drawPath(hp)
            p.setPen(QPen(self._c("--border-default")))
            p.drawLine(QPointF(px, y + hdr_h), QPointF(px + panel_w, y + hdr_h))

            p.setFont(ui_sm_b)
            p.setPen(self._c("--accent"))
            p.drawText(QRectF(px + 6, y, panel_w - 12, hdr_h), Qt.AlignVCenter, title)

            body_y = y + hdr_h + 3
            body_h = panel_h - hdr_h - 6

            if panel_idx == 0:
                # TCU timing bars
                bars = [
                    ("DIST", 0.55, self._c("--tcu-distance")),
                    ("DEPTH", 0.18, self._c("--tcu-depth")),
                    ("MCP", 0.65, self._c("--tcu-mcp")),
                ]
                bar_h = 6
                bar_spacing = body_h // len(bars)
                for bi, (bl, bw_pct, bcol) in enumerate(bars):
                    row_cy = body_y + bi * bar_spacing + bar_spacing // 2
                    lbl_rect = QRectF(px + 4, row_cy - row_h // 2, 42, row_h)
                    p.setFont(ui_sm)
                    p.setPen(self._c("--text-dim"))
                    p.drawText(lbl_rect, Qt.AlignVCenter | Qt.AlignLeft, bl)
                    trk_x = px + 52
                    trk_w = panel_w - 52 - 48
                    trk_top = row_cy - bar_h // 2
                    trk = QRectF(trk_x, trk_top, trk_w, bar_h)
                    p.setPen(Qt.NoPen)
                    p.setBrush(QBrush(self._c("--track-bg")))
                    p.drawRoundedRect(trk, 2, 2)
                    fill_r = QRectF(trk_x + 1, trk_top + 1, (trk_w - 2) * bw_pct, bar_h - 2)
                    p.setBrush(QBrush(bcol))
                    p.drawRoundedRect(fill_r, 2, 2)
                    val_rect = QRectF(px + panel_w - 44, row_cy - row_h // 2, 38, row_h)
                    p.setFont(mono)
                    p.setPen(self._c("--readout"))
                    p.drawText(val_rect, Qt.AlignVCenter | Qt.AlignRight, "4 ns")
            else:
                # Console: mini log
                log_r = QRectF(px + 4, body_y, panel_w - 8, body_h)
                p.setPen(QPen(self._c("--border-subtle")))
                p.setBrush(QBrush(self._c("--bg-deepest")))
                p.drawRoundedRect(log_r, 2, 2)
                self._draw_glow(p, log_r, "--bg-deepest")
                p.setFont(mono)
                p.setPen(self._c("--readout-dim"))
                lh = base_sz + 3
                for li in range(min(3, max(1, int(body_h / lh)))):
                    p.drawText(QPointF(px + 8, body_y + 3 + (li + 1) * lh),
                               f"> DATA.run[{li}]")

        y += panel_h + 10

        # Buttons
        y = section_label("BUTTONS", y)
        btn_defs = [
            ("NORMAL",  "--bg-header-end", "--text-muted",  "--border-default"),
            ("ACTIVE",  "--bg-elevated",   "--accent",      "--border-default"),
            ("PRIMARY", "--positive-bg",   "--readout",     "--positive-border"),
            ("DANGER",  "--negative-bg",   "--negative",    "--negative-border"),
        ]
        btn_gap = 4
        btn_w = (cw - btn_gap * (len(btn_defs) - 1)) // len(btn_defs)
        btn_h = row_h
        for i, (label, bg_v, fg_v, bd_v) in enumerate(btn_defs):
            bx = x0 + i * (btn_w + btn_gap)
            br = QRectF(bx, y, btn_w, btn_h)
            p.setPen(QPen(self._c(bd_v)))
            p.setBrush(QBrush(self._c(bg_v)))
            p.drawRoundedRect(br, 3, 3)
            self._draw_glow(p, br, bg_v, fg_v, bd_v)
            p.setFont(ui_sm_b)
            p.setPen(self._c(fg_v))
            p.drawText(br, Qt.AlignCenter, label)
        y += btn_h + 10

        # Text levels
        y = section_label("TEXT LEVELS", y)
        text_samples = [
            ("Primary -- bright headings", "--text-primary"),
            ("Secondary -- body content", "--text-secondary"),
            ("Muted -- hover states", "--text-muted"),
            ("Dim -- labels and captions", "--text-dim"),
            ("Ghost -- faintest elements", "--text-ghost"),
        ]
        p.setFont(ui)
        line_h = base_sz + 6
        for txt, var in text_samples:
            p.setPen(self._c(var))
            if self.show_glow and self._changed(var):
                tw = QFontMetrics(ui).horizontalAdvance(txt)
                p.setPen(QPen(QColor("#FFDD44"), 1, Qt.DashLine))
                p.drawLine(QPointF(x0 + 4, y + 2), QPointF(x0 + 4 + tw, y + 2))
                p.setPen(self._c(var))
            p.drawText(QPointF(x0 + 4, y), txt)
            y += line_h
        y += 10

        # Surface layers
        y = section_label("SURFACE LAYERS", y)
        layers = [
            ("deepest", "--bg-deepest"),
            ("base", "--bg-base"),
            ("raised", "--bg-raised"),
            ("elevated", "--bg-elevated"),
            ("header", "--bg-header"),
        ]
        layer_gap = 3
        lw = (cw - layer_gap * (len(layers) - 1)) // len(layers)
        lh = row_h
        for i, (label, var) in enumerate(layers):
            lx = x0 + i * (lw + layer_gap)
            lr = QRectF(lx, y, lw, lh)
            p.setPen(QPen(self._c("--border-default")))
            p.setBrush(QBrush(self._c(var)))
            p.drawRoundedRect(lr, 3, 3)
            self._draw_glow(p, lr, var)
            p.setFont(mono)
            p.setPen(self._c("--text-dim"))
            p.drawText(lr, Qt.AlignCenter, label)
        y += lh + 10

        # Borders
        y = section_label("BORDERS", y)
        borders = [
            ("subtle", "--border-subtle"),
            ("default", "--border-default"),
            ("strong", "--border-strong"),
        ]
        bdr_gap = 4
        bdr_w = (cw - bdr_gap * (len(borders) - 1)) // len(borders)
        bdr_h = row_h
        for i, (label, var) in enumerate(borders):
            bx = x0 + i * (bdr_w + bdr_gap)
            br = QRectF(bx, y, bdr_w, bdr_h)
            p.setPen(QPen(self._c(var), 2))
            p.setBrush(QBrush(self._c("--bg-raised")))
            p.drawRoundedRect(br, 3, 3)
            self._draw_glow(p, br, var)
            p.setFont(mono)
            p.setPen(self._c("--text-muted"))
            p.drawText(br, Qt.AlignCenter, label)
        y += bdr_h + 10

        # Data values + map
        y = section_label("DATA VALUES", y)
        dv_h = row_h * 2
        dv_w = int(cw * 0.5)
        dv_r = QRectF(x0, y, dv_w, dv_h)
        p.setPen(QPen(self._c("--border-default")))
        p.setBrush(QBrush(self._c("--bg-raised")))
        p.drawRoundedRect(dv_r, 3, 3)
        self._draw_glow(p, dv_r, "--readout", "--readout-dim")

        tx = x0 + 6
        ty = y + base_sz + 6
        p.setFont(mono)
        p.setPen(self._c("--readout-dim"))
        p.drawText(QPointF(tx, ty), "FPS:")
        p.setFont(mono_b)
        p.setPen(self._c("--readout"))
        p.drawText(QPointF(tx + fm.horizontalAdvance("FPS: "), ty), "60")
        ty2 = ty + base_sz + 4
        p.setFont(mono)
        p.setPen(self._c("--readout-dim"))
        p.drawText(QPointF(tx, ty2), "DIST:")
        p.setFont(mono_b)
        p.setPen(self._c("--readout"))
        p.drawText(QPointF(tx + fm.horizontalAdvance("DIST: "), ty2), "1500 m")

        # Map swatch
        map_x = x0 + dv_w + pad
        map_w = cw - dv_w - pad
        mr = QRectF(map_x, y, map_w, dv_h)
        p.setPen(QPen(self._c("--border-default")))
        p.setBrush(QBrush(self._c("--bg-deepest")))
        p.drawRoundedRect(mr, 3, 3)
        self._draw_glow(p, mr, "--map-terrain")
        p.setPen(QPen(self._c("--map-terrain"), 1))
        p.drawLine(QPointF(mr.x() + 6, mr.bottom() - 4),
                   QPointF(mr.x() + mr.width() * 0.5, mr.y() + 4))
        p.drawLine(QPointF(mr.x() + mr.width() * 0.3, mr.bottom() - 4),
                   QPointF(mr.x() + mr.width() * 0.85, mr.y() + 4))
        p.setFont(ui_sm)
        p.setPen(self._c("--text-ghost"))
        p.drawText(mr, Qt.AlignCenter, "MAP")

        p.end()


# ── Color entry row (base tab) ───────────────────────────────────────────

class ColorEntry(QWidget):
    """A single variable row: dot indicator + swatch + hex input."""

    def __init__(self, var_name, description, parent=None):
        super().__init__(parent)
        self.var_name = var_name
        self._value = "#000000"
        self._default = "#000000"

        layout = QHBoxLayout(self)
        layout.setContentsMargins(6, 3, 6, 3)
        layout.setSpacing(6)

        # Modified indicator dot
        self.dot = QLabel("\u25cf")
        self.dot.setFixedWidth(14)
        self.dot.setStyleSheet("color: transparent; font-size: 12px;")
        layout.addWidget(self.dot)

        # Variable name
        self.name_lbl = QLabel(var_name[2:])
        self.name_lbl.setMinimumWidth(130)
        self.name_lbl.setFont(QFont("Consolas", 10))
        layout.addWidget(self.name_lbl)

        # Color swatch button
        self.swatch = QPushButton()
        self.swatch.setFixedSize(36, 24)
        self.swatch.setCursor(Qt.PointingHandCursor)
        self.swatch.clicked.connect(self._pick_color)
        layout.addWidget(self.swatch)

        # Hex input
        self.entry = QLineEdit()
        self.entry.setFixedWidth(100)
        self.entry.setFont(QFont("Consolas", 10))
        self.entry.returnPressed.connect(self._on_commit)
        self.entry.editingFinished.connect(self._on_commit)
        layout.addWidget(self.entry)

        # Description
        self.desc_lbl = QLabel(description)
        self.desc_lbl.setFont(QFont("Segoe UI", 9))
        layout.addWidget(self.desc_lbl)
        layout.addStretch()

    def set_value(self, value, is_default=False):
        self._value = value
        if is_default:
            self._default = value
        self.entry.setText(value)
        dc = c6(value)
        border = getattr(self, '_border_def', '#1e2838')
        accent = getattr(self, '_accent', '#f0a030')
        self.swatch.setStyleSheet(
            f"QPushButton {{ background: {dc}; border: 1px solid {border};"
            f" border-radius: 3px; }}"
            f"QPushButton:hover {{ border-color: {accent}; }}"
        )
        self._update_dot()

    def _update_dot(self):
        changed = self._value != self._default
        self.dot.setStyleSheet(
            f"color: {'#ffdd44' if changed else 'transparent'}; font-size: 12px;"
        )

    def _pick_color(self):
        dc = c6(self._value)
        dlg = QColorDialog(QColor(dc), self)
        dlg.setWindowTitle(f"Pick color for {self.var_name}")
        win = self.window()
        v = win.variables if hasattr(win, 'variables') else {}
        bg = v.get("--bg-raised", "#0C1018")
        bg_input = v.get("--bg-base", "#080C12")
        text = v.get("--text-primary", "#C8D8E8")
        text2 = v.get("--text-secondary", "#9AAABB")
        border = v.get("--border-default", "#1E2838")
        accent = v.get("--accent", "#F0A030")
        bg_btn = v.get("--bg-elevated", "#111820")
        border_strong = v.get("--border-strong", "#2A3848")
        bg_active = v.get("--bg-active", "#243040")
        btn_ss = (
            f"background: {bg_btn}; color: {text}; border: 1px solid {border_strong};"
            f" padding: 6px 14px; border-radius: 4px;"
        )
        dlg.setStyleSheet(
            f"QColorDialog {{ background: {bg}; }}"
            f"QLabel {{ color: {text}; }}"
            f"QSpinBox, QLineEdit {{ background: {bg_input}; color: {text}; border: 1px solid {border}; }}"
            f"QPushButton {{ {btn_ss} }}"
            f"QPushButton:hover {{ border-color: {accent}; background: {bg_active}; color: {text}; }}"
            f"QGroupBox {{ color: {text2}; }}"
        )
        for btn in dlg.findChildren(QPushButton):
            btn.setStyleSheet(
                f"QPushButton {{ {btn_ss} }}"
                f"QPushButton:hover {{ border-color: {accent}; background: {bg_active}; color: {text}; }}"
            )
        if dlg.exec_() != QColorDialog.Accepted:
            return
        color = dlg.currentColor()
        if color.isValid():
            new_val = color.name()
            if len(self._value) > 7:
                new_val += self._value[7:]
            self.set_value(new_val)
            self._notify()

    def _on_commit(self):
        val = self.entry.text().strip()
        if not val.startswith("#"):
            val = "#" + val
        if HEX_OK.match(val):
            self.set_value(val)
            self._notify()
        else:
            self.entry.setText(self._value)

    def _notify(self):
        win = self.window()
        if hasattr(win, '_on_var_changed'):
            win._on_var_changed(self.var_name, self._value)

    @property
    def value(self):
        return self._value


# ── Derived color entry row (advanced tab) ───────────────────────────────

class DerivedColorEntry(QWidget):
    """A derived variable row: swatch + hex input + lock/unlock toggle."""

    def __init__(self, var_name, parent=None):
        super().__init__(parent)
        self.var_name = var_name
        self._value = "#000000"
        self._default = "#000000"
        self._locked = True  # locked = auto-computed from base

        layout = QHBoxLayout(self)
        layout.setContentsMargins(6, 3, 6, 3)
        layout.setSpacing(6)

        # Lock/unlock button
        self.lock_btn = QPushButton("\U0001f512")
        self.lock_btn.setFixedSize(28, 24)
        self.lock_btn.setCursor(Qt.PointingHandCursor)
        self.lock_btn.setToolTip("Locked: auto-computed from base colors. Click to unlock for manual override.")
        self.lock_btn.clicked.connect(self._toggle_lock)
        layout.addWidget(self.lock_btn)

        # Modified indicator dot
        self.dot = QLabel("\u25cf")
        self.dot.setFixedWidth(14)
        self.dot.setStyleSheet("color: transparent; font-size: 12px;")
        layout.addWidget(self.dot)

        # Variable name
        self.name_lbl = QLabel(var_name[2:])
        self.name_lbl.setMinimumWidth(160)
        self.name_lbl.setFont(QFont("Consolas", 10))
        layout.addWidget(self.name_lbl)

        # Color swatch button
        self.swatch = QPushButton()
        self.swatch.setFixedSize(36, 24)
        self.swatch.setCursor(Qt.PointingHandCursor)
        self.swatch.clicked.connect(self._pick_color)
        layout.addWidget(self.swatch)

        # Hex input
        self.entry = QLineEdit()
        self.entry.setFixedWidth(100)
        self.entry.setFont(QFont("Consolas", 10))
        self.entry.returnPressed.connect(self._on_commit)
        self.entry.editingFinished.connect(self._on_commit)
        layout.addWidget(self.entry)

        layout.addStretch()

        self._apply_lock_visual()

    @property
    def locked(self):
        return self._locked

    def set_locked(self, locked):
        self._locked = locked
        self._apply_lock_visual()

    def _toggle_lock(self):
        self._locked = not self._locked
        self._apply_lock_visual()
        # If re-locked, request recompute so derived value is restored
        if self._locked:
            win = self.window()
            if hasattr(win, '_recompute_derived'):
                win._recompute_derived()

    def _apply_lock_visual(self):
        self.lock_btn.setText("\U0001f512" if self._locked else "\U0001f513")
        self.lock_btn.setToolTip(
            "Locked: auto-computed from base colors. Click to unlock for manual override."
            if self._locked else
            "Unlocked: manual override. Click to lock and auto-compute."
        )
        self.entry.setReadOnly(self._locked)
        # Dim the name label when locked
        opacity_style = "color: #4a5868;" if self._locked else ""
        if hasattr(self, '_name_style_base'):
            self.name_lbl.setStyleSheet(self._name_style_base + opacity_style)

    def set_value(self, value, is_default=False):
        self._value = value
        if is_default:
            self._default = value
        self.entry.setText(value)
        dc = c6(value)
        border = getattr(self, '_border_def', '#1e2838')
        accent = getattr(self, '_accent', '#f0a030')
        self.swatch.setStyleSheet(
            f"QPushButton {{ background: {dc}; border: 1px solid {border};"
            f" border-radius: 3px; }}"
            f"QPushButton:hover {{ border-color: {accent}; }}"
        )
        self._update_dot()

    def _update_dot(self):
        changed = self._value != self._default
        self.dot.setStyleSheet(
            f"color: {'#ffdd44' if changed else 'transparent'}; font-size: 12px;"
        )

    def _pick_color(self):
        if self._locked:
            return
        dc = c6(self._value)
        dlg = QColorDialog(QColor(dc), self)
        dlg.setWindowTitle(f"Pick color for {self.var_name}")
        win = self.window()
        v = win.variables if hasattr(win, 'variables') else {}
        bg = v.get("--bg-raised", "#0C1018")
        bg_input = v.get("--bg-base", "#080C12")
        text = v.get("--text-primary", "#C8D8E8")
        text2 = v.get("--text-secondary", "#9AAABB")
        border = v.get("--border-default", "#1E2838")
        accent = v.get("--accent", "#F0A030")
        bg_btn = v.get("--bg-elevated", "#111820")
        border_strong = v.get("--border-strong", "#2A3848")
        bg_active = v.get("--bg-active", "#243040")
        btn_ss = (
            f"background: {bg_btn}; color: {text}; border: 1px solid {border_strong};"
            f" padding: 6px 14px; border-radius: 4px;"
        )
        dlg.setStyleSheet(
            f"QColorDialog {{ background: {bg}; }}"
            f"QLabel {{ color: {text}; }}"
            f"QSpinBox, QLineEdit {{ background: {bg_input}; color: {text}; border: 1px solid {border}; }}"
            f"QPushButton {{ {btn_ss} }}"
            f"QPushButton:hover {{ border-color: {accent}; background: {bg_active}; color: {text}; }}"
            f"QGroupBox {{ color: {text2}; }}"
        )
        for btn in dlg.findChildren(QPushButton):
            btn.setStyleSheet(
                f"QPushButton {{ {btn_ss} }}"
                f"QPushButton:hover {{ border-color: {accent}; background: {bg_active}; color: {text}; }}"
            )
        if dlg.exec_() != QColorDialog.Accepted:
            return
        color = dlg.currentColor()
        if color.isValid():
            new_val = color.name()
            if len(self._value) > 7:
                new_val += self._value[7:]
            self.set_value(new_val)
            self._notify()

    def _on_commit(self):
        if self._locked:
            return
        val = self.entry.text().strip()
        if not val.startswith("#"):
            val = "#" + val
        if HEX_OK.match(val):
            self.set_value(val)
            self._notify()
        else:
            self.entry.setText(self._value)

    def _notify(self):
        win = self.window()
        if hasattr(win, '_on_derived_var_changed'):
            win._on_derived_var_changed(self.var_name, self._value)

    @property
    def value(self):
        return self._value


# ── Main window ───────────────────────────────────────────────────────────

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("GLI Color Scheme Editor")
        self.setMinimumSize(1200, 750)

        self.html_path = Path(__file__).parent / "prototype.html"
        self.variables = {}
        self.defaults = {}
        self.color_entries = {}       # base entries: var_name -> ColorEntry
        self.derived_entries = {}     # derived entries: var_name -> DerivedColorEntry
        self.theme_mode = "dark"

        central = QWidget()
        self.setCentralWidget(central)
        root_layout = QVBoxLayout(central)
        root_layout.setContentsMargins(0, 0, 0, 0)
        root_layout.setSpacing(0)

        self._build_toolbar(root_layout)
        self._build_body(root_layout)
        self._load_from_html()

    def _v(self, var, fallback=""):
        return self.variables.get(var, fallback)

    def _get_base_dict(self):
        """Build base color dict from current variables for compute_derived."""
        base = {}
        for var in BASE_VARS:
            base[var] = self.variables.get(var, BASE_INTERFACE.get(var, BASE_DETAILS.get(var, "#000000")))
        return base

    def _recompute_derived(self):
        """Recompute all locked derived values from current base colors."""
        base = self._get_base_dict()
        computed = compute_derived(base, self.theme_mode)
        for var_name, de in self.derived_entries.items():
            if de.locked:
                new_val = computed.get(var_name, "#000000")
                self.variables[var_name] = new_val
                de.set_value(new_val)
        self.preview.set_data(self.variables, self.defaults)

    def _apply_scheme(self):
        """Rebuild all editor UI stylesheets from current scheme variables."""
        bg_base = self._v("--bg-base", "#080C12")
        bg_raised = self._v("--bg-raised", "#0C1018")
        bg_elevated = self._v("--bg-elevated", "#111820")
        bg_header = self._v("--bg-header", "#161E2C")
        bg_active = self._v("--bg-active", "#243040")
        border_sub = self._v("--border-subtle", "#1A2030")
        border_def = self._v("--border-default", "#1E2838")
        border_str = self._v("--border-strong", "#2A3848")
        text_pri = self._v("--text-primary", "#C8D8E8")
        text_sec = self._v("--text-secondary", "#9AAABB")
        text_mut = self._v("--text-muted", "#7A8898")
        text_dim = self._v("--text-dim", "#4A5868")
        accent = self._v("--accent", "#F0A030")
        readout = self._v("--readout", "#5A9070")

        self.setStyleSheet(f"QMainWindow {{ background: {bg_raised}; }}")

        # Toolbar
        self._toolbar.setStyleSheet(f"QFrame {{ background: {bg_header}; }}")
        tb_btn_ss = (
            f"QPushButton {{ background: {bg_elevated}; color: {text_sec}; border: none;"
            f" border-radius: 3px; padding: 6px 14px; font-weight: bold; font-size: 10pt; }}"
            f"QPushButton:hover {{ background: {bg_active}; color: {text_pri}; }}"
        )
        for btn in self._tb_buttons:
            btn.setStyleSheet(tb_btn_ss)
        for sep in self._tb_seps:
            sep.setStyleSheet(f"background: {border_str};")
        self.glow_cb.setStyleSheet(
            f"QCheckBox {{ color: {text_sec}; font-size: 10pt; }}"
            f"QCheckBox::indicator {{ width: 14px; height: 14px; }}"
        )
        self.status.setStyleSheet(
            f"color: {readout}; font-family: Consolas; font-size: 10pt;"
        )

        # Theme toggle button
        self._theme_btn.setStyleSheet(tb_btn_ss)

        # Splitter
        self._splitter.setStyleSheet(
            f"QSplitter::handle {{ background: {border_def}; width: 4px; }}"
        )

        # Tab widget
        self._tab_widget.setStyleSheet(
            f"QTabWidget::pane {{ background: {bg_raised}; border: 1px solid {border_def}; border-top: none; }}"
            f"QTabBar::tab {{ background: {bg_elevated}; color: {text_mut}; padding: 8px 20px;"
            f" border: 1px solid {border_def}; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; }}"
            f"QTabBar::tab:selected {{ background: {bg_raised}; color: {accent}; }}"
            f"QTabBar::tab:hover {{ color: {text_pri}; }}"
        )

        # Scroll areas
        scroll_ss = (
            f"QScrollArea {{ background: {bg_raised}; border: none; }}"
            f"QScrollBar:vertical {{ background: {bg_base}; width: 8px; }}"
            f"QScrollBar::handle:vertical {{ background: {border_def}; border-radius: 4px; min-height: 30px; }}"
            f"QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{ height: 0; }}"
        )
        self._base_scroll.setStyleSheet(scroll_ss)
        self._base_scroll_content.setStyleSheet(f"background: {bg_raised};")
        self._adv_scroll.setStyleSheet(scroll_ss)
        self._adv_scroll_content.setStyleSheet(f"background: {bg_raised};")

        # Category groups (base tab)
        for grp, hdr in self._cat_widgets:
            grp.setStyleSheet(
                f"QFrame {{ background: {bg_raised}; border: 1px solid {border_def}; border-radius: 4px; }}"
            )
            hdr.setStyleSheet(
                f"QLabel {{ background: {bg_header}; color: {accent}; padding: 6px 12px;"
                f" border: none; border-top-left-radius: 4px; border-top-right-radius: 4px; }}"
            )

        # Category groups (advanced tab)
        for grp, hdr in self._adv_cat_widgets:
            grp.setStyleSheet(
                f"QFrame {{ background: {bg_raised}; border: 1px solid {border_def}; border-radius: 4px; }}"
            )
            hdr.setStyleSheet(
                f"QLabel {{ background: {bg_header}; color: {accent}; padding: 6px 12px;"
                f" border: none; border-top-left-radius: 4px; border-top-right-radius: 4px; }}"
            )

        # Color entries (base)
        for ce in self.color_entries.values():
            ce.name_lbl.setStyleSheet(f"color: {text_mut};")
            ce.entry.setStyleSheet(
                f"QLineEdit {{ background: {bg_base}; color: {readout}; border: 1px solid {border_def};"
                f" border-radius: 3px; padding: 3px 6px; }}"
                f"QLineEdit:focus {{ border-color: {accent}; }}"
            )
            ce.desc_lbl.setStyleSheet(f"color: {text_dim};")
            dc = c6(ce._value)
            ce.swatch.setStyleSheet(
                f"QPushButton {{ background: {dc}; border: 1px solid {border_def};"
                f" border-radius: 3px; }}"
                f"QPushButton:hover {{ border-color: {accent}; }}"
            )
            ce._border_def = border_def
            ce._accent = accent

        # Derived entries (advanced)
        for de in self.derived_entries.values():
            name_base_style = f"color: {text_mut};"
            de._name_style_base = name_base_style
            if de.locked:
                de.name_lbl.setStyleSheet(name_base_style + f"color: {text_dim};")
            else:
                de.name_lbl.setStyleSheet(name_base_style)
            locked_bg = bg_elevated if de.locked else bg_base
            de.entry.setStyleSheet(
                f"QLineEdit {{ background: {locked_bg}; color: {readout}; border: 1px solid {border_def};"
                f" border-radius: 3px; padding: 3px 6px; }}"
                f"QLineEdit:focus {{ border-color: {accent}; }}"
                f"QLineEdit:read-only {{ color: {text_dim}; }}"
            )
            dc = c6(de._value)
            de.swatch.setStyleSheet(
                f"QPushButton {{ background: {dc}; border: 1px solid {border_def};"
                f" border-radius: 3px; }}"
                f"QPushButton:hover {{ border-color: {accent}; }}"
            )
            de.lock_btn.setStyleSheet(
                f"QPushButton {{ background: {bg_elevated}; border: 1px solid {border_def};"
                f" border-radius: 3px; font-size: 12px; }}"
                f"QPushButton:hover {{ border-color: {accent}; }}"
            )
            de._border_def = border_def
            de._accent = accent

        # Preview container + label
        self._pv_container.setStyleSheet(f"background: {bg_raised};")
        self._pv_label.setStyleSheet(f"color: {accent}; padding: 4px;")
        self.preview.setStyleSheet(
            f"border: 1px solid {border_def}; border-radius: 4px;"
        )

    def _build_toolbar(self, parent_layout):
        self._toolbar = QFrame()
        self._toolbar.setFixedHeight(44)
        layout = QHBoxLayout(self._toolbar)
        layout.setContentsMargins(12, 6, 12, 6)

        self._tb_buttons = []
        self._tb_seps = []

        for text, slot in [
            ("Load HTML", self._load_from_html),
            ("Save HTML", self._save_to_html),
        ]:
            b = QPushButton(text)
            b.setCursor(Qt.PointingHandCursor)
            b.clicked.connect(slot)
            layout.addWidget(b)
            self._tb_buttons.append(b)

        self._add_sep(layout)

        for text, slot in [
            ("Export JSON", self._export_json),
            ("Import JSON", self._import_json),
        ]:
            b = QPushButton(text)
            b.setCursor(Qt.PointingHandCursor)
            b.clicked.connect(slot)
            layout.addWidget(b)
            self._tb_buttons.append(b)

        self._add_sep(layout)

        b = QPushButton("Reset to Default")
        b.setCursor(Qt.PointingHandCursor)
        b.clicked.connect(self._reset)
        layout.addWidget(b)
        self._tb_buttons.append(b)

        self._add_sep(layout)

        self.glow_cb = QCheckBox("Highlight Changes")
        self.glow_cb.setChecked(True)
        self.glow_cb.toggled.connect(self._toggle_glow)
        layout.addWidget(self.glow_cb)

        self._add_sep(layout)

        # Dark / Light toggle
        self._theme_btn = QPushButton("Dark")
        self._theme_btn.setCursor(Qt.PointingHandCursor)
        self._theme_btn.setToolTip("Toggle between Dark and Light derivation mode")
        self._theme_btn.clicked.connect(self._toggle_theme)
        layout.addWidget(self._theme_btn)

        layout.addStretch()

        self.status = QLabel("")
        layout.addWidget(self.status)

        parent_layout.addWidget(self._toolbar)

    def _add_sep(self, layout):
        sep = QFrame()
        sep.setFixedSize(1, 22)
        layout.addWidget(sep)
        self._tb_seps.append(sep)

    def _build_body(self, parent_layout):
        self._splitter = QSplitter(Qt.Horizontal)

        # Left: tabbed inputs
        self._tab_widget = QTabWidget()
        self._tab_widget.setMinimumWidth(500)

        # --- Base tab ---
        self._base_scroll = QScrollArea()
        self._base_scroll.setWidgetResizable(True)
        self._base_scroll_content = QWidget()
        base_layout = QVBoxLayout(self._base_scroll_content)
        base_layout.setContentsMargins(8, 8, 8, 8)
        base_layout.setSpacing(6)

        self._cat_widgets = []
        for cat_name, vars_list in BASE_CATEGORIES:
            grp = QFrame()
            grp_layout = QVBoxLayout(grp)
            grp_layout.setContentsMargins(0, 0, 0, 8)
            grp_layout.setSpacing(0)

            hdr = QLabel(cat_name.upper())
            hdr.setFont(QFont("Segoe UI", 10, QFont.Bold))
            grp_layout.addWidget(hdr)
            self._cat_widgets.append((grp, hdr))

            for var_name, desc in vars_list:
                ce = ColorEntry(var_name, desc)
                ce.setStyleSheet("QWidget { border: none; }")
                grp_layout.addWidget(ce)
                self.color_entries[var_name] = ce

            base_layout.addWidget(grp)

        base_layout.addStretch()
        self._base_scroll.setWidget(self._base_scroll_content)
        self._tab_widget.addTab(self._base_scroll, "Base")

        # --- Advanced tab ---
        self._adv_scroll = QScrollArea()
        self._adv_scroll.setWidgetResizable(True)
        self._adv_scroll_content = QWidget()
        adv_layout = QVBoxLayout(self._adv_scroll_content)
        adv_layout.setContentsMargins(8, 8, 8, 8)
        adv_layout.setSpacing(6)

        self._adv_cat_widgets = []
        for cat_name, var_list in DERIVED_CATEGORIES:
            grp = QFrame()
            grp_layout = QVBoxLayout(grp)
            grp_layout.setContentsMargins(0, 0, 0, 8)
            grp_layout.setSpacing(0)

            hdr = QLabel(cat_name.upper())
            hdr.setFont(QFont("Segoe UI", 10, QFont.Bold))
            grp_layout.addWidget(hdr)
            self._adv_cat_widgets.append((grp, hdr))

            for var_name in var_list:
                de = DerivedColorEntry(var_name)
                de.setStyleSheet("QWidget { border: none; }")
                grp_layout.addWidget(de)
                self.derived_entries[var_name] = de

            adv_layout.addWidget(grp)

        adv_layout.addStretch()
        self._adv_scroll.setWidget(self._adv_scroll_content)
        self._tab_widget.addTab(self._adv_scroll, "Advanced")

        self._splitter.addWidget(self._tab_widget)

        # Right: preview (no scroll)
        self._pv_container = QWidget()
        pv_layout = QVBoxLayout(self._pv_container)
        pv_layout.setContentsMargins(8, 8, 8, 8)

        self._pv_label = QLabel("LIVE PREVIEW")
        self._pv_label.setFont(QFont("Segoe UI", 10, QFont.Bold))
        pv_layout.addWidget(self._pv_label)

        self.preview = PreviewWidget()
        pv_layout.addWidget(self.preview)

        self._splitter.addWidget(self._pv_container)
        self._splitter.setSizes([520, 600])

        parent_layout.addWidget(self._splitter)

    # ── Data flow ─────────────────────────────────────────────────────

    def _on_var_changed(self, var_name, value):
        """Called when a base color entry changes."""
        self.variables[var_name] = value
        # Recompute all locked derived values
        self._recompute_derived()
        self.preview.set_data(self.variables, self.defaults)
        self._apply_scheme()

    def _on_derived_var_changed(self, var_name, value):
        """Called when an unlocked derived color entry is manually changed."""
        self.variables[var_name] = value
        self.preview.set_data(self.variables, self.defaults)
        self._apply_scheme()

    def _update_all(self):
        for var, ce in self.color_entries.items():
            ce.set_value(self.variables.get(var, "#000000"))
        for var, de in self.derived_entries.items():
            de.set_value(self.variables.get(var, "#000000"))
        self.preview.set_data(self.variables, self.defaults)

    def _toggle_theme(self):
        if self.theme_mode == "dark":
            self.theme_mode = "light"
            self._theme_btn.setText("Light")
        else:
            self.theme_mode = "dark"
            self._theme_btn.setText("Dark")
        self._recompute_derived()
        self._apply_scheme()
        self.status.setText(f"Theme mode: {self.theme_mode}")

    def _load_from_html(self):
        if not self.html_path.exists():
            QMessageBox.critical(self, "Error", f"Not found:\n{self.html_path}")
            return
        parsed = parse_root(self.html_path.read_text(encoding='utf-8'))
        if not parsed:
            QMessageBox.critical(self, "Error", "No :root block found")
            return
        self.variables = parsed
        self.defaults = dict(parsed)

        # Set base entries
        for var, ce in self.color_entries.items():
            val = parsed.get(var, "#000000")
            ce.set_value(val, is_default=True)

        # Set derived entries
        for var, de in self.derived_entries.items():
            val = parsed.get(var, "#000000")
            de.set_value(val, is_default=True)

        self.preview.set_data(self.variables, self.defaults)
        self._apply_scheme()
        self.status.setText(f"Loaded {len(parsed)} variables")

    def _save_to_html(self):
        if not self.html_path.exists():
            QMessageBox.critical(self, "Error", f"Not found:\n{self.html_path}")
            return
        html = self.html_path.read_text(encoding='utf-8')
        updated = update_root(html, self.variables)
        self.html_path.write_text(updated, encoding='utf-8')
        self.defaults = dict(self.variables)
        for var, ce in self.color_entries.items():
            ce._default = ce._value
            ce._update_dot()
        for var, de in self.derived_entries.items():
            de._default = de._value
            de._update_dot()
        self.preview.set_data(self.variables, self.defaults)
        self.status.setText("Saved to prototype.html")

    def _schemes_dir(self):
        d = self.html_path.parent / "color_schemes"
        d.mkdir(exist_ok=True)
        return d

    def _export_json(self):
        path, _ = QFileDialog.getSaveFileName(
            self, "Export", str(self._schemes_dir() / "color_scheme.json"),
            "JSON (*.json)")
        if path:
            # Export ALL variables: base + derived
            export_data = {}
            export_data["_theme_mode"] = self.theme_mode
            # Track which derived vars are unlocked
            unlocked = [v for v, de in self.derived_entries.items() if not de.locked]
            if unlocked:
                export_data["_unlocked"] = unlocked
            for var in ALL_VARS:
                if var in self.variables:
                    export_data[var] = self.variables[var]
            with open(path, 'w') as f:
                json.dump(export_data, f, indent=2)
            self.status.setText(f"Exported to {Path(path).name}")

    def _import_json(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "Import", str(self._schemes_dir()), "JSON (*.json)")
        if path:
            with open(path) as f:
                data = json.load(f)
            # Restore theme mode if present
            if "_theme_mode" in data:
                self.theme_mode = data["_theme_mode"]
                self._theme_btn.setText("Dark" if self.theme_mode == "dark" else "Light")
            # Restore unlock states if present
            unlocked_vars = data.get("_unlocked", [])
            for var in ALL_VARS:
                if var in data:
                    self.variables[var] = data[var]
            # Set lock states for derived entries
            for var, de in self.derived_entries.items():
                if var in unlocked_vars:
                    de.set_locked(False)
                else:
                    de.set_locked(True)
            self._update_all()
            self._apply_scheme()
            self.status.setText(f"Imported from {Path(path).name}")

    def _toggle_glow(self, checked):
        self.preview.show_glow = checked
        self.preview.update()

    def _reset(self):
        self.variables = dict(self.defaults)
        # Reset all derived entries to locked
        for de in self.derived_entries.values():
            de.set_locked(True)
        self._update_all()
        self._apply_scheme()
        self.status.setText("Reset to defaults")


if __name__ == '__main__':
    import ctypes
    try:
        ctypes.windll.shcore.SetProcessDpiAwareness(0)
    except Exception:
        pass
    import os
    os.environ["QT_AUTO_SCREEN_SCALE_FACTOR"] = "0"
    os.environ["QT_SCALE_FACTOR"] = "1"
    app = QApplication(sys.argv)
    win = MainWindow()
    win.show()
    sys.exit(app.exec_())
