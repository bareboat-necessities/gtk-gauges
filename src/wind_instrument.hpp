#pragma once

#include "circular_gauge.hpp"

// Wind angle: 0..360 (compass-style). Needle points to current angle.
// Major labels overridden to N/E/S/W + diagonals.
class WindAngleGauge final : public CircularGauge {
public:
  WindAngleGauge();

  void set_angle_deg(double deg); // wraps to [0, 360)

protected:
  double value_to_angle_rad(double v) const override;
  std::string format_value_readout(double v) const override;
};

// Wind speed in knots: uses base CircularGauge mapping.
class WindSpeedGauge final : public CircularGauge {
public:
  WindSpeedGauge();
  void set_speed_kn(double kn) { set_value(kn); }
};

// A small composite widget: two gauges + a few textual readouts.
class WindInstrumentPanel final : public Gtk::Box {
public:
  WindInstrumentPanel();

  void set_wind(double angle_deg, double speed_kn);

private:
  WindAngleGauge angle_;
  WindSpeedGauge speed_;

  Gtk::Label readout_;
};
