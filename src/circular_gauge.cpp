// src/circular_gauge.cpp
#include "circular_gauge.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <string>
#include <vector>

// Needed for Cairo::ToyFontFace::{Slant,Weight} on cairomm >= 1.16.
// (Cairo::FontSlant / Cairo::FontWeight were removed/relocated.)
#include <cairomm/fontface.h>

namespace {

constexpr double kDegToRad = std::numbers::pi / 180.0;

inline double clampd(double v, double lo, double hi) { return std::clamp(v, lo, hi); }

inline void set_rgba(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::RGBA& c, double a_mul = 1.0) {
  cr->set_source_rgba(c.get_red(), c.get_green(), c.get_blue(), c.get_alpha() * a_mul);
}

// Map a gauge "value" (in degrees, -180..+180 for wind) to a polar angle in radians for Cairo.
// We want 0° at top, +90° to the right, -90° to the left, ±180° at bottom.
inline double value_to_angle_rad(double value_deg) {
  const double ang_deg = value_deg - 90.0; // 0 -> -90 (top)
  return ang_deg * kDegToRad;
}

inline void draw_centered_text(const Cairo::RefPtr<Cairo::Context>& cr,
                               const std::string& text,
                               double x, double y) {
  Cairo::TextExtents te;
  cr->get_text_extents(text, te);
  cr->move_to(x - (te.width / 2.0 + te.x_bearing), y - (te.height / 2.0 + te.y_bearing));
  cr->show_text(text);
}

inline std::string fmt_int(int v) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%d", v);
  return std::string(buf);
}

} // namespace

CircularGauge::CircularGauge() {
  set_hexpand(true);
  set_vexpand(true);
  set_content_width(240);
  set_content_height(240);

  // gtkmm4 DrawingArea API
  set_draw_func(sigc::mem_fun(*this, &CircularGauge::on_draw_gauge));
}

void CircularGauge::set_style(const CircularGaugeStyle& s) {
  style_ = s;
  queue_draw();
}

void CircularGauge::set_range(double min_value, double max_value) {
  min_value_ = min_value;
  max_value_ = max_value;
  value_ = clampd(value_, min_value_, max_value_);
  queue_draw();
}

void CircularGauge::set_value(double v) {
  value_ = clampd(v, min_value_, max_value_);
  queue_draw();
}

double CircularGauge::value() const { return value_; }

void CircularGauge::set_units(std::string units) {
  units_ = std::move(units);
  queue_draw();
}

void CircularGauge::set_title(std::string title) {
  title_ = std::move(title);
  queue_draw();
}

void CircularGauge::set_major_tick_step(double step) {
  major_tick_step_ = std::max(1e-6, step);
  queue_draw();
}

void CircularGauge::set_minor_ticks_per_major(int n) {
  minor_ticks_per_major_ = std::max(0, n);
  queue_draw();
}

void CircularGauge::set_show_numeric_labels(bool on) {
  show_numeric_labels_ = on;
  queue_draw();
}

void CircularGauge::set_zones(std::vector<GaugeZone> zones) {
  zones_ = std::move(zones);
  queue_draw();
}

void CircularGauge::on_draw_gauge(Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
  const double w = static_cast<double>(width);
  const double h = static_cast<double>(height);

  const double cx = w * 0.5;
  const double cy = h * 0.5;

  const double pad = style_.padding_px;
  const double r = std::max(1.0, std::min(w, h) * 0.5 - pad);

  // Background
  cr->save();
  set_rgba(cr, style_.bg);
  cr->paint();
  cr->restore();

  // Outer rim
  cr->save();
  set_rgba(cr, style_.rim);
  cr->set_line_width(style_.rim_width_px);
  cr->arc(cx, cy, r - style_.rim_width_px * 0.5, 0, 2.0 * std::numbers::pi);
  cr->stroke();
  cr->restore();

  // Inner face
  cr->save();
  set_rgba(cr, style_.face);
  cr->arc(cx, cy, r - style_.rim_width_px - style_.face_inset_px, 0, 2.0 * std::numbers::pi);
  cr->fill();
  cr->restore();

  const double ring_r = r - style_.rim_width_px - style_.face_inset_px;
  const double zone_r0 = ring_r - style_.zone_ring_inset_px - style_.zone_ring_width_px;
  const double zone_r1 = zone_r0 + style_.zone_ring_width_px;

  // Colored zones (arcs)
  if (!zones_.empty()) {
    cr->save();
    cr->set_line_cap(Cairo::Context::LineCap::ROUND);
    cr->set_line_width(style_.zone_ring_width_px);

    for (const auto& z : zones_) {
      // Normalize and clamp
      const double v0 = clampd(z.from, min_value_, max_value_);
      const double v1 = clampd(z.to,   min_value_, max_value_);

      // Draw in value order; if caller wants wrap-around, they should split into two zones.
      const double a0 = value_to_angle_rad(v0);
      const double a1 = value_to_angle_rad(v1);

      set_rgba(cr, z.color, style_.zone_alpha);

      cr->arc(cx, cy, (zone_r0 + zone_r1) * 0.5, a0, a1);
      cr->stroke();
    }

    cr->restore();
  }

  // Tick ring radii
  const double tick_r_outer = ring_r - style_.tick_outer_inset_px;
  const double tick_r_major = tick_r_outer - style_.tick_major_len_px;
  const double tick_r_minor = tick_r_outer - style_.tick_minor_len_px;
  const double label_r = tick_r_major - style_.label_inset_px;

  // Ticks
  cr->save();
  set_rgba(cr, style_.tick);
  cr->set_line_cap(Cairo::Context::LineCap::ROUND);

  const double major_step = major_tick_step_;
  const int minor_per_major = minor_ticks_per_major_;
  const double minor_step = (minor_per_major > 0) ? (major_step / (minor_per_major + 1)) : 0.0;

  // Avoid duplicating the ±180 overlap (both map to bottom). Iterate half-open [min, max).
  // If you *really* want both labels, do it at the demo/widget level with custom labels.
  auto draw_tick = [&](double v, bool major) {
    const double a = value_to_angle_rad(v);
    const double cs = std::cos(a);
    const double sn = std::sin(a);

    const double r0 = major ? tick_r_major : tick_r_minor;
    const double r1 = tick_r_outer;

    cr->set_line_width(major ? style_.tick_major_width_px : style_.tick_minor_width_px);
    cr->move_to(cx + r0 * cs, cy + r0 * sn);
    cr->line_to(cx + r1 * cs, cy + r1 * sn);
    cr->stroke();
  };

  // Major ticks + minors between them
  for (double v = min_value_; v < max_value_ - 1e-9; v += major_step) {
    draw_tick(v, true);

    if (minor_per_major > 0) {
      for (int i = 1; i <= minor_per_major; ++i) {
        const double vm = v + minor_step * i;
        if (vm < max_value_ - 1e-9) draw_tick(vm, false);
      }
    }
  }

  cr->restore();

  // Numeric labels
  if (show_numeric_labels_) {
    cr->save();
    set_rgba(cr, style_.text);

    cr->select_font_face(
        style_.font_family,
        Cairo::ToyFontFace::Slant::NORMAL,
        Cairo::ToyFontFace::Weight::NORMAL);

    cr->set_font_size(style_.label_font_px);

    for (double v = min_value_; v < max_value_ - 1e-9; v += major_step) {
      // Skip duplicate bottom label at -180 if you prefer only "180".
      // Here we keep -180 label; the demo can override if desired.
      const double a = value_to_angle_rad(v);
      const double cs = std::cos(a);
      const double sn = std::sin(a);

      const double x = cx + label_r * cs;
      const double y = cy + label_r * sn;

      const int iv = static_cast<int>(std::lround(v));
      draw_centered_text(cr, fmt_int(iv), x, y);
    }

    cr->restore();
  }

  // Title + units (top/bottom)
  cr->save();
  set_rgba(cr, style_.text);

  cr->select_font_face(
      style_.font_family,
      Cairo::ToyFontFace::Slant::NORMAL,
      Cairo::ToyFontFace::Weight::BOLD);

  if (!title_.empty()) {
    cr->set_font_size(style_.title_font_px);
    draw_centered_text(cr, title_, cx, cy - ring_r * 0.42);
  }

  if (!units_.empty()) {
    cr->select_font_face(
        style_.font_family,
        Cairo::ToyFontFace::Slant::NORMAL,
        Cairo::ToyFontFace::Weight::NORMAL);
    cr->set_font_size(style_.units_font_px);
    draw_centered_text(cr, units_, cx, cy + ring_r * 0.52);
  }

  cr->restore();

  // Needle
  const double v = clampd(value_, min_value_, max_value_);
  const double a = value_to_angle_rad(v);
  const double cs = std::cos(a);
  const double sn = std::sin(a);

  const double needle_len = ring_r - style_.needle_tip_inset_px;
  const double needle_back = style_.needle_back_len_px;

  // Needle shadow (subtle)
  cr->save();
  cr->set_line_cap(Cairo::Context::LineCap::ROUND);
  cr->set_line_width(style_.needle_width_px);
  set_rgba(cr, style_.needle_shadow, style_.needle_shadow_alpha);
  cr->move_to(cx - needle_back * cs + style_.needle_shadow_dx, cy - needle_back * sn + style_.needle_shadow_dy);
  cr->line_to(cx + needle_len * cs + style_.needle_shadow_dx, cy + needle_len * sn + style_.needle_shadow_dy);
  cr->stroke();
  cr->restore();

  // Needle main
  cr->save();
  cr->set_line_cap(Cairo::Context::LineCap::ROUND);
  cr->set_line_width(style_.needle_width_px);
  set_rgba(cr, style_.needle);
  cr->move_to(cx - needle_back * cs, cy - needle_back * sn);
  cr->line_to(cx + needle_len * cs, cy + needle_len * sn);
  cr->stroke();
  cr->restore();

  // Knob
  cr->save();
  set_rgba(cr, style_.knob);
  cr->arc(cx, cy, style_.knob_radius_px, 0, 2.0 * std::numbers::pi);
  cr->fill();

  set_rgba(cr, style_.knob_highlight, style_.knob_highlight_alpha);
  cr->arc(cx, cy, style_.knob_radius_px * 0.55, 0, 2.0 * std::numbers::pi);
  cr->fill();
  cr->restore();

  // Center value readout (optional; looks LVGL-ish)
  if (style_.show_center_value) {
    cr->save();
    set_rgba(cr, style_.text);

    cr->select_font_face(
        style_.font_family,
        Cairo::ToyFontFace::Slant::NORMAL,
        Cairo::ToyFontFace::Weight::BOLD);

    cr->set_font_size(style_.center_value_font_px);

    const int iv = static_cast<int>(std::lround(value_));
    draw_centered_text(cr, fmt_int(iv), cx, cy + ring_r * 0.18);

    cr->restore();
  }
}
