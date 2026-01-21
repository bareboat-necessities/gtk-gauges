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

    // Simple animated demo signal
    start_time_ = Glib::DateTime::create_now_local().to_unix();

    Glib::signal_timeout().connect(
      sigc::mem_fun(*this, &DemoWindow::on_tick),
      50 // ms
    );
  }

private:
  bool on_tick() {
    // "Boat" demo: angle swings, speed breathes, with a bit of noise.
    const double t = (Glib::DateTime::create_now_local().to_unix() - start_time_) +
                     (Glib::DateTime::create_now_local().get_microsecond() / 1e6);

    const double angle = 180.0 + 120.0 * std::sin(t * 0.35) + 25.0 * std::sin(t * 1.2);
    const double speed = 14.0 + 6.0 * std::sin(t * 0.22) + 2.0 * std::sin(t * 1.8);

    // light noise
    speed_noise_ = 0.92 * speed_noise_ + 0.08 * dist_(rng_);
    const double speed_kn = std::max(0.0, speed + speed_noise_);

    panel_.set_wind(angle, speed_kn);
    return true; // keep running
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
