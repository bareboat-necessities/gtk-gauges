# gtk-gauges (gtkmm) — LVGL-ish Circular Gauges + Sailing Wind Demo

A small **modern C++20** project using **GTK4 + gtkmm-4.0** that implements reusable **circular gauge widgets** (needle + ticks + labels) with an **LVGL-inspired** flat/clean look.

Includes a **sailboat wind instrument demo**:
- **AWA** (Apparent Wind Angle) gauge: **-180° .. +180°** (port negative, starboard positive)
- **AWS** (Apparent Wind Speed) gauge: **0 .. 40 kn**
- **Theming** (palette/typography/sizing) via a single theme struct

---

## Features

* **GTK4 / gtkmm-4.0**
* **Cairo-rendered** widget using `Gtk::DrawingArea`
* Reusable `CircularGauge`:

    * scale sweep, major/minor ticks
    * major labels + centered readout
    * needle + hub
    * **colored arc zones** (value ranges)
    * **themeable** (colors, font family, geometry)
* Wind instrument demo panel with live animated signal

---

## Requirements

* C++20 compiler (GCC/Clang)
* CMake ≥ 3.20
* GTK4 + gtkmm-4.0 development packages
* pkg-config

### Package hints

**Debian/Ubuntu:**

* `libgtkmm-4.0-dev`

**Fedora:**

* `gtkmm4.0-devel`

**Arch:**

* `gtkmm-4.0`

(Exact package names vary by distro.)

---

## Build & Run

```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/wind_demo
```

---

## Usage / Customization

### 1) Adjust sailing zones (AWA)

Zones are configured in `WindInstrumentPanel::apply_theme()`:

```cpp
// Red no-go around the bow:
zones.push_back({-35.0,  35.0, theme_.accent_red,   0.90});

// Green close-hauled window:
zones.push_back({-60.0, -35.0, theme_.accent_green, 0.85});
zones.push_back({ 35.0,  60.0, theme_.accent_green, 0.85});
```

Suggested typical alternatives:

* no-go: ±40°
* green: 40..70° (wider “good” window)

### 2) Theme (LVGL-ish look)

A single theme struct controls the look:

```cpp
SailTheme t;
t.panel_bg = Gdk::RGBA("#0b0e12");

t.gauge.style.face   = Gdk::RGBA("#10151c");
t.gauge.style.ring   = Gdk::RGBA("#27313b");
t.gauge.style.tick   = Gdk::RGBA("#d7dee8");
t.gauge.style.text   = Gdk::RGBA("#eef4ff");
t.gauge.style.subtext= Gdk::RGBA("#9fb0c3");
t.gauge.style.needle = Gdk::RGBA("#ff453a");
t.gauge.style.hub    = Gdk::RGBA("#eef4ff");

t.gauge.style.font_family = "Sans";

// geometry knobs
t.gauge.style.ring_width_frac = 0.10;
t.gauge.style.tick_len_major_frac = 0.12;
t.gauge.style.tick_len_minor_frac = 0.07;
```

Apply it in `main.cpp`:

```cpp
panel_.apply_theme(t);
```

### 3) Feeding real data

The demo uses a timer to animate values. Replace in `DemoWindow::on_tick()`:

```cpp
panel_.set_wind(awa_deg, aws_kn);
```

---

## Notes / Design

* Rendering is done in `CircularGauge::on_draw_gauge()` using Cairo.
* Mapping from value → needle angle is customizable by overriding `value_to_angle_rad()`.
* Zones are drawn as arcs under ticks/labels for a clean instrument look.

---

## Roadmap (nice next steps)

* Needle smoothing (critically damped motion)
* More zone types (bands, gradients, markers)
* Optional “target” bug / second needle (e.g. TWA vs AWA)
* Better typography scaling + label collision avoidance
* Packaging (Meson option), CI builds, screenshots


---

## Contributing

PRs welcome:

* keep it small, readable, and idiomatic C++
* prefer simple structs over heavy abstraction
* avoid global state; keep theming explicit

