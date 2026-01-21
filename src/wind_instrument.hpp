#pragma once

#include "circular_gauge.hpp"

// LVGL-ish theme bundle for the demo
struct SailTheme {
  CircularGauge::Theme gauge;
  Gdk::RGBA panel_bg = Gdk::RGBA("#0b0e12");

  // Zone colors
  Gdk::RGBA accent_red   = Gdk::RGBA("#ff3b30");
  Gdk::RGBA accent_green = Gdk::RGBA("#34c759");

  // "No-go" should be a usual caution color (not red)
  Gdk::RGBA accent_no_go = Gdk::RGBA("#ff9f0a"); // amber
};

// Apparent wind angle: -180..+180 (port -, starboard +).
class WindAngleGauge final : public CircularGauge {
public:
  WindAngleGauge();

  void set_angle_deg(double deg); // clamps to [-180, 180]
  void set_speed_kn(double kn) { speed_kn_ = kn; queue_draw(); }

  // IMPORTANT: theme application overwrites style_, so we re-apply gauge geometry after theming.
  void apply_theme(const CircularGauge::Theme& theme);

protected:
  double value_to_angle_rad(double v) const override;
  std::string format_major_label(int major_index, double major_value) const override;
  std::string format_value_readout(double v) const override;

private:
  static double clamp_180(double deg);
  void apply_geometry_overrides_();

  double speed_kn_ = 0.0;
};

// Wind speed gauge: standard arc gauge
class WindSpeedGauge final : public CircularGauge {
public:
  WindSpeedGauge();
  void set_speed_kn(double kn) { set_value(kn); }

  void apply_theme(const CircularGauge::Theme& theme);

private:
  void apply_geometry_overrides_();
};

class WindInstrumentPanel final : public Gtk::Box {
public:
  WindInstrumentPanel();

  void apply_theme(const SailTheme& t);
  void set_wind(double awa_deg, double aws_kn);

private:
  WindAngleGauge angle_;
  WindSpeedGauge speed_;

  Gtk::Label readout_;
  SailTheme theme_;
};
