#include "wind_instrument.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

static double wrap_360(double deg) {
  double x = std::fmod(deg, 360.0);
  if (x < 0) x += 360.0;
  return x;
}

WindAngleGauge::WindAngleGauge() {
  set_title("APP WIND");
  set_unit("deg");

  set_range(0.0, 360.0);

  // Full circle mapping, not an arc sweep.
  style().start_deg = -90.0; // 0° at North (up). We'll map value accordingly.
  style().end_deg   = 270.0;

  // 8 compass points: N, NE, E, SE, S, SW, W, NW, N
  style().major_ticks = 9;
  style().minor_ticks = 1;
  style().value_precision = 0;

  set_major_labels({"N","NE","E","SE","S","SW","W","NW","N"});
}

void WindAngleGauge::set_angle_deg(double deg) {
  set_value(wrap_360(deg));
}

double WindAngleGauge::value_to_angle_rad(double v) const {
  // v is degrees, 0 at North, increasing clockwise.
  // Screen coords in Cairo: +x right, +y down, angle 0 along +x and increases clockwise? (Cairo uses radians with y down => positive angles go clockwise.)
  // We want 0° at North (up) => -90° in Cairo.
  const double ang_deg = -90.0 + v;
  return deg_to_rad(ang_deg);
}

std::string WindAngleGauge::format_value_readout(double v) const {
  // show "123°"
  std::ostringstream ss;
  ss << (int)std::lround(wrap_360(v)) << "°";
  return ss.str();
}

WindSpeedGauge::WindSpeedGauge() {
  set_title("WIND SPD");
  set_unit("kn");
  set_range(0.0, 40.0);

  style().start_deg = -225.0;
  style().end_deg   =  45.0;
  style().major_ticks = 9; // 0..40 step 5
  style().minor_ticks = 4;
  style().value_precision = 1;
}

WindInstrumentPanel::WindInstrumentPanel()
: Gtk::Box(Gtk::Orientation::VERTICAL) {
  set_spacing(12);
  set_margin(16);

  auto row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
  row->set_spacing(12);

  // Make them expand nicely
  angle_.set_hexpand(true);
  angle_.set_vexpand(true);
  speed_.set_hexpand(true);
  speed_.set_vexpand(true);

  row->append(angle_);
  row->append(speed_);

  readout_.set_xalign(0.5f);
  readout_.set_margin_top(6);
  readout_.set_margin_bottom(2);
  readout_.set_markup("<span size='large' weight='bold'>--</span>");

  append(*row);
  append(readout_);
}

void WindInstrumentPanel::set_wind(double angle_deg, double speed_kn) {
  angle_.set_angle_deg(angle_deg);
  speed_.set_speed_kn(speed_kn);

  std::ostringstream ss;
  ss << std::fixed << std::setprecision(1)
     << "AWA " << (int)std::lround(wrap_360(angle_deg)) << "°"
     << "   |   AWS " << speed_kn << " kn";

  readout_.set_text(ss.str());
}
