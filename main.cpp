#include "exclusive_output.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
struct Train { std::string name, arrival_text, departure_text; int arrival, departure, platform = 0; };

bool formula_like(const std::string& id) {
    const auto first = id.find_first_not_of(" \t\f\v\r\n");
    return first != std::string::npos && std::string("=+-@").find(id[first]) != std::string::npos;
}

int time_of_day(const std::string& text) {
    if (text.size() != 5 || text[2] != ':' || text[0] < '0' || text[0] > '9' ||
        text[1] < '0' || text[1] > '9' || text[3] < '0' || text[3] > '9' || text[4] < '0' || text[4] > '9')
        throw std::runtime_error("invalid time: " + text);
    const int hour = (text[0] - '0') * 10 + (text[1] - '0');
    const int minute = (text[3] - '0') * 10 + (text[4] - '0');
    if (hour > 23 || minute > 59) throw std::runtime_error("invalid time: " + text);
    return hour * 60 + minute;
}

std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    for (std::size_t i = 0; i <= line.size(); ++i) if (i == line.size() || line[i] == ',') {
        fields.push_back(line.substr(start, i - start)); start = i + 1;
    }
    return fields;
}

void run(const fs::path& input, const fs::path& output) {
    std::ifstream in(input);
    if (!in) throw std::runtime_error("cannot open input file");
    std::string line;
    if (!std::getline(in, line)) throw std::runtime_error("empty input file");
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "train,arrival,departure") throw std::runtime_error("expected header train,arrival,departure");
    std::vector<Train> trains;
    std::set<std::string> names;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto fields = split(line);
        if (fields.size() != 3 || fields[0].empty() || fields[0].find_first_of("\"\r\n") != std::string::npos)
            throw std::runtime_error("invalid train row");
        if (formula_like(fields[0])) throw std::runtime_error("formula-like train ID is not allowed");
        if (!names.insert(fields[0]).second) throw std::runtime_error("duplicate train: " + fields[0]);
        const int arrival = time_of_day(fields[1]);
        const int departure = time_of_day(fields[2]);
        if (departure <= arrival) throw std::runtime_error("departure must follow arrival on the same day");
        trains.push_back({fields[0], fields[1], fields[2], arrival, departure});
        if (trains.size() > 100000) throw std::runtime_error("too many trains");
    }
    if (!in.eof()) throw std::runtime_error("error reading input");
    if (trains.empty()) throw std::runtime_error("empty timetable");
    std::sort(trains.begin(), trains.end(), [](const Train& a, const Train& b) {
        return a.arrival != b.arrival ? a.arrival < b.arrival : a.name < b.name;
    });
    using Occupied = std::pair<int, int>; // departure, platform
    std::priority_queue<Occupied, std::vector<Occupied>, std::greater<Occupied>> occupied;
    std::priority_queue<int, std::vector<int>, std::greater<int>> available;
    int platform_count = 0;
    for (Train& train : trains) {
        while (!occupied.empty() && occupied.top().first <= train.arrival) {
            available.push(occupied.top().second);
            occupied.pop();
        }
        if (available.empty()) train.platform = ++platform_count;
        else { train.platform = available.top(); available.pop(); }
        occupied.push({train.departure, train.platform});
    }
    ExclusiveOutput file(output);
    std::ostream& out = file.stream();
    out << "train,platform,arrival,departure\n";
    for (const Train& train : trains)
        out << train.name << ',' << train.platform << ',' << train.arrival_text << ',' << train.departure_text << '\n';
    file.publish();
    std::cout << "platforms=" << platform_count << " trains=" << trains.size() << '\n';
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: platforms <trains.csv> <allocation.csv>");
        run(argv[1], argv[2]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
