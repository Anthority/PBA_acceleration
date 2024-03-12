// This program demonstrates how to use OpenTimer's API to
// run a simple timing-driven optimization loop plus
// incremental timing update.

// Design : vga_lcd.v
// SDC    : vga_lcd.sdc
// SPEF   : vga_lcd.spef change_1.spef
// Library: vga_lcd_Early.lib (early/ min split)
// Library: vga_lcd_Late.lib  (late / max split)

#include <ot/timer/timer.hpp>

int main(int argc, char *argv[])
{

  ot::Timer timer = ot::Timer();

  // Read design
  timer.read_celllib("vga_lcd_Early.lib", ot::MIN)
      .read_celllib("vga_lcd_Late.lib", ot::MAX)
      .read_verilog("vga_lcd.v")
      .read_spef("vga_lcd.spef")
      .read_sdc("vga_lcd.sdc");

  // auto paths = timer.report_timing(1000);
  auto paths = timer.my_report_timing(1000);

  for (size_t i = 0; i < paths.size(); ++i)
  {
    std::cout << "----- Critical Path GBA Mode " << i << " -----\n";
    std::cout << *paths[i] << '\n';
  }

  timer.report_timing_pba(paths);
  // timer.new_report_timing_pba_merge(paths);

  for (size_t i = 0; i < paths.size(); ++i)
  {
    std::cout << "----- Critical Path PBA Mode " << i << " -----\n";
    std::cout << *paths[i] << '\n';
  }

  return 0;
}
