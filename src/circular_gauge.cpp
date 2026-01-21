#include "circular_gauge.hpp"

CircularGauge::CircularGauge() {
  set_content_width(260);
  set_content_height(260);

  set_draw_func(sigc::mem_fun(*this, &CircularGauge::on_draw_gauge));
}

void CircularGauge::set_range(double min_v, double max_v) {
  min_v_ = min_v;
  max_v_ = std::max(min_v + 1e-9, max_v);
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

double CircularGauge::value_to_angle_rad(double v) const {
  // Map [min..max] to [start..end]
  const double t = (v - min_v_) / (max_v_ - min_v_);
  const double a0 = deg_to_rad(style_.start_deg);
  const double a1 = deg_to_rad(style_.end_deg);
  return a0 + t * (a1 - a0);
}

std::string CircularGauge::format_major_label(int major_index, double major_value) const {
  if (!major_labels_override_.empty()) {
    if (major_index >= 0 && major_index < (int)major_labels_override_.size()) {
      return major_labels_override_[major_index];
    }
  }

  // Numeric label
  const double p = style_.value_precision;
  const double scale = std::pow(10.0, p);
  const double rounded = std::round(major_value * scale) / scale;

  if (p <= 0.0) {
    return std::to_string((int)std::llround(rounded));
  } else {
    char buf[64];
    std::snprintf(buf, sizeof(buf), ("%." + std::to_string((int)p) + "f").c_str(), rounded);
    return buf;
  }
}

std::string CircularGauge::format_value_readout(double v) const {
  // Large centered value readout
  const double p = std::max(0.0, style_.value_precision);
  char buf[64];
  std::snprintf(buf, sizeof(buf), ("%." + std::to_string((int)p) + "f").c_str(), v);
  return buf;
}

void CircularGauge::set_source_rgba(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::RGBA& c, double alpha_mul) {
  cr->set_source_rgba(c.get_red(), c.get_green(), c.get_blue(), c.get_alpha() * alpha_mul);
}

void CircularGauge::on_draw_gauge(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
  const double cx = width * 0.5;
  const double cy = height * 0.5;
  const double r  = std::min(width, height) * 0.5 * 0.95;

  // Background (transparent by default)
  if (style_.bg.get_alpha() > 0.0) {
    set_source_rgba(cr, style_.bg);
    cr->rectangle(0, 0, width, height);
    cr->fill();
  }

  // Face
  set_source_rgba(cr, style_.face);
  cr->arc(cx, cy, r, 0, 2 * M_PI);
  cr->fill();

  // Ring
  const double ring_w = r * style_.ring_width_frac;
  set_source_rgba(cr, style_.ring);
  cr->set_line_width(ring_w);
  cr->arc(cx, cy, r - ring_w * 0.5, 0, 2 * M_PI);
  cr->stroke();

  // Scale arc ticks
  const int majors = std::max(2, style_.major_ticks);
  const int minors = std::max(0, style_.minor_ticks);

  const double a0 = deg_to_rad(style_.start_deg);
  const double a1 = deg_to_rad(style_.end_deg);

  const double tick_r_outer = r - ring_w * 0.65;
  const double tick_major_len = r * style_.tick_len_major_frac;
  const double tick_minor_len = r * style_.tick_len_minor_frac;

  // Helper to draw a single tick
  auto draw_tick = [&](double ang, double len, double lw, double alpha) {
    const double x0 = cx + std::cos(ang) * tick_r_outer;
    const double y0 = cy + std::sin(ang) * tick_r_outer;
    const double x1 = cx + std::cos(ang) * (tick_r_outer - len);
    const double y1 = cy + std::sin(ang) * (tick_r_outer - len);

    set_source_rgba(cr, style_.tick, alpha);
    cr->set_line_width(lw);
    cr->set_line_cap(Cairo::Context::LineCap::ROUND);
    cr->move_to(x0, y0);
    cr->line_to(x1, y1);
    cr->stroke();
  };

  // Major labels
  cr->save();
  set_source_rgba(cr, style_.text);
  cr->select_font_face("Sans", Cairo::Context::Slant::NORMAL, Cairo::Context::Weight::SEMIBOLD);

  for (int i = 0; i < majors; ++i) {
    const double t = (double)i / (double)(majors - 1);
    const double ang = a0 + t * (a1 - a0);

    // major tick
    draw_tick(ang, tick_major_len, std::max(1.5, r * 0.012), 1.0);

    // minor ticks between majors
    if (i < majors - 1 && minors > 0) {
      for (int m = 1; m <= minors; ++m) {
        const double tm = (t + (double)m / (double)(minors + 1) / (double)(majors - 1));
        const double angm = a0 + tm * (a1 - a0);
        draw_tick(angm, tick_minor_len, std::max(1.0, r * 0.008), 0.8);
      }
    }

    // label
    const double major_value = min_v_ + t * (max_v_ - min_v_);
    const std::string label = format_major_label(i, major_value);

    const double lr = r * style_.label_radius_frac;
    const double lx = cx + std::cos(ang) * lr;
    const double ly = cy + std::sin(ang) * lr;

    cr->set_font_size(std::max(10.0, r * 0.085));
    Cairo::TextExtents te;
    cr->get_text_extents(label, te);

    cr->move_to(lx - (te.width * 0.5 + te.x_bearing), ly - (te.height * 0.5 + te.y_bearing));
    cr->show_text(label);
  }
  cr->restore();

  // Title + unit
  {
    cr->save();
    cr->select_font_face("Sans", Cairo::Context::Slant::NORMAL, Cairo::Context::Weight::NORMAL);
    cr->set_font_size(std::max(10.0, r * 0.070));
    set_source_rgba(cr, style_.subtext);

    Cairo::TextExtents te;
    cr->get_text_extents(title_, te);
    cr->move_to(cx - (te.width * 0.5 + te.x_bearing), cy - r * 0.18);
    cr->show_text(title_);

    if (!unit_.empty()) {
      const std::string u = unit_;
      cr->get_text_extents(u, te);
      cr->move_to(cx - (te.width * 0.5 + te.x_bearing), cy + r * 0.23);
      cr->show_text(u);
    }
    cr->restore();
  }

  // Center value readout
  {
    cr->save();
    cr->select_font_face("Sans", Cairo::Context::Slant::NORMAL, Cairo::Context::Weight::BOLD);
    cr->set_font_size(std::max(14.0, r * 0.17));
    set_source_rgba(cr, style_.text);

    const std::string vtxt = format_value_readout(value_);
    Cairo::TextExtents te;
    cr->get_text_extents(vtxt, te);
    cr->move_to(cx - (te.width * 0.5 + te.x_bearing), cy + r * style_.value_radius_frac * 0.15);
    cr->show_text(vtxt);
    cr->restore();
  }

  // Needle
  {
    const double ang = value_to_angle_rad(value_);

    const double needle_r = r * 0.72;
    const double base_r   = r * 0.10;

    const double nx = cx + std::cos(ang) * needle_r;
    const double ny = cy + std::sin(ang) * needle_r;

    set_source_rgba(cr, style_.needle);
    cr->set_line_width(std::max(2.0, r * 0.02));
    cr->set_line_cap(Cairo::Context::LineCap::ROUND);
    cr->move_to(cx, cy);
    cr->line_to(nx, ny);
    cr->stroke();

    // hub
    set_source_rgba(cr, style_.hub);
    cr->arc(cx, cy, base_r, 0, 2 * M_PI);
    cr->fill();
  }
}
