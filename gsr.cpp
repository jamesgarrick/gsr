/*
 * gsr - gradescript runner
 *
 * Author: James Garrick
 *
 * gradescript runner is a gradescript CLI tool UTK CS courses.
 * It compiles your code, runs gradescripts, and displays failures
 * side-by-side all in one step.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the MIT License.
 */

#include <vector>
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>
#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <unordered_map>
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
#define MAX_DIFF_LINES 9999
#define COMPILE_OPTS "g++ -Wall -Wextra -std=c++11 -o"
#define AUTO_UPDATE_CHECK_INTERVAL_HOURS 24

#ifndef VERSION
#define VERSION "unknown"
#endif
#ifndef AUTHOR
#define AUTHOR "unknown"
#endif

using namespace std;

namespace {
const unordered_map<string, vector<string>> class_data{
    // initial file, executable,gradescript path
    // cs302
    {"302/1",
     {"./lib_info.cpp", "./lib_info", "~jplank/cs302/Labs/Lab1/"}},
    {"302/2",
     {"make", "", "~jplank/cs302/Labs/Lab2/"}},
    // cs202
    {"202/0",
     {"make", "", "~jplank/cs202/Labs/Lab0/"}},
    {"202/1",
     {"make", "", "~jplank/cs202/Labs/Lab1/"}},
    {"202/2",
     {"make", "", "~jplank/cs202/Labs/Lab2/"}},
    {"202/3",
     {"make", "", "~jplank/cs202/Labs/Lab3/"}},
    {"202/4",
     {"make", "", "~jplank/cs202/Labs/Lab4/"}},
    {"202/5",
     {"make", "", "~jplank/cs202/Labs/Lab5/"}},
    {"202/6",
     {"make", "", "~jplank/cs202/Labs/Lab6/"}},
    {"202/7",
     {"make", "", "~jplank/cs202/Labs/Lab7/"}},
    {"202/8",
     {"make", "", "~jplank/cs202/Labs/Lab8/"}},
    {"202/9",
     {"make", "", "~jplank/cs202/Labs/Lab9/"}},
    {"202/A",
     {"make", "", "~jplank/cs202/Labs/LabA/"}},
    {"202/B",
     {"make", "", "~jplank/cs202/Labs/LabB/"}}
};
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
    regex lab_pattern(R"((?:lab)?(\d+|[ab]))", regex::icase);

    smatch match;

    for (const auto& part : cwd) {
        string component = part.string();

        // match course number
        if (info.course.empty() && std::regex_match(component, match, course_pattern)) {
            info.course = match[1];  // just number part
        }
        // lab number
        else if (info.lab.empty() && std::regex_match(component, match, lab_pattern)) {
            string lab = match[1];
            transform(lab.begin(), lab.end(), lab.begin(), ::toupper);
            info.lab = lab;
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
      cout << buffer;
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

void display_diff(ifstream &yours, ifstream &correct,
                  const string &yours_label, const string &correct_label,
                  bool all_lines, bool single_diff) {
    string y_line, c_line;
    bool diff_found = false;
    int line_number = 0;
    int width = get_terminal_width();
    int col_width = (width - 5) / 2;

    auto wrap = [col_width](const string &s) {
        vector<string> chunks;
        for (size_t i = 0; i < s.size(); i += col_width) {
            chunks.push_back(s.substr(i, col_width));
        }
        if (chunks.empty())
            chunks.push_back("");
        return chunks;
    };

    while (line_number < MAX_DIFF_LINES) {
        if (!getline(yours, y_line) || !getline(correct, c_line))
            break;

        if (y_line != c_line || all_lines) {
            if (!diff_found) {
                int l_pad = col_width - (int)yours_label.size();
                int r_pad = col_width - (int)correct_label.size();
                printf("%s%*s     %s%*s\n", yours_label.c_str(), l_pad, "",
                       correct_label.c_str(), r_pad, "");
                diff_found = true;
            }

            vector<string> left = wrap(y_line);
            vector<string> right = wrap(c_line);
            size_t max_rows = max(left.size(), right.size());

            for (size_t i = 0; i < max_rows; i++) {
                string l = (i < left.size()) ? left[i] : "";
                string r = (i < right.size()) ? right[i] : "";

                auto [l_out, r_out, l_extra, r_extra] = diff_strings(l, r);

                int l_pad = col_width - (int)l.size() - l_extra;
                int r_pad = col_width - (int)r.size() - r_extra;

                printf("%s%*s  |  %s%*s\n", l_out.c_str(), l_pad, "",
                       r_out.c_str(), r_pad, "");

                if (single_diff)
                    exit(0);
            }
        }
        line_number++;
    }
}

void self_update() {
    auto [json, status] = exec_output("curl -s https://api.github.com/repos/jamesgarrick/gsr/releases/latest");
    if (status != 0) {
        cerr << "Failed to check for updates" << endl;
        return;
    }

    // parse json for tag
    size_t pos = json.find("\"tag_name\"");
    if (pos == string::npos) {
        cerr << "No releases found" << endl;
        return;
    }
    size_t start = json.find("\"", pos + 10) + 1;
    size_t end = json.find("\"", start);
    string latest = json.substr(start, end - start);

    if (latest == VERSION) {
        cout << "Already up to date (" << VERSION << ")" << endl;
        return;
    }

    cout << "Current: " << VERSION << " -> Latest: " << latest << endl;
    cout << "Update? [y/N] ";

    char c;
    cin >> c;
    if (tolower(c) != 'y') {
        cout << "Cancelled" << endl;
        return;
    }

    string install_path = string(getenv("HOME")) + "/.local/bin/gsr";
    string temp_path = install_path + "~";
    string url = "https://github.com/jamesgarrick/gsr/releases/download/" + latest + "/gsr";

    // download new version
    cout << "Downloading..." << endl;
    string dl_cmd = "curl -sL " + url + " -o " + temp_path;
    auto [dl_out, dl_status] = exec_output(dl_cmd);
    if (dl_status != 0) {
        cerr << "Download failed" << endl;
        remove(temp_path.c_str());
        return;
    }

    // make executable
    chmod(temp_path.c_str(), 0755);

    // swap with old version
    remove(install_path.c_str());
    rename(temp_path.c_str(), install_path.c_str());

    cout << "Updated to " << latest << endl;
}

void check_for_update() {
    string cache_dir = string(getenv("HOME")) + "/.cache/gsr";
    string timestamp_file = cache_dir + "/last_update_check";

    // create cache if doesnt exist
    mkdir(cache_dir.c_str(), 0755);

    struct stat st;
    if (stat(timestamp_file.c_str(), &st) == 0) {
        time_t now = time(nullptr);
        time_t last_check = st.st_mtime;
        double hours = difftime(now, last_check) / 3600.0;

        if (hours < AUTO_UPDATE_CHECK_INTERVAL_HOURS) {
            return;
        }
    }

    ofstream(timestamp_file).close();

    // check for new release
    auto [json, status] = exec_output("curl -s --max-time 2 https://api.github.com/repos/jamesgarrick/gsr/releases/latest 2>/dev/null");
    if (status != 0) return;

    // get tag
    size_t pos = json.find("\"tag_name\"");
    if (pos == string::npos) return;
    size_t start = json.find("\"", pos + 10) + 1;
    size_t end = json.find("\"", start);
    string latest = json.substr(start, end - start);

    if (latest != VERSION && !latest.empty()) {
        cerr << "Update available: " << VERSION << " -> " << latest
             << " (run gsr -u to update)\n\n";
    }
}

int main(int argc, char *argv[]) {
  bool all_lines = false;
  bool single_diff = false;

  static struct option long_options[] = {{"all", no_argument, nullptr, 'a'},
                                         {"all", no_argument, nullptr, 'y'},
                                         {"single", no_argument, nullptr, 's'},
                                         {"help", no_argument, nullptr, 'h'},
                                         {"version", no_argument, nullptr, 'v'},
                                         {"update", no_argument, nullptr, 'u'},

                                         {nullptr, 0, nullptr, 0}};

  int opt;

  while ((opt = getopt_long(argc, argv, "aysvhu", long_options, nullptr)) !=
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
    case 'v':
      cout << VERSION << endl;
      return 0;
    case 'u':
      self_update();
      return 0;
    default:
      return 1;
    }
  }

  check_for_update();

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

  int required_args = lab_found_auto ? 1 : 2;
  if (argc - optind < required_args) {
      cout << "Usage: gsr [options] " << (lab_found_auto ? "" : "[class/lab] ") << "gradescript#\n";
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
        COMPILE_OPTS + program + " " + lab_file + " 2>&1");
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

  if (!y_out_file.is_open() || !c_out_file.is_open()) {
    cerr << "Error: could not access output files" << endl;
    return 1;
  }

  display_diff(y_out_file, c_out_file, "Your output:", "Correct output:",
               all_lines, single_diff);

  ifstream y_err_file(y_err);
  ifstream c_err_file(c_err);

  if (!y_err_file.is_open() || !c_err_file.is_open()) {
    cerr << "Error: could not access error files" << endl;
    return 1;
  }

  display_diff(y_err_file, c_err_file, "Your error:", "Correct error:",
               all_lines, single_diff);

  return 0;
}
