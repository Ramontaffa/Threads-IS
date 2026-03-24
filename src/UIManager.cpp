#include "UIManager.h"
#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <conio.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {

#ifndef _WIN32
class TerminalRawModeGuard {
public:
    TerminalRawModeGuard() {
        tcgetattr(STDIN_FILENO, &originalTermios_);
        termios raw = originalTermios_;
        raw.c_lflag &= static_cast<unsigned int>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        originalFlags_ = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, originalFlags_ | O_NONBLOCK);
    }

    ~TerminalRawModeGuard() {
        tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios_);
        fcntl(STDIN_FILENO, F_SETFL, originalFlags_);
    }

private:
    termios originalTermios_{};
    int originalFlags_{0};
};
#endif

void clearScreen() {
    std::cout << "\x1B[2J\x1B[H";
}

int readKeyNonBlocking() {
#ifdef _WIN32
    if (_kbhit() == 0) {
        return -1;
    }
    return _getch();
#else
    unsigned char ch = 0;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        return static_cast<int>(ch);
    }
    return -1;
#endif
}

void printDashboard(AudioState* state) {
    clearScreen();

    std::cout << "=====================================\n";
    std::cout << "          DJ Mixer (Teclado)         \n";
    std::cout << "=====================================\n\n";

    const bool isGlobalPlay = state->globalPlay.load(std::memory_order_relaxed);
    std::cout << "Master: " << (isGlobalPlay ? "TOCANDO" : "PAUSADO") << "\n";
    std::cout << "Frame atual: "
              << state->currentFrame.load(std::memory_order_relaxed)
              << " / " << state->totalFrames << "\n";
    std::cout << "Assets: " << state->assetName << "\n\n";

    std::cout << "Faixas:\n";
    for (size_t i = 0; i < state->tracks.size(); ++i) {
        const bool trackPlaying = state->tracks[i].isPlaying.load(std::memory_order_relaxed);
        std::cout << "  [" << (i + 1) << "] " << state->tracks[i].trackName
                  << " : " << (trackPlaying ? "ON" : "OFF") << "\n";
    }

    std::cout << "\nControles:\n";
    std::cout << "  1-8 : Ativa/desativa faixa\n";
    std::cout << "  ESPACO: Play/Pause global\n";
    std::cout << "  r : Reinicia música (frame 0)\n";
    std::cout << "  q : Encerrar\n";
    std::cout.flush();
}

} // namespace

void uiLoop(AudioState* state) {
#ifndef _WIN32
    TerminalRawModeGuard rawModeGuard;
#endif

    printDashboard(state);

    while (state->programRunning.load(std::memory_order_relaxed)) {
        const int key = readKeyNonBlocking();

        if (key == -1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            continue;
        }

        if (key == 'q' || key == 'Q') {
            state->programRunning.store(false, std::memory_order_relaxed);
            break;
        }

        if (key == ' ' || key == 'p' || key == 'P') {
            bool currentPlay = state->globalPlay.load(std::memory_order_relaxed);
            state->globalPlay.store(!currentPlay, std::memory_order_relaxed);
            printDashboard(state);
            continue;
        }

        if (key == 'r' || key == 'R') {
            state->currentFrame.store(0, std::memory_order_relaxed);
            printDashboard(state);
            continue;
        }

        if (key >= '1' && key <= '8') {
            int trackIdx = key - '1';
            if (trackIdx >= static_cast<int>(state->tracks.size())) {
                continue;
            }

            bool currentStatus = state->tracks[trackIdx].isPlaying.load(std::memory_order_relaxed);
            state->tracks[trackIdx].isPlaying.store(!currentStatus, std::memory_order_relaxed);
            printDashboard(state);
            continue;
        }

        if (key == '\n' || key == '\r') {
            continue;
        }

        printDashboard(state);
    }
}