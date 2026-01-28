#include "vector"
#include <algorithm>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <unordered_map>
#include <filesystem>
#include <utility>

#include <filesystem>
#include <regex>
#include <string>
#include <iostream>

#define RED "\033[31m"
#define RED_BG "\033[41m"
#define RED_BOTH "\033[31;41m"
#define RED_HIGHLIGHT "\033[31;47m"
#define RESET "\033[0m"

using namespace std;

namespace {
const unordered_map<string, vector<string>> class_data{
    // initial file, executable,gradescript path
    {"302/1",
     {"./lib_info.cpp", "./lib_info", "~jplank/cs302/Labs/Lab1/"}},
    {"302/2",
     {"make", "bin/lab_2", "~jplank/cs302/Labs/Lab2/"}}};
}

struct LabInfo {
    std::string course;  // 302
    std::string lab;     // 1
    bool found = false;
};

LabInfo detect_lab_directory(const std::filesystem::path& cwd) {
    LabInfo info;

    // cs302, CS302, 302
    regex course_pattern(R"((?:cs|CS)?(\d{3}))", regex::icase);
    // lab1, Lab1, 1
    regex lab_pattern(R"((?:lab)?(\d+))", regex::icase);

    smatch match;

    for (const auto& part : cwd) {
        string component = part.string();

        // match course number
        if (info.course.empty() && std::regex_match(component, match, course_pattern)) {
            info.course = match[1];  // just number part
        }
        // lab number
        else if (info.lab.empty() && std::regex_match(component, match, lab_pattern)) {
            info.lab = match[1];
        }
    }

    info.found = !info.course.empty() && !info.lab.empty();
    return info;
}

int get_terminal_width() {
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
    return w.ws_col;
  }
  return 80; // default width
}

pair<string, int> exec_output(const string &cmd, bool printout = false) {
  string result;
  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe)
    return {"", -1};

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe)) {
    result += buffer;
    if (printout) {
      cout << result;
    }
  }
  int status = pclose(pipe);
  int exit_code = WEXITSTATUS(status);
  return {result, exit_code};
}

auto diff_strings = [](const std::string &a, const std::string &b) {
  size_t i = 0, j = 0;
  std::string a_out, b_out;
  int a_extra = 0, b_extra = 0;

  while (i < a.size() || j < b.size()) {
    if (i < a.size() && j < b.size() && a[i] == b[j]) {
      a_out += a[i++];
      b_out += b[j++];
      continue;
    }

    bool found = false;
    for (int look = 1; look < 20 && !found; look++) {
      // a has extra chars (missing from b)
      if (i + look < a.size() && j < b.size() && a[i + look] == b[j]) {
        a_out += RED_BG;
        for (int k = 0; k < look; k++)
          a_out += a[i++];
        a_out += RESET;
        b_out += RED_BG;
        for (int k = 0; k < look; k++)
          b_out += ' ';
        b_out += RESET;
        b_extra += look;
        found = true;
      }
      // b has extra chars (missing from a)
      else if (j + look < b.size() && i < a.size() && a[i] == b[j + look]) {
        a_out += RED_BG;
        for (int k = 0; k < look; k++)
          a_out += ' ';
        a_out += RESET;
        a_extra += look;
        b_out += RED_BG;
        for (int k = 0; k < look; k++)
          b_out += b[j++];
        b_out += RESET;
        found = true;
      }
    }

    if (!found) {
      if (i < a.size() && j < b.size()) {
        a_out += RED_BG;
        a_out += a[i++];
        a_out += RESET;
        b_out += RED_BG;
        b_out += b[j++];
        b_out += RESET;
      } else if (i < a.size()) {
        a_out += RED_BG;
        a_out += a[i++];
        a_out += RESET;
        b_out += RED_BG;
        b_out += ' ';
        b_out += RESET;
        b_extra++;
      } else {
        a_out += RED_BG;
        a_out += ' ';
        a_out += RESET;
        a_extra++;
        b_out += RED_BG;
        b_out += b[j++];
        b_out += RESET;
      }
    }
  }

  return std::make_tuple(a_out, b_out, a_extra, b_extra);
};

int main(int argc, char *argv[]) {
  bool all_lines = false;
  bool single_diff = false;

  static struct option long_options[] = {{"all", no_argument, nullptr, 'a'},
                                         {"all", no_argument, nullptr, 'y'},
                                         {"single", no_argument, nullptr, 's'},
                                         {"help", no_argument, nullptr, 'h'},
                                         {nullptr, 0, nullptr, 0}};

  int opt;

  while ((opt = getopt_long(argc, argv, "ays:o:h", long_options, nullptr)) !=
         -1) {
    switch (opt) {
    case 'a':
      all_lines = true;
      break;
    case 'y':
      all_lines = true;
      break;
    case 's':
      single_diff = true;
      break;
    case 'h':
      cout << "Usage: gsr [options] [class/lab] gradescript#n\n"
           << "  -a, --all, -y     Show all lines\n"
           << "  -s, --single      Show only one diff\n";
           // gradescript# == 0 = run_all
      return 0;
    default:
      return 1;
    }
  }

  unordered_map<string, vector<string>>::const_iterator lab_file_it;
  string lab_file;

  std::filesystem::path cwd = std::filesystem::current_path();
  string path_str = cwd.string();
  string lab;
  LabInfo info = detect_lab_directory(cwd);
  bool lab_found_auto = false;

  if (info.found) {
    string key = info.course + "/" + info.lab;

    lab_file_it = class_data.find(info.course + "/" + info.lab);
    if (lab_file_it != class_data.end()) {
      lab_found_auto = true;
      lab_file = lab_file_it->second[0];
      lab = key;
    }
  }

  if (argc - optind < 2 && !lab_found_auto) {
    cout << "Usage: gsr [options] [class/lab] gradescript#\n";
    return 1;
  }
  string gradescript_num;


  if (!lab_found_auto) {
      lab = argv[optind];
      gradescript_num = argv[optind + 1];
      lab_file_it = class_data.find(lab);
  } else {
    gradescript_num = argv[optind];
  }
  if (lab_file_it != class_data.end() && !lab_found_auto) {
    lab_file = lab_file_it->second[0];
  } else if (!lab_found_auto) {
    cerr << "Lab " << lab << " not found." << endl;
    return 1;
  }
  string program = class_data.at(lab)[1];
  string gradescript = class_data.at(lab)[2] + "gradescript";
  string gradeall = class_data.at(lab)[2] + "gradeall";

  if (lab_file == "make") {
    auto [make_output, make_status] = exec_output("make 2>&1");
    if (make_status != 0) {
      cerr << "Make failed:\n" << make_status << endl;
      return 1;
    }
  } else {
    auto [compile_output, compile_status] = exec_output(
        "g++ -Wall -Wextra -std=c++11 -o " + program + " " + lab_file + " 2>&1");
    if (compile_status != 0) {
      cerr << "Compilation failed:\n" << compile_output << endl;
      return 1;
    }
  }

  if (gradescript_num == "0") {
     exec_output(gradeall, true);
     return 0;
  }

  cout << gradescript + " " + gradescript_num << endl;
  auto [gradescript_output, gradescript_status] =
      exec_output(gradescript + " " + gradescript_num);
  if (gradescript_status != 0) {
    cerr << "Gradescript compilation failed:\n" << gradescript_output << endl;
    return 1;
  }

  stringstream iss(gradescript_output);

  string line;
  getline(iss, line);

  // success condition
  if (line.find("incorrect") == string::npos) {
    cout << gradescript_output;
    return 0;
  }

  cout << "Gradescript " << gradescript_num << " failed.\n" << endl;

  string y_out, y_err, c_out, c_err;

  while (getline(iss, line)) {
    if (line.find("Your standard output is") != string::npos) {
      y_out = "./" + line.substr(line.find("tmp"), 23);
    }
    if (line.find("Your standard error") != string::npos) {
      y_err = line.substr(line.find("tmp"), 23);
    }
    if (line.find("correct standard output") != string::npos) {
      c_out = "./" + line.substr(line.find("tmp"), 26);
    }
    if (line.find("correct standard error") != string::npos) {
      c_err = line.substr(line.find("tmp"), 26);
    }
  }

  ifstream y_out_file(y_out);
  ifstream c_out_file(c_out);

  // cout << y_out << endl;
  // cout << y_err << endl;

  bool end_of_out_file = false;
  bool out_diff_found = false;
  string y_out_line, c_out_line;
  int out_line_number = 0;

  if (!y_out_file.is_open() || !c_out_file.is_open()) {
    cerr << "Error: could not access output files" << endl;
    return 1;
  }

  while (end_of_out_file == false && out_line_number < 9999) {
    if (!getline(y_out_file, y_out_line) || !getline(c_out_file, c_out_line)) {
      end_of_out_file = true;
      break;
    }

    if (y_out_line != c_out_line || all_lines) {
      int width = get_terminal_width();
      int col_width = (width - 5) / 2;

      if (!out_diff_found) {
        string l_text = "Your output:";
        string r_text = "Correct output:";

        int t_l_pad = col_width - (int)l_text.size();
        int t_r_pad = col_width - (int)r_text.size();

        printf("%s%*s     %s%*s\n", l_text.c_str(), t_l_pad, "", r_text.c_str(),
               t_r_pad, "");

        out_diff_found = true;
      }

      auto wrap = [col_width](const string &s) {
        vector<string> chunks;
        for (size_t i = 0; i < s.size(); i += col_width) {
          chunks.push_back(s.substr(i, col_width));
        }
        if (chunks.empty())
          chunks.push_back("");
        return chunks;
      };

      vector<string> left = wrap(y_out_line);
      vector<string> right = wrap(c_out_line);
      size_t max_rows = max(left.size(), right.size());

      for (size_t i = 0; i < max_rows; i++) {
        std::string l = (i < left.size()) ? left[i] : "";
        std::string r = (i < right.size()) ? right[i] : "";

        auto [l_out, r_out, l_extra, r_extra] = diff_strings(l, r);

        int l_pad = col_width - (int)l.size() - l_extra;
        int r_pad = col_width - (int)r.size() - r_extra;

        printf("%s%*s  |  %s%*s\n", l_out.c_str(), l_pad, "", r_out.c_str(),
               r_pad, "");
        // if (!all_lines) printf("\n");
        if (single_diff)
          exit(0);
      }
    }

    out_line_number++;
  }

  ifstream y_err_file(y_err);
  ifstream c_err_file(c_err);

  bool end_of_err_file = false;
  bool err_diff_found = false;
  string y_err_line, c_err_line;
  int err_line_number = 0;

  if (!y_err_file.is_open() || !c_err_file.is_open()) {
    cerr << "Error: could not access error files" << endl;
    return 1;
  }

  while (end_of_err_file == false && err_line_number < 9999) {
    if (!getline(y_err_file, y_err_line) || !getline(c_err_file, c_err_line)) {
      end_of_err_file = true;
      break;
    }

    if (y_err_line != c_err_line) {
      int width = get_terminal_width();
      int col_width = (width - 5) / 2;

      if (!err_diff_found) {
        string l_text = "Your error:";
        string r_text = "Correct error:";

        int t_l_pad = col_width - (int)l_text.size();
        int t_r_pad = col_width - (int)r_text.size();

        printf("%s%*s     %s%*s\n", l_text.c_str(), t_l_pad, "", r_text.c_str(),
               t_r_pad, "");

        err_diff_found = true;
      }

      auto wrap = [col_width](const string &s) {
        vector<string> chunks;
        for (size_t i = 0; i < s.size(); i += col_width) {
          chunks.push_back(s.substr(i, col_width));
        }
        if (chunks.empty())
          chunks.push_back("");
        return chunks;
      };

      vector<string> left = wrap(y_err_line);
      vector<string> right = wrap(c_err_line);
      size_t max_rows = max(left.size(), right.size());

      for (size_t i = 0; i < max_rows; i++) {
        std::string l = (i < left.size()) ? left[i] : "";
        std::string r = (i < right.size()) ? right[i] : "";

        auto [l_err, r_err, l_extra, r_extra] = diff_strings(l, r);

        int l_pad = col_width - (int)l.size() - l_extra;
        int r_pad = col_width - (int)r.size() - r_extra;

        printf("%s%*s  |  %s%*s\n", l_err.c_str(), l_pad, "", r_err.c_str(),
               r_pad, "");
        if (single_diff)
          exit(0);
      }
    }

    err_line_number++;
  }

  return 0;
}
