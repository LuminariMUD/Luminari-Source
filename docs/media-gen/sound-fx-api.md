# ElevenLabs sound-effects API

<!-- cspell:words allowlisting ElevenLabs nonaudio noncommercial sublicensing -->

> **Credential boundary:** `.env.example` declares
> `ELEVENLABS_API_KEY`. Put a real value only in the ignored repository-root
> `.env` file when performing a versioned, budget-bounded generation run. Never put
> the value in this document, `.env.example`, source, browser code, a `VITE_`
> variable, tests, logs, screenshots, prompts, or retained evidence.

Status: Proposed

Audience: Audio automation maintainers, game developers, security validators,
originality validators, and specification maintainers

Scope: ElevenLabs `POST /v1/sound-generation` as a pre-production source for
Project Adventure sound-effect candidates, including authentication, request and
response fields, prompting, local scratch handling, provenance, failure, and
rights checks; excludes runtime vendor integration, music, speech, final asset
acceptance, and changes to the proposed hybrid audio direction

Authority: Evidence

Last verified: 2026-07-15

Evidence: [ElevenLabs sound-effects API
reference](https://elevenlabs.io/docs/api-reference/text-to-sound-effects/convert),
[sound-effects overview](https://elevenlabs.io/docs/overview/capabilities/sound-effects),
[API authentication](https://elevenlabs.io/docs/api-reference/authentication),
[API errors](https://elevenlabs.io/docs/eleven-api/resources/errors),
[commercial-use guidance](https://help.elevenlabs.io/hc/en-us/articles/13313564601361-Can-I-publish-the-content-I-generate-on-the-platform),
and [Sound Effects Terms](https://elevenlabs.io/sound-effects-terms), checked
2026-07-15

## Supplemental disposition

This document is external-integration evidence for a proposed production source.
It does not make ElevenLabs a product dependency, accept downloaded audio
files, or make a successful provider response shippable. A live batch runs only
from a versioned batch configuration with bounded cost; shipping requires the
autonomous acceptance suite below.

The accepted Phase 02-05 replica baseline still requires local procedural audio
and no downloaded audio. The proposed hybrid direction is documented in the
[audio production findings](../../ongoing-projects/audio-production-findings.md).
File-based audio requires a versioned manifest contract plus synchronized
specification changes that pass repository validation before implementation.

## Project boundary

ElevenLabs may be used only before build, from a local or otherwise trusted
production tool. The shipped browser game must contain:

- no ElevenLabs SDK, API key, account identifier, generation endpoint, remote
  audio URL, telemetry, or vendor fallback;
- no request to ElevenLabs or any other non-product origin;
- only locally packaged, provenance-accepted runtime audio if the file-based
  direction is later accepted; and
- fully playable muted behavior with redundant nonaudio feedback.

This endpoint is for sound-effect candidates only. Project Adventure currently
plans no recorded speech, narration, battle barks, or game-owned text-to-speech.
Instrumental music belongs to the separately proposed music workflow, not the
sound-effects endpoint.

The production inventory is the [conservative audio
checklist](../../ongoing-projects/audio-production-findings.md#conservative-production-checklist).
Do not infer new footsteps, ambience, actor voices, decorative Foley, songs, or
cue identities from generic ElevenLabs examples.

## Current repository state

As verified on 2026-07-15:

- `.env.example` declares only the safe placeholder name
  `ELEVENLABS_API_KEY`;
- no ElevenLabs package is present in `package.json`;
- no repository generation script or accepted audio-output location exists;
- `tmp/` is ignored and is the safe location for disposable local candidates;
  and
- missing ElevenLabs credentials do not block normal product, documentation,
  replica, archive, build, or test commands.

The examples below are provider-integration references, not Current repository
commands or evidence that a live request has been executed.

## Credential setup

The committed placeholder is:

```bash
ELEVENLABS_API_KEY=YOUR_ELEVENLABS_API_KEY_HERE
```

Copy the variable name into the repository-root `.env` and replace only the
ignored local value. A shell example assumes the calling process already has
`ELEVENLABS_API_KEY` in its environment; it does not require or authorize a
particular secret-loading tool.

Use a dedicated key with:

- only the Sound Effects permission required by the generation workflow;
- a bounded credit quota from the versioned batch configuration;
- IP allowlisting when the account and execution environment support it; and
- rotation or revocation after suspected disclosure.

Never print the environment, enable shell tracing around a request, paste the
key into a command, or send it to browser code. A key found in source or retained
output must be revoked or rotated; deleting the visible line is not sufficient
containment.

## Endpoint contract

The request contract below follows the official API reference as verified on
2026-07-15.

| Item           | Value                                                         |
| -------------- | ------------------------------------------------------------- |
| Method         | `POST`                                                        |
| URL            | `https://api.elevenlabs.io/v1/sound-generation`               |
| Authentication | `xi-api-key: ${ELEVENLABS_API_KEY}`                           |
| Request        | `application/json`                                            |
| Success        | Binary generated audio with response metadata in HTTP headers |

Although the endpoint schema renders `xi-api-key` as optional, the official
authentication guide requires an API key for API requests. Project tooling must
treat it as required.

### Query parameter

| Name            | Required | Project treatment                                                                                                                               |
| --------------- | -------- | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| `output_format` | No       | Record the exact selected enum. Values use `codec_sample_rate_bitrate`; availability and higher-quality formats can depend on the account plan. |

Do not copy a stale list of all output enums into project tooling. Re-read the
live API reference before the pilot and final batches. The current overview says
MP3 is available for all effects and 48 kHz WAV for non-looping effects. The
versioned runtime codec policy selects the first supported format that passes
browser decode, size, and quality thresholds.

### JSON request body

| Field              | Required | Current default           | Rules and Project Adventure treatment                                                                                    |
| ------------------ | -------- | ------------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| `text`             | Yes      | None                      | Original cue brief only. Do not include archive text, artist names, copied signatures, private data, or unreleased copy. |
| `loop`             | No       | `false`                   | Smooth looping is available only with `eleven_text_to_sound_v2`. Keep false for the current one-shot catalog.            |
| `duration_seconds` | No       | `null`                    | API reference range is 0.5 through 30 seconds; null lets the model infer duration.                                       |
| `prompt_influence` | No       | `0.3`                     | Range 0 through 1. Higher values follow the prompt more literally and reduce variation.                                  |
| `model_id`         | No       | `eleven_text_to_sound_v2` | Set explicitly in a recorded production request so the provenance record does not depend on a moving default.            |

The overview currently states a 0.1-second lower duration bound while the API
reference states 0.5 seconds. Treat the endpoint reference as the integration
contract and validate explicit durations from 0.5 through 30 seconds until the
two official sources agree.

### Success response

The success body is audio bytes, not JSON. The API reference documents the
`character-cost` response header. The general API documentation also identifies
`request-id` and `x-trace-id` as useful request metadata.

For every successful candidate, retain safe provenance fields before editing:

- exact output format and byte length;
- `character-cost`, `request-id`, and `x-trace-id` when returned;
- SHA-256 of the original response bytes; and
- generation time, model, prompt, parameters, and candidate disposition.

Do not retain the API key, raw account response, unrelated workspace metadata,
or full provider error payload as evidence.

## Minimal candidate request

Run a live request only when the generation batch, account entitlement, prompt,
and usage budget pass their configured preflight checks. This example writes
one disposable candidate under ignored `tmp/`; it does not create an accepted
source or runtime asset.

```bash
mkdir -p tmp/audio-generation/elevenlabs

printf 'xi-api-key: %s\n' "$ELEVENLABS_API_KEY" | curl --silent --show-error --fail \
  --connect-timeout 10 \
  --max-time 120 \
  --max-filesize 26214400 \
  --request POST \
  --url "https://api.elevenlabs.io/v1/sound-generation?output_format=mp3_44100_128" \
  --header @- \
  --header "Content-Type: application/json" \
  --data '{
    "text": "Short polished glass and brass interface confirmation, warm precise transient, close and dry mix, no voice, no words, no music, no melody, under one second",
    "duration_seconds": 0.8,
    "prompt_influence": 0.4,
    "loop": false,
    "model_id": "eleven_text_to_sound_v2"
  }' \
  --output tmp/audio-generation/elevenlabs/ui-confirm-candidate-001.mp3
```

Use a unique candidate filename for every request. Never overwrite a prior
candidate or imply that repeating the same prompt will reproduce identical
bytes. The header is supplied on standard input so the expanded key is not a
`curl` process argument.

## Node.js request pattern

Project Adventure uses supported Node.js 24. This illustrative pattern uses the
built-in `fetch` API and returns bounded bytes plus safe response metadata. It
intentionally does not write into a production asset directory.

```javascript
const endpoint = new URL("https://api.elevenlabs.io/v1/sound-generation");
endpoint.searchParams.set("output_format", "mp3_44100_128");

const apiKey = process.env.ELEVENLABS_API_KEY;
const maximumAudioBytes = 25 * 1024 * 1024;

if (!apiKey) {
  throw new Error("ElevenLabs sound-effect generation is not configured");
}

const response = await fetch(endpoint, {
  method: "POST",
  signal: AbortSignal.timeout(120_000),
  headers: {
    "Content-Type": "application/json",
    "xi-api-key": apiKey,
  },
  body: JSON.stringify({
    text: "Short layered impact, firm attack, restrained tail, no voice, no words, no music, no melody",
    duration_seconds: 0.9,
    prompt_influence: 0.4,
    loop: false,
    model_id: "eleven_text_to_sound_v2",
  }),
});

const requestId = response.headers.get("request-id") ?? "unavailable";

if (!response.ok) {
  throw new Error(`ElevenLabs generation failed: status=${response.status} requestId=${requestId}`);
}

const declaredLength = Number(response.headers.get("content-length"));

if (Number.isFinite(declaredLength) && declaredLength > maximumAudioBytes) {
  throw new Error("ElevenLabs audio exceeded the candidate byte limit");
}

if (!response.body) {
  throw new Error("ElevenLabs returned no candidate body");
}

const reader = response.body.getReader();
const chunks = [];
let receivedBytes = 0;

try {
  while (true) {
    const { done, value } = await reader.read();
    if (done) break;
    receivedBytes += value.byteLength;
    if (receivedBytes > maximumAudioBytes) {
      await reader.cancel("candidate byte limit exceeded");
      throw new Error("ElevenLabs audio exceeded the candidate byte limit");
    }
    chunks.push(value);
  }
} finally {
  reader.releaseLock();
}

const bytes = new Uint8Array(receivedBytes);
let offset = 0;
for (const chunk of chunks) {
  bytes.set(chunk, offset);
  offset += chunk.byteLength;
}

if (bytes.byteLength === 0) {
  throw new Error("ElevenLabs returned an empty candidate");
}

const candidate = {
  bytes,
  requestId,
  traceId: response.headers.get("x-trace-id"),
  characterCost: response.headers.get("character-cost"),
  contentType: response.headers.get("content-type"),
};
```

A future repository-owned script must additionally validate content type, write
to a unique `.part` path, clean partial output on failure or cancellation, hash
the exact bytes, record bounded provenance, and promote nothing automatically.

## Project prompt contract

Start from one checklist role and write a cue brief with these fields:

| Field                   | Prompt content                                                                                      |
| ----------------------- | --------------------------------------------------------------------------------------------------- |
| Role                    | Exact checklist event and whether it is essential, meaning-bearing, or decorative.                  |
| Source and material     | What physically or synthetically produces the sound.                                                |
| Action                  | Impact, movement, pulse, scrape, release, rise, decay, or another concrete acoustic event.          |
| Timing                  | One-shot or loop, target duration, attack speed, sustain, and tail.                                 |
| Perspective and space   | Close, distant, dry, reflected, narrow, broad, or otherwise mix-relevant placement.                 |
| Intensity and frequency | Subtle through terminal, plus the frequency range that should remain clear around music and cues.   |
| Exclusions              | No voice, words, singing, music, melody, clipping, excessive tail, or other role-specific failures. |

A useful prompt describes an audio asset, not game lore. Keep the same validated
material and mix vocabulary across a cue family, then change only the event,
intensity, or variation being tested.

Examples for candidate briefs:

- Menu navigation: `Very short neutral interface tick, precise soft transient,
dry and quiet, repeat-safe, no voice, no words, no music, no melody`.
- Major threat: `Short rising mechanical-and-air warning, immediate readable
attack, restrained tail, clear over combat, no voice, no words, no music`.
- Player damage: `Compact layered impact with a brittle energy crack, urgent but
not cinematic, close perspective, short tail, no vocalization, no music`.
- Tonic use: `Brief restorative liquid-and-resonance shimmer, health and energy
recovery, warm release, no voice, no melody, under one second`.

Do not prompt with an artist, franchise, branded sonic logo, recognizable song,
third-party character, archive proper noun, copied narrative phrase, or a request
to imitate protected audio. A generic musical capability in the provider does
not authorize its use for the Project Adventure sound-effect catalog.

### Parameter starting points

- Use `loop: false` for the current one-shot inventory.
- Set explicit duration for timing-critical combat and interface cues.
- Use `prompt_influence` around 0.3 to 0.45 for the first candidate pass, then
  change one variable at a time during comparison.
- Raise prompt influence only when the output misses a required material,
  action, duration, or exclusion.
- Generate multiple candidates instead of treating the first response as an
  accepted asset.

These are Proposed production heuristics, not provider guarantees. Record the
actual parameter values and acoustic-validator result for each candidate.

## Candidate-to-runtime workflow

The required project path is:

```text
validated cue brief
  -> provider request
  -> immutable original response and safe provenance
  -> autonomous selection and originality checks
  -> deterministic editing, trimming, fades, mix, and accepted master
  -> deterministic runtime export
  -> manifest validation and hash
  -> local package
  -> Web Audio decode and playback
```

Apply the following rules:

1. Generate only into a unique ignored scratch location such as
   `tmp/audio-generation/elevenlabs/`.
2. Treat every response as untrusted candidate bytes. Validate size, format,
   duration, channel facts, decode behavior, and hash before scoring.
3. Preserve the exact provider response separately from the edited master.
4. Score several candidates with versioned acoustic, semantic-role, and
   in-game-mix evaluators; select the highest passing candidate deterministically.
5. Reject unwanted speech, singing, music, similarity, artifacts, clipping,
   weak attacks, excessive tails, unusable loops, and poor separation.
6. Record trimming, fades, layering, processing, normalization, mixing, and
   mastering as deterministic modifications.
7. Export with a versioned deterministic recipe and stable project-owned asset
   identity. Provider filenames and request URLs never become runtime IDs.
8. Promote the complete accepted source record, runtime output, manifest entry,
   and hash atomically. Failure preserves the prior complete set.

The canonical provenance and promotion rules are owned by the [audio source
contract](../../engineering/data-contracts.md#audio-source-and-provenance) and
[generated-asset ADR](../../engineering/adr/0014-generated-asset-manifest-and-provenance.md).

## Provenance fields

Every generated candidate that remains under consideration should record:

| Field                      | Required evidence                                                                                     |
| -------------------------- | ----------------------------------------------------------------------------------------------------- |
| Candidate ID               | Unique local identity that is not a runtime cue ID.                                                   |
| Intended cue role          | Checklist role and intended game use.                                                                 |
| Provider and product       | ElevenLabs Sound Effects.                                                                             |
| Model and feature status   | Exact model ID and confirmation that the feature was not a prohibited beta for the intended use.      |
| Generation time            | ISO timestamp binding the request to then-current plan and terms.                                     |
| Account entitlement class  | Paid or research-only class without account secrets or unnecessary personal data.                     |
| Prompt and options         | Exact prompt, loop, duration, prompt influence, and output format.                                    |
| Provider request metadata  | Safe request, trace, cost, or history identity when available.                                        |
| Original response          | Byte length, media facts, and SHA-256.                                                                |
| Deterministic modifications | Selection, trimming, arrangement, layering, fades, processing, mixing, and mastering recipes.        |
| Accepted master and export  | Master hash, deterministic export recipe, runtime hash, size, codec, rate, channels, and loop points. |
| Validation                  | Rights, originality, similarity, unwanted speech or music, mix, decode, and archive-boundary results. |
| Disposition                 | Research-only, rejected, selected, accepted, superseded, or shipped after all autonomous checks pass. |

Provider request metadata supports investigation but does not replace the
original-response hash, prompt record, entitlement evidence, acoustic
validation, or project acceptance record.

## Failure, cancellation, duplicate, and retry behavior

| Condition                                       | Treatment                                                                                                                           |
| ----------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| Invalid request or unsupported option           | Correct the cue brief or request; do not retry unchanged.                                                                           |
| Missing, invalid, or under-scoped key           | Stop the provider path without output; use the configured licensed fallback and preserve credential restrictions.                   |
| Insufficient credits or unavailable plan        | Stop the provider path; apply the configured free/licensed fallback without increasing the budget.                                  |
| IP allowlist or permission rejection            | Stop the provider path; keep the restriction intact and apply the configured licensed fallback.                                     |
| Rate limit                                      | Apply bounded exponential backoff and retain the same candidate operation identity.                                                 |
| Concurrency limit                               | Wait for current requests to finish; do not increase parallelism.                                                                   |
| Timeout, `5xx`, or service unavailable          | Retry a bounded number of times only while the same validated request remains current.                                              |
| Cancellation, stale batch, or superseded brief  | Abort or ignore the response, remove partial scratch output, and never promote it.                                                  |
| Duplicate request or repeated callback          | Keep a distinct candidate identity; never overwrite, double-charge a production record, or claim deterministic reproduction.        |
| Empty, oversized, corrupt, or undecodable audio | Quarantine or delete the scratch candidate, retain a safe failure record, and preserve every previously accepted source and output. |

The general API error guide documents `400` validation, `401` authentication,
`402` payment, `403` authorization, `429` rate or concurrency, `500` internal,
and `503` unavailable families. The sound-effect endpoint separately documents
`422` validation errors. Parse provider details only inside the local tool and
retain allowlisted fields such as status, provider code, request ID, candidate
ID, attempt count, and disposition. Do not retain the full prompt or provider
message in routine logs.

Provider failure never creates a runtime generation fallback and never changes
gameplay, records, deterministic state, or release status.

## Rights and production eligibility

These controls summarize current provider evidence; they are not legal advice or
a guarantee of copyright protection or non-infringement.

As verified on 2026-07-15:

- ElevenLabs states that free-plan output has no commercial license.
- It states that paid-plan output may be used commercially when it was not made
  with a Beta Service and the user holds the necessary rights and follows the
  applicable terms and law.
- The Sound Effects Terms allow the account to disable future third-party
  sublicensing of its sound-effect outputs, but the opt-out does not unwind uses
  already granted or commenced.

Before a final production batch:

- verify a paid entitlement and a non-beta Sound Effects feature through
  account/API evidence;
- enable the Sound Effects third-party sublicensing opt-out;
- retain plan, terms, feature-status, date, prompt, and output evidence;
- use only original prompts and authorized inputs;
- run acoustic originality and similarity validation; and
- apply the repository jurisdiction and distribution rights policy, quarantining
  any candidate whose license evidence is insufficient.

Research candidates generated under an ineligible plan remain research-only and
must not be promoted later merely because the account subsequently changes plan.

## Batch checklist

- [ ] The hybrid file-source mode matches the versioned manifest scope, or the
      batch is explicitly research-only.
- [ ] The cue roles come from the current conservative audio checklist.
- [ ] The exact prompts contain no archive terms, copied signatures, artist
      imitation, private data, speech, singing, music, or melody.
- [ ] The key is restricted, locally loaded from `.env`, and absent from command
      output and retained artifacts.
- [ ] The plan, non-beta feature status, terms, opt-out, pricing, and usage budget
      have been rechecked.
- [ ] Candidate IDs, output paths, byte limits, timeouts, cancellation, bounded
      retry, partial cleanup, and duplicate behavior are defined.
- [ ] Every successful response receives exact request metadata and an original
      byte hash before editing.
- [ ] Autonomous selection, deterministic edits, mix, originality, and
      similarity validator results plus rejection reasons are recorded.
- [ ] Accepted masters and runtime exports have deterministic recipes, stable
      project IDs, hashes, provenance, and complete manifest coverage.
- [ ] The packaged game makes no ElevenLabs or other vendor request and remains
      fully playable with audio muted or unavailable.

## Reverification

Recheck the official sources automatically before the pilot, before every final
batch, and whenever the endpoint, model, fields, output formats, plan, pricing,
terms, feature status, distribution model, or project audio contract changes:

- [Create sound effect API reference](https://elevenlabs.io/docs/api-reference/text-to-sound-effects/convert)
- [Sound-effects overview and prompting guide](https://elevenlabs.io/docs/overview/capabilities/sound-effects)
- [API authentication](https://elevenlabs.io/docs/api-reference/authentication)
- [API keys and restrictions](https://elevenlabs.io/docs/overview/administration/workspaces/api-keys)
- [API errors](https://elevenlabs.io/docs/eleven-api/resources/errors)
- [API pricing](https://elevenlabs.io/pricing/api)
- [Commercial-use guidance](https://help.elevenlabs.io/hc/en-us/articles/13313564601361-Can-I-publish-the-content-I-generate-on-the-platform)
- [Sound Effects Terms](https://elevenlabs.io/sound-effects-terms)

For local document quality, run:

```bash
npx prettier --check docs/media-gen/elevenlabs/sound-fx-api.md
npx markdownlint-cli2 docs/media-gen/elevenlabs/sound-fx-api.md
npx cspell lint --no-progress docs/media-gen/elevenlabs/sound-fx-api.md
```

These checks validate formatting and local prose only. They do not contact
ElevenLabs, test a credential, spend credits, accept a generated asset, or
change the proposed production direction.
