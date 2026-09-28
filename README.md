# MinimalChainVST3

JUCE/C++ VST3 effect prototype.

Signal chain:
Input -> Delay -> Low Pass -> High Pass -> Distortion -> LA-2A-style Compressor -> Output

Visible controls:
- Delay: Time, Feedback, Mix
- Low Pass: Cutoff, Resonance
- High Pass: Cutoff, Resonance
- Distortion: Tone, Amount, Mix
- Compressor: Input, Peak Reduction
- Output: Gain

The compressor uses an optical-style gain computer with program-dependent attack/release.
This is LA-2A-inspired behavior, not a circuit-perfect emulation.

## Build
Install JUCE 8 and CMake 3.22+.

Then configure with:
cmake -B build -S .
cmake --build build --config Release

The VST3 target is `MinimalChainVST3`.
