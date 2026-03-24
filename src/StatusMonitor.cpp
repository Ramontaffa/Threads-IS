#include "StatusMonitor.h"
#include <iomanip>
#include <sstream>
#include <cmath>

std::string formatTime(size_t frames, uint32_t sampleRate) {
    if (sampleRate == 0) return "0:00";
    double seconds = static_cast<double>(frames) / sampleRate;
    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    std::ostringstream oss;
    oss << mins << ":" << std::setfill('0') << std::setw(2) << secs;
    return oss.str();
}

std::string renderProgressBar(size_t current, size_t total, int width) {
    if (total == 0) {
        return std::string(width, ' ');
    }
    
    double progress = static_cast<double>(current) / total;
    int filled = static_cast<int>(progress * width);
    
    std::string bar(filled, '=');
    bar += std::string(width - filled, '-');
    return bar;
}
