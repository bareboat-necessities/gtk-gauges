#pragma once

#include <gtkmm.h>
#include <cairomm/context.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

class CircularGauge : public Gtk::DrawingArea {
public:
  struct Style {
    // Angles in degrees for scale sweep (e.g. -225 .. +45 = 270° arc)
    double start_deg = -225.0;
    double end_deg   =  45.0;

    int major_ticks = 9;   // number of major tick marks
    int minor_ticks = 4;   // minor ticks per major interval

    double value_precision = 0.0; // 0 => integer labels, 1 => 0.1, etc

    // Basic look. Keep it flat/clean (LVGL-ish).
    double ring_width_frac = 0.10;  // ring width relative to radius
    double tick_len_major_frac = 0.12;
    double tick_len_minor_frac = 0.07;

    // Label radii
    double label_radius_frac = 0.74;
    double value_radius_frac = 0.45;

    // Colors (simple sRGB). You can theme these later.
    Gdk::RGBA bg      = Gdk::RGBA("transparent");
    Gdk::RGBA ring    = Gdk::RGBA("#2a2f36");
    Gdk::RGBA face    = Gdk::RGBA("#111419");
    Gdk::RGBA tick    = Gdk::RGBA("#cfd6df");
    Gdk::RGBA text    = Gdk::RGBA("#e6edf6");
    Gdk::RGBA subtext = Gdk::RGBA("#a9b4c1");
    Gdk::RGBA needle  = Gdk::RGBA("#ff4d4d");
    Gdk::RGBA hub     = Gdk::RGBA("#e6edf6");
  };

  CircularGauge();
  ~CircularGauge() override = default;

  // Model
  void set_range(double min_v, double max_v);
  void set_value(double v);
  double value() const { return value_; }

  void set_title(std::string t);
  void set_unit(std::string u);

  // Scale labels (optional: override numeric majors)
  void set_major_labels(std::vector<std::string> labels);

  // Style
  Style& style() { return style_; }
  const Style& style() const { return style_; }

protected:
  void on_draw_gauge(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);

  // Overridables for specialization (wind angle gauge uses custom mapping/labels)
  virtual double value_to_angle_rad(double v) const; // maps value to angle along sweep
  virtual std::string format_major_label(int major_index, double major_value) const;
  virtual std::string format_value_readout(double v) const;

  // Helpers
  static double deg_to_rad(double d) { return d * M_PI / 180.0; }
  static void set_source_rgba(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::RGBA& c, double alpha_mul = 1.0);

  double min_v_ = 0.0;
  double max_v_ = 100.0;
  double value_ = 0.0;

  std::string title_ = "Gauge";
  std::string unit_  = "";

  std::vector<std::string> major_labels_override_;

  Style style_;
};
