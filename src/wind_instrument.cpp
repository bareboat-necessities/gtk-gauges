#include "wind_instrument.hpp"

#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdio>

double WindAngleGauge::clamp_180(double deg) {
  return std::clamp(deg, -180.0, 180.0);
}

WindAngleGauge::WindAngleGauge() {
  set_title("APP WIND");
  set_unit("AWA");

  set_range(-180.0, 180.0);

  // Full 360° dial:
  // 0° at top (bow), +90° right, -90° left, ±180° bottom (stern).
  style().start_deg = -90.0 - 180.0; // -270
  style().end_deg   = -90.0 + 180.0; // +90

  // Major every 30° across -180..+180 => 13 majors.
  // Minor every 10° => 2 minors between majors.
  style().major_ticks = 13;
  style().minor_ticks = 2;

  // Move readout down so it doesn't get covered by the needle.
  style().value_radius_frac = 0.28;

  style().value_precision = 0;
}

void WindAngleGauge::set_angle_deg(double deg) {
  set_value(clamp_180(deg));
}

double WindAngleGauge::value_to_angle_rad(double v) const {
  // Direct wind mapping: angle = -90° + AWA (so 0 is up)
  const double ang_deg = -90.0 + clamp_180(v);
  return deg_to_rad(ang_deg);
}

std::string WindAngleGauge::format_major_label(int /*major_index*/, double major_value) const {
  int v = static_cast<int>(std::lround(clamp_180(major_value)));
  if (std::abs(v) == 180) v = 180;
  return std::to_string(v);
}

std::string WindAngleGauge::format_value_readout(double /*v*/) const {
  // Show speed (kn) on the wind angle gauge readout.
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.1f kn", speed_kn_);
  return std::string(buf);
}

WindSpeedGauge::WindSpeedGauge() {
  set_title("WIND SPD");
  set_unit("kn");

  set_range(0.0, 40.0);

  style().start_deg = -225.0;
  style().end_deg   =  45.0;
  style().major_ticks = 9;   // 0..40 step 5
  style().minor_ticks = 4;
  style().value_precision = 1;

  style().value_radius_frac = 0.22;
}

WindInstrumentPanel::WindInstrumentPanel()
: Gtk::Box(Gtk::Orientation::VERTICAL) {
  set_spacing(12);
  set_margin(16);

  auto row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
  row->set_spacing(12);

  angle_.set_hexpand(true);
  angle_.set_vexpand(true);
  speed_.set_hexpand(true);
  speed_.set_vexpand(true);

  row->append(angle_);
  row->append(speed_);

  readout_.set_xalign(0.5f);
  readout_.set_margin_top(6);
  readout_.set_margin_bottom(2);

  append(*row);
  append(readout_);

  apply_theme(SailTheme{});
}

void WindInstrumentPanel::apply_theme(const SailTheme& t) {
  theme_ = t;

  angle_.apply_theme(theme_.gauge);
  speed_.apply_theme(theme_.gauge);

  // Zones per request:
  // - no-go: -20..+20 (NOT red; use amber/orange)
  // - port scale: -60..-20 (red)
  // - stbd scale: +20..+60 (green)
  std::vector<CircularGauge::Zone> zones;
  //zones.push_back({ -20.0,   20.0, theme_.accent_no_go, 0.90 }); // no-go (caution)
  zones.push_back({ -60.0,  -20.0, theme_.accent_red,   0.75 }); // port scale (red)
  zones.push_back({  20.0,   60.0, theme_.accent_green, 0.85 }); // stbd scale (green)
  zones.push_back({-160.0, -120.0, theme_.accent_red,   0.75 }); // port scale (red)
  zones.push_back({ 120.0,  160.0, theme_.accent_green, 0.85 }); // stbd scale (green)

  angle_.set_zones(std::move(zones));
  speed_.set_zones({});

  // Panel background via CSS
  auto css = Gtk::CssProvider::create();
  const auto bg = theme_.panel_bg.to_string(); // rgba(...)
  css->load_from_data("window, box { background-color: " + bg + "; }");
  Gtk::StyleContext::add_provider_for_display(
      Gdk::Display::get_default(),
      css,
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );
}

void WindInstrumentPanel::set_wind(double awa_deg, double aws_kn) {
  angle_.set_angle_deg(awa_deg);
  angle_.set_speed_kn(aws_kn);  // readout on AWA gauge is AWS
  speed_.set_speed_kn(aws_kn);

  std::ostringstream ss;
  ss << "AWA " << static_cast<int>(std::lround(std::clamp(awa_deg, -180.0, 180.0))) << "°"
     << "   |   AWS " << std::fixed << std::setprecision(1) << aws_kn << " kn";
  readout_.set_text(ss.str());
}
