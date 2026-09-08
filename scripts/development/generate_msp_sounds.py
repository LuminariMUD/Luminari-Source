#!/usr/bin/env python3
"""Generate the original, public-domain Luminari MSP demonstration cues."""

import math
from pathlib import Path
import struct
import wave

RATE = 22050
DESTINATION = Path(__file__).resolve().parents[2] / "lib" / "sounds"


def write_cue(name, notes):
    """Write frequency/duration pairs as a mono, 16-bit PCM cue with a fading envelope."""
    samples = []
    for frequency, duration in notes:
        count = round(RATE * duration)
        for index in range(count):
            time = index / RATE
            envelope = min(1.0, time / 0.01) * max(0.0, 1.0 - index / count) ** 2
            value = math.sin(2 * math.pi * frequency * time)
            samples.append(round(0.25 * 32767 * envelope * value))
    with wave.open(str(DESTINATION / name), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(RATE)
        output.writeframes(struct.pack("<" + "h" * len(samples), *samples))


def main():
    """Reproduce the two bundled cues in the repository sound directory."""
    DESTINATION.mkdir(parents=True, exist_ok=True)
    write_cue("luminari-test.wav", [(523.25, 0.18), (659.25, 0.18), (783.99, 0.24)])
    write_cue("luminari-door-open.wav", [(220.0, 0.10), (440.0, 0.18)])


if __name__ == "__main__":
    main()
