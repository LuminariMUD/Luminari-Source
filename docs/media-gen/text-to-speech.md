# Fish Audio text-to-speech API

<!-- cspell:words fishaudio httpx kbps msgpack ormsgpack packb pathlib prosody bitrate bitrates unparseable -->

> **Important:** The Fish Audio API key and TTS model choice are configured in
> the repository-root `.env` file as `FISH_API_KEY` and
> `FISH_AUDIO_MODEL_ID`. Use the names and placeholders in `.env.example`, never
> commit `.env`, and do not copy its secret values into this document.

Status: Accepted

Audience: Developers integrating generated speech into Project Adventure tooling
or services

Scope: Fish Audio `POST /v1/tts` authentication, request fields, binary response,
examples, and failure handling; excludes WebSocket streaming, speech recognition,
billing, and voice-model administration

Authority: Evidence

Last verified: 2026-07-15

Evidence: [Fish Audio endpoint reference](https://docs.fish.audio/api-reference/endpoint/openapi-v1/text-to-speech),
[canonical OpenAPI schema](https://api.fish.audio/openapi.json), and
[Fish Audio error guide](https://docs.fish.audio/api-reference/errors), checked
2026-07-15

## Supplemental disposition

This post-ledger document is retained at its existing path as non-canonical
external integration evidence. No active migration target exists. It does not
enable a dependency, credential, model, voice, provider, data flow, budget, or
production integration. Any future proposal must be expressed as a versioned
contract and pass the autonomous product, data, privacy, security, rights, and
release suites.

## Project boundary

This document is external integration reference evidence. It does not mean that
Fish Audio is a production dependency or that Project Adventure currently sends
data to Fish Audio. Any production integration requires a versioned
architecture, credential boundary, usage budget, and machine-readable privacy
and voice-rights evidence. Missing or ambiguous evidence rejects the integration
without a human gate.

Never call Fish Audio directly from browser code with a long-lived API key. Keep
the key in a trusted server, build tool, or local generation script, and do not
commit it to the repository.

## Endpoint summary

The request and defaults below follow the official endpoint OpenAPI contract as
verified on 2026-07-15.

| Item                | Value                                                   |
| ------------------- | ------------------------------------------------------- |
| Method              | `POST`                                                  |
| URL                 | `https://api.fish.audio/v1/tts`                         |
| Authentication      | Bearer API key                                          |
| Request types       | `application/json` or `application/msgpack`             |
| Required header     | `model`                                                 |
| Required body field | `text`                                                  |
| Success             | `200` with chunked binary audio in the requested format |

Create and manage API keys at the
[Fish Audio API Keys page](https://fish.audio/app/api-keys/). A local shell or
trusted server process must load these repository-root `.env` settings:

```bash
FISH_API_KEY=YOUR_FISH_API_KEY_HERE
FISH_AUDIO_MODEL_ID=s2.1-pro-free
```

The values above mirror `.env.example`; the real values belong only in `.env`.
Standalone shell examples require these variables to be exported into the shell
environment first. Do not put a real key in `.env.example`, client-side
environment variables, test fixtures, logs, screenshots, or documentation.

## Minimal request

Only `text` is required in the JSON body. The `model` header is required even
though the schema declares a default.

```bash
curl --silent --show-error --fail \
  --connect-timeout 10 \
  --max-time 120 \
  --max-filesize 26214400 \
  --request POST \
  --url https://api.fish.audio/v1/tts \
  --header "Authorization: Bearer ${FISH_API_KEY}" \
  --header "Content-Type: application/json" \
  --header "model: ${FISH_AUDIO_MODEL_ID}" \
  --data '{
    "text": "Welcome, adventurer. Your journey begins now.",
    "format": "mp3"
  }' \
  --output welcome.mp3
```

The success body is audio, not JSON. Save it as bytes or stream it to its final
destination.

## Headers

| Header          | Required | Value                                          |
| --------------- | -------- | ---------------------------------------------- |
| `Authorization` | Yes      | `Bearer` followed by the Fish Audio API key    |
| `Content-Type`  | Yes      | `application/json` or `application/msgpack`    |
| `model`         | Yes      | `s1`, `s2-pro`, `s2.1-pro`, or `s2.1-pro-free` |

The official [text-to-speech guide](https://docs.fish.audio/features/text-to-speech)
describes `s2.1-pro` as the current production recommendation,
`s2.1-pro-free` as the free developer option, `s2-pro` as the model for
multi-speaker dialogue, and `s1` as a previous-generation option. Recheck the
[model selection guide](https://docs.fish.audio/developer-guide/models-pricing/choosing-a-model)
before selecting a production model.

## JSON request fields

Fields not marked required are optional. Defaults come from the canonical
OpenAPI schema rather than from SDK-specific defaults.

| Field                          | Type                                             | Required | Default  | Rules and purpose                                                                                               |
| ------------------------------ | ------------------------------------------------ | -------- | -------- | --------------------------------------------------------------------------------------------------------------- |
| `text`                         | string                                           | Yes      | -        | Text to synthesize. Speaker tags are supported for multi-speaker requests.                                      |
| `reference_id`                 | string, string array, or null                    | No       | `null`   | Saved voice-model ID for one speaker, or one ID per speaker for `s2-pro` dialogue.                              |
| `references`                   | reference array, nested reference array, or null | No       | -        | Inline zero-shot voice samples. Binary samples require MessagePack.                                             |
| `temperature`                  | number                                           | No       | `0.7`    | Expressiveness; range `0` through `1`. Higher values are more varied.                                           |
| `top_p`                        | number                                           | No       | `0.7`    | Nucleus-sampling diversity; range `0` through `1`.                                                              |
| `prosody`                      | object or null                                   | No       | `null`   | Speed, volume, and optional loudness normalization.                                                             |
| `chunk_length`                 | integer                                          | No       | `300`    | Text segment size; range `100` through `300`.                                                                   |
| `normalize`                    | boolean                                          | No       | `true`   | Normalizes English and Chinese text, including numbers.                                                         |
| `format`                       | string                                           | No       | `mp3`    | `wav`, `pcm`, `mp3`, or `opus`.                                                                                 |
| `sample_rate`                  | integer or null                                  | No       | `null`   | Output sample rate in hertz. A null value uses the format default.                                              |
| `mp3_bitrate`                  | integer                                          | No       | `128`    | MP3 only; `64`, `128`, or `192` kbps.                                                                           |
| `opus_bitrate`                 | integer                                          | No       | `-1000`  | Opus only; `-1000` selects automatic, or use `24000`, `32000`, `48000`, or `64000` bps.                         |
| `latency`                      | string                                           | No       | `normal` | `normal` favors quality, `balanced` reduces latency, and `low` minimizes latency.                               |
| `max_new_tokens`               | integer                                          | No       | `1024`   | Maximum audio tokens generated for each text chunk.                                                             |
| `repetition_penalty`           | number                                           | No       | `1.2`    | Values above `1.0` reduce repeated audio patterns.                                                              |
| `min_chunk_length`             | integer                                          | No       | `50`     | Minimum characters before a new chunk; range `0` through `100`.                                                 |
| `condition_on_previous_chunks` | boolean                                          | No       | `true`   | Uses previous audio as context to improve voice consistency.                                                    |
| `early_stop_threshold`         | number                                           | No       | `1`      | Batch-processing early-stop threshold; range `0` through `1`.                                                   |
| `features`                     | string array                                     | No       | `[]`     | Request-scoped backend flags. `quality-guard` is documented, but availability depends on the inference backend. |

Start with the defaults. Change sampling and chunk controls only with repeatable
transcription, pronunciation, prosody, similarity, and acoustic-quality tests
against representative game copy.

### Prosody object

| Field                | Type    | Default | Rules and purpose                                                               |
| -------------------- | ------- | ------- | ------------------------------------------------------------------------------- |
| `speed`              | number  | `1`     | Speaking-rate multiplier. The documented range is `0.5` through `2.0`.          |
| `volume`             | number  | `0`     | Volume adjustment in decibels; positive is louder and negative is quieter.      |
| `normalize_loudness` | boolean | `true`  | Normalizes perceived loudness. The endpoint schema marks this as `s2-pro` only. |

Example:

```json
{
  "text": "The gate is opening.",
  "prosody": {
    "speed": 0.9,
    "volume": -1,
    "normalize_loudness": true
  },
  "format": "wav",
  "sample_rate": 44100
}
```

## Voice selection

### Saved voice model

For repeated use, Fish Audio recommends creating or selecting a voice model and
sending its ID as `reference_id`. Pre-uploaded reference audio generally improves
quality and reduces request latency.

```bash
export FISH_VOICE_ID="voice-model-id"

curl --silent --show-error --fail \
  --connect-timeout 10 \
  --max-time 120 \
  --max-filesize 26214400 \
  --request POST \
  --url https://api.fish.audio/v1/tts \
  --header "Authorization: Bearer ${FISH_API_KEY}" \
  --header "Content-Type: application/json" \
  --header "model: ${FISH_AUDIO_MODEL_ID}" \
  --data "{\"text\":\"The forest remembers.\",\"reference_id\":\"${FISH_VOICE_ID}\",\"format\":\"mp3\"}" \
  --output narration.mp3
```

The `voice-model-id` value above is illustrative. Replace it with a model ID the
calling account is permitted to use.

### Inline reference audio

Inline zero-shot cloning uses `references` and requires
`Content-Type: application/msgpack`; JSON cannot carry the raw binary sample.
Do not send multipart form data to this endpoint.

Each `ReferenceAudio` object contains:

| Field   | Type         | Required | Purpose                        |
| ------- | ------------ | -------- | ------------------------------ |
| `audio` | binary bytes | Yes      | WAV, MP3, or FLAC voice sample |
| `text`  | string       | Yes      | Exact transcript of the sample |

Fish Audio recommends a clean 10-30 second voice sample with minimal background
noise. Use only voices and recordings for which the project has the necessary
rights and consent.

This Python example encodes the binary sample with MessagePack and streams the
result to disk:

```python
import os
from pathlib import Path

import httpx
import ormsgpack

api_key = os.environ["FISH_API_KEY"]
model_id = os.environ["FISH_AUDIO_MODEL_ID"]
max_audio_bytes = 25 * 1024 * 1024
output_path = Path("zero-shot.mp3")
partial_path = output_path.with_suffix(".mp3.part")
payload = {
    "text": "Spoken in the reference voice.",
    "references": [
        {
            "audio": Path("reference.wav").read_bytes(),
            "text": "The exact words spoken in the reference recording.",
        }
    ],
    "format": "mp3",
}

try:
    with httpx.stream(
        "POST",
        "https://api.fish.audio/v1/tts",
        content=ormsgpack.packb(payload),
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/msgpack",
            "model": model_id,
        },
        timeout=120.0,
    ) as response:
        response.raise_for_status()
        received_bytes = 0
        with partial_path.open("xb") as output:
            for chunk in response.iter_bytes():
                received_bytes += len(chunk)
                if received_bytes > max_audio_bytes:
                    raise ValueError("Fish Audio response exceeded the byte limit")
                output.write(chunk)
    partial_path.replace(output_path)
except Exception:
    partial_path.unlink(missing_ok=True)
    raise
```

## Multi-speaker dialogue

The current endpoint schema documents multi-speaker synthesis for `s2-pro`. Set
`FISH_AUDIO_MODEL_ID=s2-pro` in `.env` for this request, provide a voice-model ID
array, and use zero-based speaker tags in `text`. The tag index maps to the same
index in `reference_id`.

```bash
curl --silent --show-error --fail \
  --connect-timeout 10 \
  --max-time 120 \
  --max-filesize 26214400 \
  --request POST \
  --url https://api.fish.audio/v1/tts \
  --header "Authorization: Bearer ${FISH_API_KEY}" \
  --header "Content-Type: application/json" \
  --header "model: ${FISH_AUDIO_MODEL_ID}" \
  --data '{
    "text": "<|speaker:0|>Is the path clear?<|speaker:1|>Not yet. Wait for my signal.",
    "reference_id": ["scout-voice-id", "warden-voice-id"],
    "format": "mp3"
  }' \
  --output dialogue.mp3
```

For zero-shot multi-speaker synthesis, `references` is a nested array. Each inner
array contains the reference samples for one speaker, and `reference_id` is an
array of identifiers in the same order. Inline audio still requires MessagePack.

## Output formats

The format limits below come from the official endpoint reference as verified on
2026-07-15.

| Format | Sample rates                                          | Encoding                                | Bitrate choices                   |
| ------ | ----------------------------------------------------- | --------------------------------------- | --------------------------------- |
| `wav`  | 8000, 16000, 24000, 32000, or 44100 Hz; default 44100 | 16-bit mono PCM in a WAV container      | Not applicable                    |
| `pcm`  | 8000, 16000, 24000, 32000, or 44100 Hz; default 44100 | Raw 16-bit mono PCM without a container | Not applicable                    |
| `mp3`  | 32000 or 44100 Hz; default 44100                      | Mono MP3                                | 64, 128, or 192 kbps; default 128 |
| `opus` | 48000 Hz                                              | Mono Opus                               | Automatic, 24, 32, 48, or 64 kbps |

Choose the output extension from `format`. A `200` response is transferred in
chunks, but the REST request still sends the complete input text up front. Use
the separate Fish Audio WebSocket API when text itself arrives incrementally.

## Server-side JavaScript example

This example streams a successful response to a file without buffering the full
audio in memory:

```javascript
import { createWriteStream } from "node:fs";
import { rename, unlink } from "node:fs/promises";
import { Readable } from "node:stream";
import { pipeline } from "node:stream/promises";

const apiKey = process.env.FISH_API_KEY;
const modelId = process.env.FISH_AUDIO_MODEL_ID;
const maxAudioBytes = 25 * 1024 * 1024;
const outputPath = "distant-bell.mp3";
const partialPath = `${outputPath}.part`;

if (!apiKey || !modelId) {
  throw new Error("Fish Audio settings are not configured");
}

const response = await fetch("https://api.fish.audio/v1/tts", {
  method: "POST",
  signal: AbortSignal.timeout(120_000),
  headers: {
    Authorization: `Bearer ${apiKey}`,
    "Content-Type": "application/json",
    model: modelId,
  },
  body: JSON.stringify({
    text: "A distant bell rings across the valley.",
    format: "mp3",
  }),
});

if (!response.ok) {
  throw new Error(`Fish Audio request failed with HTTP ${response.status}`);
}

if (!response.body) {
  throw new Error("Fish Audio returned no response body");
}

async function* limitAudioBytes(body) {
  let receivedBytes = 0;

  for await (const chunk of Readable.fromWeb(body)) {
    receivedBytes += chunk.byteLength;
    if (receivedBytes > maxAudioBytes) {
      throw new Error("Fish Audio response exceeded the byte limit");
    }
    yield chunk;
  }
}

try {
  await pipeline(limitAudioBytes(response.body), createWriteStream(partialPath, { flags: "wx" }));
  await rename(partialPath, outputPath);
} catch (error) {
  await unlink(partialPath).catch(() => undefined);
  throw error;
}
```

This code belongs in a trusted Node.js process, not in the browser bundle.

## Responses and errors

### Success

| Status | Body                 | Handling                                                                          |
| ------ | -------------------- | --------------------------------------------------------------------------------- |
| `200`  | Chunked binary audio | Stream or save using the extension selected by `format`. Do not parse it as JSON. |

The OpenAPI response does not promise a filename or a JSON envelope. The caller
owns file naming and storage.

### Failure

Most Fish Audio errors use this JSON shape:

```json
{
  "status": 401,
  "message": "Invalid Token"
}
```

An unparseable request body may instead return a plain-text parse error. Input
validation errors from this endpoint use a JSON array:

```json
[
  {
    "loc": ["body", "chunk_length"],
    "type": "validation_error",
    "msg": "Invalid value"
  }
]
```

The endpoint schema explicitly declares `401`, `402`, and `422`. The general
Fish Audio error contract also documents the other statuses below.

| Status | Meaning                                    | Action                                                                |
| ------ | ------------------------------------------ | --------------------------------------------------------------------- |
| `400`  | Invalid request or missing voice reference | Correct the parameters or model ID.                                   |
| `401`  | Missing or invalid API key                 | Check the bearer token and server configuration.                      |
| `402`  | Insufficient credits                       | Check the account balance and billing configuration.                  |
| `403`  | Key is not permitted to use the resource   | Check key scope and resource ownership.                               |
| `404`  | Model or voice not found                   | Check the requested model or `reference_id`.                          |
| `422`  | Request validation failed                  | Read the validation array and correct the named field.                |
| `429`  | Rate limit exceeded                        | Retry with bounded exponential backoff and jitter.                    |
| `5xx`  | Fish Audio service failure                 | Retry with bounded exponential backoff; escalate persistent failures. |

Retry only `429` and transient `5xx` responses automatically. Other `4xx`
responses require a request, permission, credential, or billing change. Consult
the current [pricing and rate-limit guide](https://docs.fish.audio/developer-guide/models-pricing/pricing-and-rate-limits)
instead of hard-coding a plan limit into the integration.

## Integration checklist

- Keep `FISH_API_KEY` in `.env` and outside source control and
  browser-delivered code.
- Select the request model through `FISH_AUDIO_MODEL_ID` in `.env`.
- Send the required `model` header on every request.
- Match the output filename and decoder to `format`.
- Treat a successful body as binary and an error body as JSON or plain text.
- Use MessagePack, not JSON or multipart data, for inline reference audio.
- Put timeouts and size limits around generation and downstream storage.
- Retry only rate-limit and transient server failures.
- Avoid logging API keys, full synthesis text, or reference-audio bytes.
- Confirm consent, provenance, and allowed use for every cloned voice.
- Recheck the OpenAPI schema, model guidance, pricing, and rate limits before a
  production rollout.

## Source maintenance

Reverify this guide when Fish Audio changes its OpenAPI schema, model lineup,
SDK behavior, pricing, or rate-limit documentation. The primary sources are:

- [Text-to-speech endpoint](https://docs.fish.audio/api-reference/endpoint/openapi-v1/text-to-speech)
- [Canonical OpenAPI schema](https://api.fish.audio/openapi.json)
- [Text-to-speech implementation guide](https://docs.fish.audio/features/text-to-speech)
- [Error handling](https://docs.fish.audio/api-reference/errors)
- [Model selection](https://docs.fish.audio/developer-guide/models-pricing/choosing-a-model)
- [Pricing and rate limits](https://docs.fish.audio/developer-guide/models-pricing/pricing-and-rate-limits)
