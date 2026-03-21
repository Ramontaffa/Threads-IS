#pragma once
#include "AudioState.h"

void rtAudioSetup(AudioState* state);

int startAudioStream(AudioState* state);

int stopAudioStream();
