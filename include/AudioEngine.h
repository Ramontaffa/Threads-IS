#pragma once
#include "AudioState.h"

void rtAudioSetup(AudioState* state);

int startAudioStream(AudioState* state);

int stopAudioStream();

int startInstrumentThreads(AudioState* state);

void wakeInstrumentThreads(AudioState* state);

void stopInstrumentThreads(AudioState* state);
