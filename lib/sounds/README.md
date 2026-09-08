# Luminari MSP demonstration sound pack

These two original synthesized cues are distributed under the project's
Unlicense/public-domain terms. They contain no sampled recordings or third-party
media. No attribution is required. The existing ElevenLabs onboarding catalog is
not used: it does not establish redistribution rights for this sound pack.

| Filename | Meaning | Duration |
| --- | --- | --- |
| `luminari-test.wav` | Three ascending notes for `sound test` | 0.60 seconds |
| `luminari-door-open.wav` | Two ascending notes for successful door opening | 0.28 seconds |

Both files are mono, 16-bit PCM WAV at 22050 Hz. Regenerate them with
`python3 scripts/development/generate_msp_sounds.py` from the repository root.
The script uses only the Python standard library and produces the same files.

## Client setup

Copy both WAV files into your client's MSP sound search directory, retaining the
exact filenames. Enable MSP in the client, connect, enter `sound on`, and then
`sound test`. `sound status` distinguishes server consent from negotiated MSP.
Use `sound off` to stop future effects; client volume/mute controls remain useful.

The server sends filenames, not an automatic-download URL. Distribution is through
this repository's `lib/sounds` directory. Client-specific directory locations and
playback versions must be recorded in the acceptance report after verification;
no particular client/version is claimed as tested yet.

If a cue is silent, check file placement, MSP settings, volume/mute, and the output
device. A sent trigger is not a playback acknowledgement. Missing files do not
interrupt gameplay. Text feedback remains available for every gameplay cue.

References: [MSP specification](https://wiki.mudlet.org/images/7/73/MUD_Sound_Protocol.pdf)
and [Mudlet supported protocols](https://wiki.mudlet.org/w/Manual:Supported_Protocols#MSP).
