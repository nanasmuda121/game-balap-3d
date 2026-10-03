#include "audio.h"
#include <cstdlib>

static Wave GenerateToneWave(float freq, float duration, float volume) {
    int sampleRate = 44100;
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)malloc(frameCount * sizeof(short));

    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        float env = 1.0f - ((float)i / frameCount); // Linear decay
        float sample = sinf(2.0f * PI * freq * t) * env * volume;
        data[i] = (short)(sample * 32767.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;
    return wave;
}

static Wave GenerateNoiseWave(float duration, float volume) {
    int sampleRate = 44100;
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)malloc(frameCount * sizeof(short));

    float last = 0.0f;
    for (int i = 0; i < frameCount; i++) {
        float white = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        last = (last + white * 0.25f) * 0.85f; // Bandpass filtering
        float env = 1.0f - ((float)i / frameCount) * 0.5f;
        data[i] = (short)(last * env * volume * 32767.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;
    return wave;
}

static Wave GenerateImpactWave() {
    int sampleRate = 44100;
    float duration = 0.35f;
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)malloc(frameCount * sizeof(short));

    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        float env = expf(-14.0f * t);
        float noise = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * 0.4f;
        float thud = sinf(2.0f * PI * (90.0f - t * 150.0f) * t);
        float sample = (thud * 0.7f + noise) * env * 0.9f;
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        data[i] = (short)(sample * 32767.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;
    return wave;
}

static Wave GenerateEngineWave() {
    int sampleRate = 44100;
    float duration = 1.0f; // 1-second loopable engine tone
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)malloc(frameCount * sizeof(short));

    float f0 = 55.0f; // Base engine fundamental frequency (low rumble)
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        // Engine harmonics: f0, 2*f0, 3*f0, 4*f0
        float s1 = sinf(2.0f * PI * f0 * t) * 0.45f;
        float s2 = sinf(2.0f * PI * (f0 * 2.0f) * t) * 0.3f;
        float s3 = sinf(2.0f * PI * (f0 * 3.0f) * t) * 0.15f;
        float s4 = sinf(2.0f * PI * (f0 * 4.0f) * t) * 0.1f;
        float sample = s1 + s2 + s3 + s4;
        data[i] = (short)(sample * 0.6f * 32767.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;
    return wave;
}

AudioSystem::AudioSystem() : m_initialized(false), m_driftCooldown(0.0f) {}

AudioSystem::~AudioSystem() {
    Close();
}

void AudioSystem::Init() {
    if (m_initialized) return;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;

    Wave wBeep = GenerateToneWave(750.0f, 0.12f, 0.7f);
    m_sndBeep = LoadSoundFromWave(wBeep);
    UnloadWave(wBeep);

    Wave wGo = GenerateToneWave(1400.0f, 0.4f, 0.9f);
    m_sndGo = LoadSoundFromWave(wGo);
    UnloadWave(wGo);

    Wave wCrash = GenerateImpactWave();
    m_sndCrash = LoadSoundFromWave(wCrash);
    UnloadWave(wCrash);

    Wave wNitro = GenerateToneWave(450.0f, 0.5f, 0.8f);
    m_sndNitro = LoadSoundFromWave(wNitro);
    UnloadWave(wNitro);

    Wave wDrift = GenerateNoiseWave(0.25f, 0.5f);
    m_sndDrift = LoadSoundFromWave(wDrift);
    UnloadWave(wDrift);

    Wave wEngine = GenerateEngineWave();
    m_sndEngine = LoadSoundFromWave(wEngine);
    UnloadWave(wEngine);

    m_initialized = true;
}

void AudioSystem::Close() {
    if (!m_initialized) return;

    UnloadSound(m_sndBeep);
    UnloadSound(m_sndGo);
    UnloadSound(m_sndCrash);
    UnloadSound(m_sndNitro);
    UnloadSound(m_sndDrift);
    UnloadSound(m_sndEngine);

    CloseAudioDevice();
    m_initialized = false;
}

void AudioSystem::Update(float speedRatio, bool isDrifting, bool isNitro) {
    if (!m_initialized) return;

    // Continuous engine rumble
    if (!IsSoundPlaying(m_sndEngine)) {
        PlaySound(m_sndEngine);
    }
    float pitch = 0.65f + speedRatio * 1.6f;
    if (isNitro) pitch += 0.35f;
    SetSoundPitch(m_sndEngine, pitch);
    SetSoundVolume(m_sndEngine, 0.25f + speedRatio * 0.45f);

    // Drift screeching
    if (isDrifting) {
        if (!IsSoundPlaying(m_sndDrift)) {
            PlaySound(m_sndDrift);
        }
    }
}

void AudioSystem::PlayCountdownBeep(bool isGo) {
    if (!m_initialized) return;
    if (isGo) PlaySound(m_sndGo);
    else PlaySound(m_sndBeep);
}

void AudioSystem::PlayCollision() {
    if (!m_initialized) return;
    PlaySound(m_sndCrash);
}

void AudioSystem::PlayNitro() {
    if (!m_initialized) return;
    PlaySound(m_sndNitro);
}
