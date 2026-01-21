#pragma once

#include <gtkmm.h>
#include <cairomm/context.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numbers>

class CircularGauge : public Gtk::DrawingArea {
public:
  struct Zone {
    double from_value = 0.0;  // in gauge units
    double to_value   = 0.0;  // in gauge units
    Gdk::RGBA color   = Gdk::RGBA("#00ff00");
    double alpha      = 1.0;
  };

  struct Style {
    // Scale sweep (degrees). Generic arc gauges map [min..max] onto [start..end].
    double start_deg = -225.0;
    double end_deg   =   45.0;

    int major_ticks = 9;
    int minor_ticks = 4;
    double value_precision = 0.0;

    // Geometry
    double ring_width_frac = 0.10;
    double tick_len_major_frac = 0.12;
    double tick_len_minor_frac = 0.07;

    double label_radius_frac = 0.74;

    // Value readout vertical offset from center (in units of radius).
    // Positive moves down.
    double value_radius_frac = 0.22;

    // Zone arc placement (relative to ring)
    double zone_width_mul = 0.55;   // zone arc width relative to ring width
    double zone_radius_mul = 0.88;  // zone arc radius relative to (r - ring_w*0.5)

    // Colors
    Gdk::RGBA bg      = Gdk::RGBA("transparent");
    Gdk::RGBA ring    = Gdk::RGBA("#2a2f36");
    Gdk::RGBA face    = Gdk::RGBA("#111419");
    Gdk::RGBA tick    = Gdk::RGBA("#cfd6df");
    Gdk::RGBA text    = Gdk::RGBA("#e6edf6");
    Gdk::RGBA subtext = Gdk::RGBA("#a9b4c1");
    Gdk::RGBA needle  = Gdk::RGBA("#ff4d4d");
    Gdk::RGBA hub     = Gdk::RGBA("#e6edf6");

    // Typography
    std::string font_family = "Sans";
  };

  struct Theme {
    Style style;
    double corner_radius = 0.0; // reserved
  };

  CircularGauge();
  ~CircularGauge() override = default;

  // Model
  void set_range(double min_v, double max_v);
  void set_value(double v);
  double value() const { return value_; }

  void set_title(std::string t);
  void set_unit(std::string u);

  // Labels
  void set_major_labels(std::vector<std::string> labels);

  // Zones
  void set_zones(std::vector<Zone> z);
  const std::vector<Zone>& zones() const { return zones_; }

  // Theming
  void apply_theme(const Theme& theme);
  Style& style() { return style_; }
  const Style& style() const { return style_; }

protected:
  void on_draw_gauge(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);

  // Mapping + formatting hooks
  virtual double value_to_angle_rad(double v) const; // monotone mapping by default
  virtual std::string format_major_label(int major_index, double major_value) const;
  virtual std::string format_value_readout(double v) const;

  // Helpers
  static double deg_to_rad(double d) { return d * std::numbers::pi / 180.0; }
  static void set_source_rgba(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::RGBA& c, double alpha_mul = 1.0);

  // drawing helpers for zones
  void draw_zone_arc(const Cairo::RefPtr<Cairo::Context>& cr,
                     double cx, double cy, double r, double ring_w,
                     const Zone& zone) const;

  double min_v_ = 0.0;
  double max_v_ = 100.0;
  double value_ = 0.0;

  std::string title_ = "Gauge";
  std::string unit_  = "";

  std::vector<std::string> major_labels_override_;
  std::vector<Zone> zones_;

  Style style_;
};
