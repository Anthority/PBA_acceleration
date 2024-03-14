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

  if (argv[2] != 0)
    path_num = std::stoi(argv[2]);
  else
    path_num = INTMAX_MAX;

  std::fstream file;
  std::filesystem::create_directory("report");

  // ------------------------------------------------------------
  auto paths = timer.report_timing(path_num);

  // timer.dump_graph(std::cout);

  std::cout << case_name << " has " << paths.size() << " paths" << std::endl;
  size_t path_gap;
  if (paths.size() < 1000)
    path_gap = 1;
  else if (paths.size() < 10000)
    path_gap = 10;
  else if (paths.size() < 100000)
    path_gap = 100;
  else
    path_gap = 1000;

  file.open("report/" + case_name + "_GBA.log", std::ios::out);
  if (file.is_open())
  {
    for (size_t i = 0; i < paths.size(); i += path_gap)
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

  // ------------------------------------------------------------
  auto start_time = std::chrono::high_resolution_clock::now();

  timer.report_timing_pba(paths);

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  std::cout << "PBA FULL Mode Execution Time: " << duration.count() << " milliseconds" << std::endl;

  file.open("report/" + case_name + "_PBA_FULL.log", std::ios::out);
  if (file.is_open())
  {
    for (size_t i = 0; i < paths.size(); i += path_gap)
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
    std::cout << "acceptable_slew:" << acceptable_slew << std::endl;

    auto start_time_2 = std::chrono::high_resolution_clock::now();

    timer.report_timing_pba_merge(paths, acceptable_slew);

    auto end_time_2 = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time_2 - start_time_2);

    std::cout << "PBA MERGE Mode Execution Time: " << duration.count() << " milliseconds" << std::endl;

    file.open("report/" + case_name + "_PBA_MERGE.log", std::ios::out);
    if (file.is_open())
    {
      for (size_t i = 0; i < paths.size(); i += path_gap)
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

  return 0;
}
