#include "wind_instrument.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

static double sign0(double x) { return (x < 0) ? -1.0 : 1.0; }

double WindAngleGauge::clamp_180(double deg) {
  // Hard clamp for this instrument (user requested -180..+180).
  return std::clamp(deg, -180.0, 180.0);
}

WindAngleGauge::WindAngleGauge() {
  set_title("APP WIND");
  set_unit("AWA");

  set_range(-180.0, 180.0);

  // We want a full 360° dial, with:
  //  0° at top (bow), +90° right, -90° left, ±180° at bottom (stern).
  // Cairo angle: 0 at +x, +90 at +y, so "north/top" is -90.
  // Map: angle = -90 + value_deg.
  //
  // Note: -180 and +180 point to same direction (stern). That's fine for needle.
  style().start_deg = -90.0 - 180.0;
  style().end_deg   = -90.0 + 180.0;

  // Majors: -180, -135, -90, -45, 0, 45, 90, 135, 180
  style().major_ticks = 9;
  style().minor_ticks = 2;
  style().value_precision = 0;
}

void WindAngleGauge::set_angle_deg(double deg) {
  set_value(clamp_180(deg));
}

double WindAngleGauge::value_to_angle_rad(double v) const {
  const double ang_deg = -90.0 + clamp_180(v);
  return deg_to_rad(ang_deg);
}

std::string WindAngleGauge::format_major_label(int /*major_index*/, double major_value) const {
  // Show signed angles. At ±180 show "180".
  int v = (int)std::lround(clamp_180(major_value));
  if (std::abs(v) == 180) v = 180;

  // Compact: no plus sign, keep minus for port.
  return std::to_string(v);
}

std::string WindAngleGauge::format_value_readout(double v) const {
  // Show "AWA -23°" style readout, but compact.
  const int d = (int)std::lround(clamp_180(v));
  std::ostringstream ss;
  ss << d << "°";
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

  // Default theme
  apply_theme(SailTheme{});
}

void WindInstrumentPanel::apply_theme(const SailTheme& t) {
  theme_ = t;

  // Apply gauge theme
  angle_.apply_theme(theme_.gauge);
  speed_.apply_theme(theme_.gauge);

  // Add sailing-style zones to AWA:
  // Red "no-go" near bow: +/- 35°
  // Green close-hauled window: 35..60° both sides
  // (You can tweak these numbers to match your boat/polar preferences.)
  std::vector<CircularGauge::Zone> zones;
  zones.push_back({-35.0,  35.0, theme_.accent_red,   0.90});
  zones.push_back({-60.0, -35.0, theme_.accent_green, 0.85});
  zones.push_back({ 35.0,  60.0, theme_.accent_green, 0.85});
  angle_.set_zones(std::move(zones));

  // Optional: speed gauge could also get zones if you want (not requested).
  speed_.set_zones({});

  // Panel background via CSS
  auto css = Gtk::CssProvider::create();
  const auto bg = theme_.panel_bg.to_string(); // rgba(...)
  css->load_from_data(
    "window, box { background-color: " + bg + "; }"
  );
  Gtk::StyleContext::add_provider_for_display(
    Gdk::Display::get_default(),
    css,
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );
}

void WindInstrumentPanel::set_wind(double awa_deg, double aws_kn) {
  angle_.set_angle_deg(awa_deg);
  speed_.set_speed_kn(aws_kn);

  std::ostringstream ss;
  ss << std::fixed << std::setprecision(1)
     << "AWA " << (int)std::lround(std::clamp(awa_deg, -180.0, 180.0)) << "°"
     << "   |   AWS " << aws_kn << " kn";

  readout_.set_text(ss.str());
}
