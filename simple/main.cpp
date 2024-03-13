#include <ot/timer/timer.hpp>
#include <filesystem>

int main(int argc, char *argv[])
{

  ot::Timer timer = ot::Timer();

  std::string case_name = argv[1];

  std::string lib_early = case_name + "_Early.lib";
  std::string lib_late = case_name + "_Late.lib";
  std::string verilog_file = case_name + ".v";
  std::string spef_file = case_name + ".spef";
  std::string sdc_file = case_name + ".sdc";

  // Read design
  timer.read_celllib(lib_early, ot::MIN)
      .read_celllib(lib_late, ot::MAX)
      .read_verilog(verilog_file)
      .read_spef(spef_file)
      .read_sdc(sdc_file);

  size_t path_num;

  if (argv[2])
    path_num = std::stoi(argv[2]);
  else
    path_num = 100;

  std::fstream file;
  std::filesystem::create_directory("report");

  // ------------------------------------------------------------
  auto paths = timer.report_timing(path_num);
  file.open("report/" + case_name + "_GBA.log", std::ios::out);
  if (file.is_open())
  {
    for (size_t i = 0; i < paths.size(); ++i)
    {
      file << "----- Critical Path GBA Mode " << i << " -----\n";
      file << paths[i] << '\n';
    }
    file.close();
  }
  else
  {
    std::cerr << "Failed to open file for writing." << std::endl;
  }
  // for (size_t i = 0; i < paths.size(); ++i)
  // {
  //   std::cout << "----- Critical Path GBA Mode " << i << " -----\n";
  //   std::cout << paths[i] << '\n';
  // }

  // ------------------------------------------------------------
  timer.report_timing_pba(paths);
  file.open("report/" + case_name + "_PBA_FULL.log", std::ios::out);
  if (file.is_open())
  {
    for (size_t i = 0; i < paths.size(); ++i)
    {
      file << "----- Critical Path PBA FULL Mode " << i << " -----\n";
      file << paths[i] << '\n';
    }
    file.close();
  }
  else
  {
    std::cerr << "Failed to open file for writing." << std::endl;
  }

  // ------------------------------------------------------------
  float acceptable_slew;
  if (argv[3])
  {
    acceptable_slew = std::stof(argv[3]);
    timer.report_timing_pba_merge(paths, acceptable_slew);
    file.open("report/" + case_name + "_PBA_MERGE.log", std::ios::out);
    if (file.is_open())
    {
      for (size_t i = 0; i < paths.size(); ++i)
      {
        file << "----- Critical Path PBA MERGE Mode " << i << " -----\n";
        file << paths[i] << '\n';
      }
      file.close();
    }
    else
    {
      std::cerr << "Failed to open file for writing." << std::endl;
    }
  }
  // for (size_t i = 0; i < paths.size(); ++i)
  // {
  //   std::cout << "----- Critical Path PBA Mode " << i << " -----\n";
  //   std::cout << paths[i] << '\n';
  // }

  return 0;
}
