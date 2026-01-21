#include "wind_instrument.hpp"
#include <gtkmm.h>
#include <cmath>
#include <random>

class DemoWindow final : public Gtk::Window {
public:
  DemoWindow() {
    set_title("Wind Instrument Demo (gtkmm)");
    set_default_size(720, 380);
    set_child(panel_);

    // Apply a slightly more "LVGL default dark" feel
    SailTheme t;
    t.panel_bg = Gdk::RGBA("#0b0e12");
    t.gauge.style.face = Gdk::RGBA("#10151c");
    t.gauge.style.ring = Gdk::RGBA("#27313b");
    t.gauge.style.tick = Gdk::RGBA("#d7dee8");
    t.gauge.style.text = Gdk::RGBA("#eef4ff");
    t.gauge.style.subtext = Gdk::RGBA("#9fb0c3");
    t.gauge.style.needle = Gdk::RGBA("#ff453a");
    t.gauge.style.hub = Gdk::RGBA("#eef4ff");
    t.gauge.style.font_family = "Sans";
    panel_.apply_theme(t);

    start_time_ = Glib::DateTime::create_now_local().to_unix();
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &DemoWindow::on_tick), 50);
  }

private:
  bool on_tick() {
    const auto now = Glib::DateTime::create_now_local();
    const double t = (now.to_unix() - start_time_) + (now.get_microsecond() / 1e6);

    // AWA in [-180,180]
    const double awa =  75.0 * std::sin(t * 0.35) + 25.0 * std::sin(t * 1.2);

    // AWS with some noise
    const double base = 14.0 + 6.0 * std::sin(t * 0.22) + 2.0 * std::sin(t * 1.8);
    speed_noise_ = 0.92 * speed_noise_ + 0.08 * dist_(rng_);
    const double aws = std::max(0.0, base + speed_noise_);

    panel_.set_wind(awa, aws);
    return true;
  }

  WindInstrumentPanel panel_;
  double start_time_ = 0.0;

  std::mt19937 rng_{12345};
  std::normal_distribution<double> dist_{0.0, 0.6};
  double speed_noise_ = 0.0;
};

int main(int argc, char** argv) {
  auto app = Gtk::Application::create("com.example.gtk.gauges.winddemo");
  return app->make_window_and_run<DemoWindow>(argc, argv);
}
