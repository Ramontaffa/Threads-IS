#ifndef STATUS_MONITOR_H
#define STATUS_MONITOR_H

#include "AudioState.h"
#include <string>

// Funções auxiliares para formatação de status
std::string formatTime(size_t frames, uint32_t sampleRate);
std::string renderProgressBar(size_t current, size_t total, int width = 20);

#endif // STATUS_MONITOR_H
