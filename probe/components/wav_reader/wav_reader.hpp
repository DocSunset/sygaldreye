// Copyright 2026 Travis West
#pragma once
#include <string>
#include <vector>

// Minimal RIFF reader: PCM16 or float32, any rate, channel 0 → mono 48k.
// Extracted from wav_player so sample_player (block region) shares it.
bool read_wav_mono48k(const std::string& path, std::vector<float>& out);
