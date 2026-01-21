// src/circular_gauge.cpp
#include "circular_gauge.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <string>
#include <utility>

// Needed on cairomm 1.16+ for Cairo::ToyFontFace::{Slant,Weight}
#include <cairomm/fontface.h>

CircularGauge::CircularGauge() {
  set_content_width(260);
  set_content_height(260);
  set_draw_func(sigc::mem_fun(*this, &CircularGauge::on_draw_gauge));
}

void CircularGauge::set_range(double min_v, double max_v) {
  min_v_ = min_v;
  max_v_ = std::max(min_v + 1e-9, max_v);
  value_ = std::clamp(value_, min_v_, max_v_);
  queue_draw();
}

void CircularGauge::set_value(double v) {
  value_ = std::clamp(v, min_v_, max_v_);
  queue_draw();
}

void CircularGauge::set_title(std::string t) {
  title_ = std::move(t);
  queue_draw();
}

void CircularGauge::set_unit(std::string u) {
  unit_ = std::move(u);
  queue_draw();
}

void CircularGauge::set_major_labels(std::vector<std::string> labels) {
  major_labels_override_ = std::move(labels);
  queue_draw();
}

void CircularGauge::set_zones(std::vector<Zone> z) {
  zones_ = std::move(z);
  queue_draw();
}

void CircularGauge::apply_theme(const Theme& theme) {
  style_ = theme.style;
  queue_draw();
}

double CircularGauge::deg_to_rad(double deg) {
  return deg * (std::numbers::pi / 180.0);
}

void CircularGauge::set_source_rgba(const Cairo::RefPtr<Cairo::Context>& cr,
                                    const Gdk::RGBA& c,
                                    double alpha_mul) {
  cr->set_source_rgba(c.get_red(), c.get_green(), c.get_blue(), c.get_alpha() * alpha_mul);
}

double CircularGauge::value_to_angle_rad(double v) const {
  const double t = (v - min_v_) / (max_v_ - min_v_);
  const double a0 = deg_to_rad(style_.start_deg);
  const double a1 = deg_to_rad(style_.end_deg);
  return a0 + t * (a1 - a0);
}

std::string CircularGauge::format_major_label(int major_index, double major_value) const {
  if (!major_labels_override_.empty()) {
    if (major_index >= 0 && major_index < static_cast<int>(major_labels_override_.size())) {
      return major_labels_override_[major_index];
    }
  }

  const double p = style_.value_precision;
  const double scale = std::pow(10.0, p);
  const double rounded = std::round(major_value * scale) / scale;

  if (p <= 0.0) {
    return std::to_string(static_cast<int>(std::llround(rounded)));
  }

  char buf[64];
  std::snprintf(buf, sizeof(buf), ("%." + std::to_string(static_cast<int>(p)) + "f").c_str(), rounded);
  return std::string(buf);
}

std::string CircularGauge::format_value_readout(double v) const {
  const double p = std::max(0.0, style_.value_precision);
  char buf[64];
  std::snprintf(buf, sizeof(buf), ("%." + std::to_string(static_cast<int>(p)) + "f").c_str(), v);
  return std::string(buf);
}

static void cairo_arc_visual(const Cairo::RefPtr<Cairo::Context>& cr,
                             double cx, double cy, double rad,
                             double a0, double a1) {
  if (a1 >= a0) cr->arc(cx, cy, rad, a0, a1);
  else         cr->arc_negative(cx, cy, rad, a0, a1);
}

void CircularGauge::draw_zone_arc(const Cairo::RefPtr<Cairo::Context>& cr,
                                  double cx, double cy, double r, double ring_w,
                                  const Zone& zone) const {
  const double v0 = std::clamp(zone.from_value, min_v_, max_v_);
  const double v1 = std::clamp(zone.to_value,   min_v_, max_v_);

  const double a0 = value_to_angle_rad(v0);
  const double a1 = value_to_angle_rad(v1);

  const double rad = (r - ring_w * 0.5) * style_.zone_radius_mul;
  const double w   = ring_w * style_.zone_width_mul;

  set_source_rgba(cr, zone.color, zone.alpha);
  cr->set_line_width(std::max(1.0, w));
  cr->set_line_cap(Cairo::Context::LineCap::BUTT);

  cr->begin_new_path();
  cairo_arc_visual(cr, cx, cy, rad, a0, a1);
  cr->stroke(
