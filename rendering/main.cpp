#include <curses.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>

#include <algorithm>
#include <cstdint>
#include <thread>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <queue>

using XType = int64_t;
using YType = float;

constexpr size_t NumberOfChannels = 3;
constexpr size_t NumberOfSamplesPerSecond = 500;
constexpr size_t NumberOfSamples = NumberOfSamplesPerSecond * 60;
constexpr size_t SamplesPerMinute = 60 * NumberOfSamplesPerSecond;

using Channels = std::array<float, NumberOfChannels>;

class Sample {
private:
    uint64_t ts_;
    Channels values_;

public:
    Sample(uint64_t ts, Channels values) : ts_(ts), values_(values) {
    }

public:
    uint64_t ts() {
        return ts_;
    }

    float operator[](int32_t index) const {
        return values_[index];
    }

};

class Samples {
private:
    std::queue<Sample> samples_;

public:
    std::queue<Sample>& samples() {
        return samples_;
    }

    void clear() {
        while (!samples_.empty()) {
                samples_.pop();
        }
    }

    size_t size() {
        return samples_.size();
    }

public:
    std::vector<YType> advance(uint32_t samples) {
        std::vector<YType> values;

        if (samples_.size() >= samples) {
            for (auto i = 0; i < (int32_t)samples; ++i) {
                if (samples_.size()) {
                    auto s = samples_.front();
                    samples_.pop();
                    values.emplace_back(s[0]);
                }
                else {
                    break;
                }
            }
        }

        return values;
    }

};

class IncomingGeophoneData {
private:
    std::string path_;
    std::ifstream ifs;
    size_t lastSize_{ 0 };
    size_t position_{ 0 };

public:
    IncomingGeophoneData(std::string path) : path_(path), ifs(path, std::ios::binary) {
    }

public:
    bool refresh(Samples &samples, size_t numberOfSamples) {
        auto sizeNow = ifs.is_open() ? fileSize(path_) : 0;
        auto reopen = sizeNow < lastSize_;

        if (!ifs.is_open() || reopen) {
            if (false) {
                std::cerr << "Reopening " << lastSize_ << " " << sizeNow << std::endl;
            }
            ifs.open(path_, std::ios::binary);
            if (!ifs.is_open()) {
                if (false) {
                    std::cerr << "Unable to open file" << std::endl;
                }
                return false;
            }
            position_ = 0;
        }

        if (ifs.is_open()) {
            ifs.seekg(position_);

            while (true) {
                float data[NumberOfChannels][numberOfSamples];
                ifs.read(reinterpret_cast<char*>(data), sizeof(data));
                if (!ifs.eof()) {
                    if (ifs.fail()) {
                        ifs.close();
                        break;
                    }

                    for (auto i = 0; i < (int32_t)numberOfSamples; ++i) {
                        samples.samples().emplace(0, Channels{ data[0][i], data[1][i], data[2][i] });
                    }

                    if (false) {
                        std::cerr << "Read " << ifs.tellg() << " " << numberOfSamples << std::endl;
                    }
                    position_ = ifs.tellg();
                }
                else {
                    ifs.clear();
                    break;
                }
            }
        }

        lastSize_ = sizeNow;

        return true;
    }

    size_t fileSize(std::string path) {
        struct stat info;
        stat(path.c_str(), &info);
        return info.st_size;
    }
};

class GeophoneFile {
private:
    std::string path_;

public:
    GeophoneFile(std::string path) : path_(path) {
    }

public:
    bool read(Samples &samples) {
        std::tm t = {};
        if (!parseTime(filenameOf(path_), t)) {
            return false;
        }

        auto starting = mktime(&t);
        auto tick = ((int64_t)starting * 1000);

        std::ifstream ifs(path_, std::ios::binary);
        float data[NumberOfChannels][NumberOfSamples];
        ifs.read(reinterpret_cast<char*>(data), sizeof(data));
        if (ifs.fail()) {
            return false;
        }

        for (auto i = 0; i < (int32_t)NumberOfSamples; ++i) {
            samples.samples().emplace(tick, Channels{ data[0][i], data[1][i], data[2][i] });
            tick += 1000 / NumberOfSamples;
        }

        return true;
    }

private:
    bool parseTime(std::string s, std::tm &t) {
        auto layout = std::string { "geophones_20180423_233335.bin" };
        if (s.size() != layout.size()) {
            return false;
        }
        auto timeOnly = s.substr(10, 15);
        auto r = strptime(timeOnly.c_str(), "%Y%m%d_%H%M%S", &t);
        return r != nullptr && r[0] == 0;
    }

    std::string filenameOf(std::string path) {
        auto copy = strdup(path.c_str());
        auto name = basename(copy);
        free(copy);
        return name;
    }
};

class DataReader {
private:
    std::vector<std::string> seen_;

public:
    void refresh(std::string path, Samples &samples, bool load) {
        std::vector<std::string> files;

        scan(path, files);

        for (auto& p : files) {
            auto f = std::find(std::begin(seen_), std::end(seen_), p);
            if (f == std::end(seen_)) {
                if (load) {
                    GeophoneFile file{ p };
                    file.read(samples);
                }
                seen_.push_back(p);
            }
        }
    }

private:
    void scan(std::string path, std::vector<std::string> &files) {
        auto dir = opendir(path.c_str());
        if (dir == nullptr) {
            return;
        }

        while (true) {
            auto entry = readdir(dir);
            if (entry == nullptr) {
                break;
            }

            if (entry->d_name[0] != '.') {
                auto relative = path + '/' + entry->d_name;
                if (isFile(relative)) {
                    files.emplace_back(relative);
                }
                else {
                    scan(relative, files);
                }
            }
        }
    }

    bool isFile(std::string path) {
        struct stat info;
        stat(path.c_str(), &info);
        return S_ISREG(info.st_mode);
    }

};

template<typename T>
T scale(T value, T omin, T omax, T nmin, T nmax) {
    auto x = (value - omin) / (omax - omin);
    return x * (nmax - nmin) + nmin;
}

class Viewport {
private:
    WINDOW *window_;
    int32_t ym_;
    int32_t xm_;

public:
    Viewport(WINDOW *window) : window_(window) {
        update();
    }

public:
    void erase() {
        werase(window_);
    }

    int32_t height() {
        return ym_;
    }

    int32_t width() {
        return xm_;
    }

    void drawGraph(std::vector<YType> &values) {
        update();

        auto range = getRange(values);
        auto time = 0;
        auto halfY = ym_ / 2;
        auto timeStep = (values.size() - 0) / (width());;

        move(0, 0);
        for (auto screenX = 0; screenX < xm_; screenX++) {
            auto y = values[time];
            auto screenY = (int32_t)(scale<YType>(y, range.first, range.second, halfY, -halfY));

            mvwaddch(window_, screenY + halfY, screenX, '#');
            column(screenX, halfY, screenY + halfY);

            time += timeStep;
        }

        move(height() - 1, 0);
        attron(COLOR_PAIR(1));
        printw("[%f, %f] %d %d", range.first, range.second, values.size(), timeStep);
        attroff(COLOR_PAIR(1));
    }

private:
    void update() {
        getmaxyx(window_, ym_, xm_);
    }

    std::pair<YType, YType> getRange(std::vector<YType> &values) {
        auto min = std::min_element(std::begin(values), std::end(values));
        auto max = std::max_element(std::begin(values), std::end(values));
        return { *min, *max };
    }

    void column(int32_t x, int32_t start, int32_t end) {
        if (end < start) {
            auto temp = start;
            start = end;
            end = temp;
        }

        for (auto i = start; i <= end; ++i) {
            mvwaddch(window_, i, x, '#');
        }
    }

};

int32_t main(int32_t argc, const char **argv) {
    Samples samples;
    DataReader reader;

    auto onlyIncoming = true;
    auto onlyNew = false;
    auto rate = SamplesPerMinute;
    std::string path = "./";

    for (auto i = 0; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--only-new") {
            onlyNew = true;
        }

        if (arg == "--directory") {
            if (i + 1 < argc) {
                path = argv[++i];
            }
        }
    }

    IncomingGeophoneData incoming{ "incoming.bin" };
    if (onlyIncoming) {
        incoming.refresh(samples, rate);
        samples.clear();
    }
    else {
        reader.refresh(path, samples, !onlyNew);
    }

    std::cerr << "Ready, starting viewer." << std::endl;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);

    Viewport viewport{ stdscr };
    rate = viewport.width();
    std::vector<YType> visible;

    while (true) {
        viewport.erase();

        auto values = samples.advance(rate);
        if (values.size() > 0) {
            visible = values;
        }

        if (visible.size() > 0) {
            viewport.drawGraph(visible);
        }

        move(viewport.height() - 2, 0);
        attron(COLOR_PAIR(1));
        printw("%d samples", samples.size());
        attroff(COLOR_PAIR(1));

        refresh();

        if (onlyIncoming) {
            incoming.refresh(samples, rate);
        }
        else {
            reader.refresh(path, samples, true);
        }

        usleep((1000000 / 1000) * 400);
    }

    endwin();
}
