# MusicAPI.ai instrumental-music API

<!-- cspell:words audiopipe crossfade crossfades ffprobe Kilnflare loopable LUFS Moonveil MusicAPI nonaudio Rimeglass soundfx Suno suno Sunwake tasklist tideglass unmastered unreviewed Verdigris -->

> **Credential boundary:** use the raw MusicAPI.ai token from
> `MUSIC_API_KEY` in the ignored repository-root `.env`. Code constructs the
> `Authorization: Bearer ...` header. Never add `Bearer` to the stored value or
> copy the token into this document, `.env.example`, source, browser code,
> logs, screenshots, prompts, or retained evidence.

Status: Proposed

Audience: Audio automation maintainers, game developers, security validators,
originality validators, and specification maintainers

Scope: MusicAPI.ai Sonic generation as a pre-build source for instrumental
Project Adventure music candidates, including authentication, asynchronous task
handling, the nine Skyglass Drift music packages, prompting, local scratch
handling, technical validation, provenance, adaptive preparation, failure, cost,
and rights validation; excludes runtime vendor integration, lyrics, vocals,
speech, customer-album automation, publishing, and production acceptance

Authority: Evidence

Last verified: 2026-07-15

Evidence: [MusicAPI.ai introduction](https://docs.musicapi.ai/introduction),
[Sonic instructions](https://docs.musicapi.ai/sonic-instructions),
[Sonic create endpoint](https://docs.musicapi.ai/concat-music),
[Sonic task endpoint](https://docs.musicapi.ai/get-sonic-music),
[instrumental guidance](https://docs.musicapi.ai/faq),
[credit guide](https://docs.musicapi.ai/credits-usage-guide),
[WAV endpoint](https://docs.musicapi.ai/wav),
[basic-stem endpoint](https://docs.musicapi.ai/stems-basic), and
[MusicAPI.ai terms](https://musicapi.ai/terms), checked 2026-07-15

## Supplemental disposition

This document is external-integration evidence for a proposed pre-production
source. It does not make MusicAPI.ai or its upstream models a product dependency,
accept file-based music into the runtime, or make a successful provider response
shippable. A live batch runs only from a versioned batch configuration with a
bounded budget; shipping requires the autonomous acceptance suite below.

The accepted Phase 02-05 baseline still requires nine original procedural data
compositions and no downloaded audio. The proposed hybrid direction is recorded
in the [audio production
findings](../../ongoing-projects/audio-production-findings.md). File-based music
requires a versioned manifest contract and synchronized specification changes
that pass repository validation before it enters implementation.

## Project boundary

MusicAPI.ai may be used only before build from a trusted local production tool.
The shipped browser game must contain:

- no MusicAPI.ai SDK, API key, account ID, task ID, generation endpoint,
  provider URL, webhook, analytics, or online fallback;
- no request to MusicAPI.ai, an upstream model provider, or a generated-media
  CDN;
- only local, provenance-accepted runtime exports if the file-based direction is
  later accepted; and
- fully playable muted behavior with independent master, music, and effects
  controls plus redundant nonaudio feedback.

The [archive boundary
ADR](../../engineering/adr/0001-reference-archive-boundary.md) applies to every
brief and output. Do not upload, quote, imitate, transform, or prompt from archive
audio, melodies, rhythms, patches, sound definitions, filenames, or creative
vocabulary. Do not name artists, bands, franchises, copyrighted songs, or
provider catalog tracks in a prompt. Musical direction must be original and
derived only from accepted Skyglass product language.

This workflow is instrumental-only. It does not permit:

- lyrics, intelligible words, narration, battle barks, chanting, sung syllables,
  choir, vocal personas, or uploaded voices;
- `add_vocals`, `persona_music`, lyric-generation, vocal extraction, cover, or
  voice-cloning endpoints;
- customer data, personal data, private source audio, or third-party recordings;
  or
- separate title, class-selection, Codex, shop, Gauntlet, guardian, completion,
  or death songs beyond the nine accepted packages.

## Current repository state

As verified on 2026-07-15:

- `.env.example` declares safe placeholders for `MUSIC_API_KEY`,
  `MUSIC_MODEL_VERSION`, `GEN_MUSIC_ENDPOINT`, and `GET_MUSIC_ENDPOINT`;
- the ignored `.env` contains the local raw token and must never be committed;
- the local model default is `sonic-v5-5`, but each live batch must recheck the
  provider's accepted models and record the requested and returned values;
- no MusicAPI.ai dependency is installed and no browser integration exists;
- the ignored `tmp/audio-generation/musicapi-ai/` directory contains the
  resumable task, refund, source-selection, stem-pilot, and local-package state
  from the explicit paid completion run;
- `skyglass-assets/music/` contains nine exact selected provider source MP3s,
  36 synchronized 48-second layer loops, and 18 four-second transitions; and
- all nine packages remain pending autonomous runtime acceptance, as recorded in
  the [research
  manifest](../../../skyglass-assets/manifest.md#music-candidate-records).

Ordinary documentation maintenance makes no live request. The 2026-07-15 pilot
and nine-package completion batch recorded below were explicit paid runs.
Missing MusicAPI.ai credentials must not affect normal product, documentation,
archive, build, or test commands.

## Credential and endpoint configuration

The committed placeholders are:

```bash
GEN_MUSIC_ENDPOINT=https://api.musicapi.ai/api/v1/sonic/create
GET_MUSIC_ENDPOINT=https://api.musicapi.ai/api/v1/sonic/task/
MUSIC_API_KEY=YOUR_MUSIC_API_KEY_HERE
MUSIC_MODEL_VERSION=sonic-v5-5
```

Put a real value only in `.env`. `MUSIC_API_KEY` is the token alone, without a
`Bearer` prefix. Use a project-specific provider key, a bounded credit balance,
the least account access available, and provider-side IP restrictions when the
account supports them. Rotate or revoke the key after suspected disclosure.

Local Node.js tools should load the ignored file without printing it:

```bash
node --env-file=.env tmp/generate-musicapi-candidate.mjs
```

Never source `.env` into an interactive shell merely to run a request, enable
shell tracing near credentials, interpolate the token into a command argument,
or expose it through a `VITE_` variable. A leaked key must be revoked or rotated;
removing the visible copy is not sufficient containment.

## Project music inventory

The production inventory comes from the [conservative music
checklist](../../ongoing-projects/audio-production-findings.md#songs-and-adaptive-music-packages).
It contains exactly nine original instrumental compositions:

| Package ID          | Composition       | Initial authoring direction                                                                 |
| ------------------- | ----------------- | ------------------------------------------------------------------------------------------- |
| `verdigris-shoals`  | Verdigris Shoals  | Buoyant tideglass exploration, hand-worked brass pulses, open celestial water, lucid warmth |
| `chronometer-grave` | Chronometer Grave | Patient broken-clock motion, low porcelain resonance, suspended age, restrained unease      |
| `tidal-run`         | Tidal Run         | Forward-flowing current, interlocking light percussion, navigational momentum               |
| `kilnflare-reef`    | Kilnflare Reef    | Dry ceramic heat, pressure rhythm, ember-brass tension, controlled intensity                |
| `moonveil-basin`    | Moonveil Basin    | Sparse refracted harmony, soft glass resonance, nocturnal depth, quiet uncertainty          |
| `mercury-verge`     | Mercury Verge     | Quick liquid-metal figures, precise ticking motion, agile tension, clean negative space     |
| `sunwake-expanse`   | Sunwake Expanse   | Broad solar lift, wind-driven rhythm, warm brass over open horizons                         |
| `rimeglass-reach`   | Rimeglass Reach   | Brittle frozen-glass color, slow aurora movement, spacious cold, resilient pulse            |
| `settlement`        | Settlement        | Sheltered workshop calm, cloth and brass warmth, conversational space, no combat insistence |

These directions are bounded research briefs, not new product canon. They use
the accepted Skyglass material language and may be revised during the Phase 05
music-director session.

Each composition ultimately needs one adaptive package, not merely one flat
song. Every package must provide or explicitly resolve:

- a seamless base atmosphere or harmony loop;
- a synchronized rhythm or movement layer;
- a synchronized threat or combat layer, or validated intentional silence;
- a synchronized surge, guardian, or peak layer where consumed;
- compatible transition material and clean loop boundaries; and
- direction coverage for region, settlement, threat, health, chain, surge,
  guardian phase, completion, and death through a layer, short sound-effect
  stinger, or validated intentional silence.

A generated full mix is only source material. The research checklist may mark a
technical package complete only after all named candidate files and provenance
exist. Production acceptance requires the autonomous instrumental-content,
originality, rights, browser-loop, mix, and in-engine evidence suite.

## Relevant provider surface

The project uses the smallest provider surface that can create and retrieve an
instrumental candidate. All requests require HTTPS and Bearer authentication.

| Method | Endpoint                       | Project use                                                                 |
| ------ | ------------------------------ | --------------------------------------------------------------------------- |
| `GET`  | `/api/v1/get-credits`          | Optional preflight balance check; never record the account's full response  |
| `POST` | `/api/v1/sonic/create`         | Submit one `create_music` instrumental task                                 |
| `GET`  | `/api/v1/sonic/task/{task_id}` | Poll the accepted task until every returned clip is terminal                |
| `POST` | `/api/v1/sonic/wav`            | Optional lossless-source request for a selected generated clip              |
| `POST` | `/api/v1/sonic/stems/basic`    | Optional stem experiment for a selected clip; not an automatic package pass |
| `POST` | `/api/v1/sonic/stems/full`     | Exceptional, high-cost stem experiment only when the configured cost policy passes |

The project does not need multi-provider routing, automatic fallback, callbacks,
remote workflow persistence, customer intake, album automation, publishing, or
scheduled balance monitoring. Polling from one trusted local process is
sufficient.

### Cost and request-rate boundary

The provider's 2026-07-15 documentation lists Sonic `create_music` at 15 credits
for two candidates, WAV conversion at 1 credit, basic stems at 15 credits, and
full stems at 75 credits. It lists a standard create limit of one request every
three seconds and recommends polling every 15 to 25 seconds. Pricing, output
count, model availability, and rate limits are provider-controlled and must be
rechecked immediately before any live batch.

The basic-stem endpoint page and credit guide currently disagree about the exact
number of returned stems. Treat its response as untrusted and do not plan the
adaptive package around a claimed count until a bounded pilot confirms the live
shape.

Run one generation task at a time. A nine-package exploration pass normally
means nine sequential create tasks and up to eighteen flat candidates before
selection. WAV or stem requests are separate configured expenditures and should
run only for selected clips.

### Live batch reliability and refund accounting

The 2026-07-15 project run observed nine successful create tasks at 15 credits
each and one successful full-stem task at 75 credits: 210 documented paid
credits total. Three earlier create attempts and four later full-stem attempts
ended in provider errors that reported a refund. This small project sample is
too narrow to estimate a service-wide failure rate, but it is high enough that
the project must treat terminal provider failure as a normal bounded outcome.

Do not accept a refund message as complete accounting when a balance endpoint is
available. Capture only the numeric before/after delta, never the full balance
response or account total. In this run, one independent check observed the
balance increase by exactly 75 credits after a failed full-stem task. The final
Chronometer and Tidal full-stem workers each observed a zero net balance change
after the reported refund. Earlier failures were not individually paired with
before/after snapshots, so their provider messages remain weaker evidence.

After repeated full-stem timeouts, the project stopped submitting that operation
and used deterministic local package derivation. The task runner retries only
after a terminal task state, sufficient remaining budget, and refund
reconciliation where a debit could have occurred. The versioned policy bounds
attempt count and switches automatically to local derivation when exhausted.

## Sonic create request

The project uses `create_music` in description mode with an explicit
instrumental flag.

| Field                    | Required here | Project value or rule                                                                                                     |
| ------------------------ | ------------- | ------------------------------------------------------------------------------------------------------------------------- |
| `task_type`              | Yes           | `create_music`                                                                                                            |
| `custom_mode`            | Yes           | `false`; use a bounded original description rather than lyrics                                                            |
| `mv`                     | Yes           | Exact validated `MUSIC_MODEL_VERSION`; record both requested and returned values                                          |
| `make_instrumental`      | Yes           | `true`; lexical lyric metadata rejects the clip, while a bracket-only marker still requires autonomous acoustic validation |
| `gpt_description_prompt` | Yes           | Original package brief plus common constraints, at most 400 characters                                                    |
| `title`                  | Yes           | Composition name plus a neutral candidate suffix; never treat provider title generation as product canon                  |
| `tags`                   | Optional      | Original instrumentation, texture, pacing, and mix vocabulary only                                                        |
| `negative_tags`          | Optional      | Vocals, words, choir, abrupt ending, clipping, and unwanted genre or mix traits                                           |
| `style_weight`           | Optional      | Record when used; begin at a moderate value and compare candidates                                                        |
| `weirdness_constraint`   | Optional      | Record when used; begin conservatively and change one variable per comparison                                             |

Do not send `prompt`, `lyrics`, `auto_lyrics`, `vocal_gender`, `persona_id`,
`continue_clip_id`, upload URLs, webhook fields, or third-party material in the
initial project request.

The current Sonic instructions set a 400-character maximum for
`gpt_description_prompt`. Older provider pages still mention 200 characters;
the 2026-07-15 live pilot accepted the current 397-character project brief. Keep
new briefs at or below 400, validate locally before submission, and treat a live
validation rejection as authoritative for the selected model.

### Prompt construction

Each brief should state, in this order:

1. package identity and gameplay function;
2. emotional contour and energy range;
3. original instrument and material palette;
4. pulse, density, and transition behavior;
5. loop and stem intent without claiming the provider can create exact loops;
6. mix space reserved for combat sound effects; and
7. unwanted content and originality constraints.

Append a common constraint such as:

> Original instrumental game-music source for an adaptive package; no vocals,
> words, speech, chanting, choir, sung syllables, vocal chops, recognizable
> melody, quotation, pastiche, artist imitation, franchise reference, abrupt
> ending, clipping, or dense mastering. Leave transient and spectral space for
> combat sound effects.

Do not claim that negative wording guarantees a vocal-free or original result.
Every returned clip must pass transcription, vocal-event classification,
acoustic similarity, and originality checks.

### Example request body

This is a project-shaped reference body. Running it spends provider credits.

```json
{
  "task_type": "create_music",
  "custom_mode": false,
  "mv": "sonic-v5-5",
  "make_instrumental": true,
  "title": "Verdigris Shoals candidate 01",
  "gpt_description_prompt": "Full-length instrumental game-music source for Verdigris Shoals: buoyant celestial-ocean exploration, hand-worked brass pulses, glass resonance, restrained frame percussion, lucid warmth, calm navigation building to alert motion and a controlled outro. Layered and spacious with room for combat effects. No vocals, words, choir, artist imitation, recognizable melody, clipping, or dense mastering.",
  "tags": "instrumental game score, celestial ocean, hand-worked brass, glass resonance, restrained percussion, layered, spacious",
  "negative_tags": "vocals, lyrics, spoken words, choir, vocal chops, abrupt ending, clipping, dense mastering"
}
```

## Safe single-task request pattern

Use Node.js 24's built-in `fetch`; no provider SDK is required. A local tool must
validate configuration before spending credits and must never print the key or
the full provider response.

```javascript
const createEndpoint = process.env.GEN_MUSIC_ENDPOINT;
const apiKey = process.env.MUSIC_API_KEY;
const model = process.env.MUSIC_MODEL_VERSION;
const description = "Use the validated project brief here";

if (!createEndpoint || !apiKey || !model) {
  throw new Error("MusicAPI.ai generation is not configured");
}
if (apiKey.startsWith("Bearer ")) {
  throw new Error("MUSIC_API_KEY must contain the raw token only");
}
const createUrl = new URL(createEndpoint);
if (createUrl.origin !== "https://api.musicapi.ai") {
  throw new Error("Unexpected MusicAPI.ai create origin");
}
if (Array.from(description).length > 400) {
  throw new Error("MusicAPI.ai description exceeds 400 characters");
}

const response = await fetch(createUrl, {
  method: "POST",
  headers: {
    Authorization: `Bearer ${apiKey}`,
    "Content-Type": "application/json",
  },
  body: JSON.stringify({
    task_type: "create_music",
    custom_mode: false,
    mv: model,
    make_instrumental: true,
    title: "Verdigris Shoals candidate 01",
    gpt_description_prompt: description,
    tags: "instrumental game score, layered, spacious",
    negative_tags: "vocals, words, choir, vocal chops, clipping",
  }),
  signal: AbortSignal.timeout(30_000),
});

const raw = await response.text();
if (!response.ok || raw.length > 65_536) {
  throw new Error(`MusicAPI.ai submit failed with status ${response.status}`);
}

const result = JSON.parse(raw);
if (
  !result ||
  typeof result !== "object" ||
  Array.isArray(result) ||
  typeof result.task_id !== "string" ||
  result.task_id.length > 128
) {
  throw new Error("MusicAPI.ai returned no bounded task ID");
}

console.log(JSON.stringify({ accepted: true, taskId: result.task_id }));
```

The live pilot returned HTTP 200 with only `message` and `task_id`; the optional
top-level `code` field was absent. Success handling must validate the bounded
task ID and must not require `code`.

Store the accepted task ID immediately in ignored scratch provenance. Once a
valid task ID is returned, do not submit the same package again merely because
polling or downloading times out.

## Polling contract

Poll `GET_MUSIC_ENDPOINT + encodeURIComponent(taskId)` every 15 to 25 seconds.
Use a bounded overall deadline, allow cancellation, and record only safe state
transitions.

The task endpoint documents these states:

| State       | Treatment                                                                                                |
| ----------- | -------------------------------------------------------------------------------------------------------- |
| `pending`   | Wait; do not resubmit or download a placeholder URL                                                      |
| `running`   | Wait; progress is advisory                                                                               |
| `succeeded` | Validate each returned clip and require a final HTTPS audio URL before downloading                       |
| `failed`    | Record a sanitized failure and stop; do not promote bytes                                                |
| Other       | Treat as an untrusted provider-schema change; stop safely without guessing or mutating accepted evidence |

The provider may return an initial `not_ready` object or a task `data` array.
Parse from `unknown`, bound the body before JSON parsing, and validate every
field used. Never log the entire body, generated lyric fields, URLs, account
data, headers, or exception objects that may contain request details.

For a create task, do not declare completion until every returned clip is
terminal. Keep each `clip_id` distinct. A streaming or placeholder URL is not a
final asset; success requires the clip state and a downloadable final URL.

### Live create-response observations

The paid 2026-07-15 pilot and completion batch produced these safe contract
observations:

| Stage or field      | Observed value and required handling                                                                       |
| ------------------- | ---------------------------------------------------------------------------------------------------------- |
| Submit response     | HTTP 200 object with `message` and `task_id`; no `code`                                                    |
| First task poll     | `type: not_ready` object with no `data` array                                                              |
| Later task polls    | HTTP 200 object with numeric `code`, string `message`, and a two-element `data` array                      |
| Clip state sequence | Both clips moved through `running` to `succeeded`; select only after all clips are terminal                |
| `duration`          | Decimal string, not a number; parse to a finite positive number                                            |
| `lyrics`            | A 14-character bracket-only instrumental marker, not an empty string; reject lexical text and still run acoustic validation |
| `mv`                | Returned `sonic-v5-5`, matching the requested model                                                        |
| `video_url`         | `null`; video is irrelevant to this workflow                                                               |
| Final `audio_url`   | HTTPS `cdn1.suno.ai` MP3 URL; validate the allowlisted origin and response bytes                           |
| Provider title      | `Verdigris Shoals`, not the submitted candidate suffix; never use provider title mutation as canon         |

Do not equate bracket-only lyric metadata with proof that the waveform contains
no voice. It permits technical download; the autonomous unwanted-vocal suite
must pass before acceptance.

### Retry and duplicate-spend rules

- Before a request is sent, configuration or validation failure is safe to fix
  and retry.
- A clear HTTP rejection with no task ID may be retried only after correcting
  the cause and confirming the spend boundary.
- A connection loss after request transmission is ambiguous. Do not blindly
  resubmit because no provider idempotency key is documented.
- After a task ID is accepted, retry polling and downloads only. A terminal
  provider failure may create a replacement task only when the configured
  attempt, remaining-budget, and refund-reconciliation rules pass.
- On cancellation, preserve the accepted task ID and current safe state so a
  later autonomous run can resume polling without duplicate generation.

## Download and scratch handling

Generated URLs are untrusted, temporary production inputs. They never become
runtime asset IDs.

1. Require HTTPS and an allowlisted provider or generated-media CDN origin.
2. Use `GET` with redirects bounded to HTTPS; do not rely on `HEAD` behavior.
3. Write to a unique `.part` file under
   `tmp/audio-generation/musicapi-ai/{package-id}/{task-id}/`.
4. Bound download time and bytes before buffering or decoding.
5. Validate MIME, signature, codec, channels, sample rate, duration, and full
   decode with `ffprobe` and `ffmpeg`.
6. Compute SHA-256 before editing and record the exact original byte count.
7. Rename atomically to the candidate filename only after technical validation.
8. Delete incomplete `.part` files on failure while preserving safe task
   provenance.

Suggested scratch names are:

```text
tmp/audio-generation/musicapi-ai/
  provenance.json
  verdigris-shoals/
    {task-id}/
      verdigris-shoals-{clip-id}-provider.mp3
```

Selected research candidates may use:

```text
skyglass-assets/music/
  musicapi-ai-provenance.json
  verdigris-shoals/
    verdigris-shoals-source-01.mp3
```

Do not overwrite an earlier candidate. Promotion means copying the exact
provider response plus its provenance; trimming, looping, stem separation,
mixing, and mastering create new derived records with new hashes.

The selected live responses were `audio/mp3`, 48 kHz stereo, and ranged from
67.719979 to 192.599979 decoded seconds. The Verdigris pilot's ID3 comment
identifies the upstream Suno generator, creation time, and clip ID. Preserve and
hash original metadata where present; metadata stripping creates a derived asset
with a new record.

## Autonomous technical and acoustic validation

Every downloaded candidate must pass:

- full decode with no malformed frames or unexplained truncation;
- finite, plausible duration and file size;
- non-silent waveform and no clipping or severe encoding artifacts;
- start, middle, and end acoustic-model checks plus full-track transcription;
- zero intelligible words, speech, singing, chanting, choir, vocal chops, or
  lyric-like fragments;
- no recognizable melody, imitation, copied structure, archive signature, or
  suspicious similarity to a known work;
- enough mix headroom and spectral room for effects and accessibility cues;
- a usable musical contour for the package's base and escalation roles; and
- exact SHA-256, byte length, codec, sample rate, channels, and decoded duration
  in provenance.

Do not normalize or master a provider response before retaining its original
hash. Loudness, peak, dynamic range, and true-peak targets remain production-mix
decisions; record measured values rather than inventing an acceptance target in
this provider guide.

### Live batch technical result

All nine selected source MP3s passed full FFmpeg decode, FFprobe metadata,
byte-count, SHA-256, and non-silence checks. The original Verdigris pilot also
received the detailed loudness scan: -13.0 LUFS integrated, 7.1 LU loudness
range, -1.0 dBFS true peak, and no silence of at least two seconds below -50 dB.

The local completion pass produced 54 derivatives. All are 48 kHz stereo MP3,
all 36 loops decode to exactly 48 seconds, all 18 transitions decode to exactly
four seconds, and the quietest derivative peaks at -14.6 dB. A decoded-sample
boundary audit found no loop seam outside its file's normal internal
sample-difference distribution; the worst seam ranked at the 98.335th
percentile. These measurements prove technical package integrity, not musical,
vocal, similarity, originality, browser-loop, or production acceptance.

## Adaptive-package preparation

MusicAPI.ai creates source tracks, not a complete deterministic game-music
director. After candidate selection, the deterministic audio pipeline produces
compatible material for the package:

1. retain the original provider MP3 and request a WAV only for a selected clip
   when the account and live endpoint support it;
2. evaluate basic stems on one pilot before spending for a batch;
3. reject contaminated stems and use deterministic lossless-master arrangement
   or regenerate when stems contain bleed, phase problems, vocals, or unstable
   timing;
4. align every layer to one tempo, meter, phrase length, and loop grid;
5. create equal-length base, rhythm, threat, and peak layers or record validated
   intentional silence for unused states;
6. render clean loop boundaries and separate short transition material where
   crossfades alone are insufficient;
7. test synchronized starts, crossfades, ducking, pause, visibility suspension,
   restart, recovery, completion, and death in the Web Audio director; and
8. export runtime files deterministically only after format, transfer, decoded
   memory, and first-region budgets are measured.

For this research batch, one Verdigris full-stem request succeeded at 75 credits
and returned 24 synchronized MP3 stems to ignored scratch. Two requests for the
initial Chronometer source, one for its alternate source, and one Tidal request
then failed upstream with reported refunds. The batch therefore stopped paid
stem work. To close the technical candidate inventory consistently, every
package uses the same local FFmpeg fallback: a four-second cyclic crossfade into
a 48-second master, complementary 8th-order crossovers at 500, 1900, and 5100
Hz, bounded role gains and limiting, settled filter state from a repeated master,
and matched four-second rise/fall transitions. These are independently
gain-controllable frequency layers, not semantic provider stems, and require
automated loudness, masking, clipping, transition, and in-engine mix checks
before production use.

Stem separation is an editing aid, not proof of independent musical authorship,
clean loops, or mix compatibility. If no candidate can support an adaptive
package, reject it and author new source material rather than shipping a flat
track under an adaptive label.

## Provenance and manifest record

Every candidate and derived asset must have a safe record. The record must not
contain credentials, full response payloads, secret-bearing URLs, personal data,
or unrelated account information.

| Field                        | Purpose                                                                                          |
| ---------------------------- | ------------------------------------------------------------------------------------------------ |
| Stable package and role IDs  | Decouple game-facing meaning from provider names, titles, clip IDs, and URLs                     |
| Provider and endpoint        | Record MusicAPI.ai Sonic and the operation used                                                  |
| Requested and returned model | Detect provider aliasing or model drift                                                          |
| Generation timestamp         | Bind the output to the plan and terms captured at generation time                                |
| Exact prompt and parameters  | Preserve the original authorized brief and generation settings                                   |
| Task ID and clip ID          | Support bounded provider-side traceability without storing raw responses                         |
| Plan-class state and cost    | Record confirmed entitlement or an explicit pending state plus cost without storing account data |
| Terms evidence               | Store the official URL, checked date, hash, and retained snapshot location                       |
| Original output metadata     | Record SHA-256, bytes, codec, sample rate, channels, duration, and measured audio properties     |
| Vocal and originality checks | Record validator versions, timestamps, scores, result, similarity evidence, and rejection reason |
| Deterministic edits          | Record selection, arrangement, stem processing, trimming, looping, mixing, and mastering recipes |
| Master and runtime hashes    | Identify each derived byte sequence and its deterministic export recipe                          |
| Package mapping              | Map base, movement, threat, peak, transitions, and intentional-silence decisions                 |
| Acceptance state             | Keep unknown, unchecked, rejected, or research-only output out of the production audio manifest  |

The research manifest is
[`skyglass-assets/manifest.md`](../../../skyglass-assets/manifest.md). The
production schema and admission rules remain owned by the [engineering audio
contracts](../../engineering/data-contracts.md#audio-source-manifest-and-lifecycle).

Checking a technical research composition or package box in the audio findings
requires all named candidate members and safe provenance to exist. A technically
valid flat provider response alone is not enough, and a checked research box is
not production acceptance.

The tested credit and task responses do not expose the account plan class. The
pipeline obtains that evidence through an account API, billing export, or
authenticated browser automation and records the source hash and date. If it
cannot obtain sufficient entitlement evidence, it quarantines the provider
output and selects a licensed fallback; it does not wait for human input. A live
tool may compare preflight and post-task balances, but it should retain only the
observed per-task delta, never the full account balances or raw credit responses.

## Rights, originality, and provider risk

MusicAPI.ai's terms state that the user retains ownership and commercial-use
rights for generated songs, while also assigning the user responsibility for
originality, non-infringement, and third-party claims. Treat those statements as
provider policy, not as a guarantee that a particular output is copyrightable,
exclusive, non-infringing, or safe to ship.

Before a generation batch and again before production acceptance, the pipeline:

- record the account plan class and verify that its terms permit the intended
  commercial game use;
- retains a dated, hashed terms snapshot according to repository evidence
  policy;
- verifies that prompts and uploaded inputs, if separately enabled by the batch
  contract, are original and authorized;
- runs acoustic similarity and unwanted-vocal validators on every retained
  output;
- records deterministic selection, arrangement, editing, looping, mixing, and
  mastering recipes; and
- applies the repository rights policy and quarantines any output whose license
  or jurisdiction evidence is insufficient.

Provider claims, generated metadata, lack of an automated match, or payment for
credits do not replace the repository's provenance and originality checks.

## Preflight and completion checklist

Before a live request:

- [ ] The exact package, candidate suffix, brief, model, request count, and
      maximum credit spend match the versioned batch configuration.
- [ ] The official create, task, credit, model, rate, output, and terms pages
      have been rechecked.
- [ ] `gpt_description_prompt` is present and no longer than 400 characters.
- [ ] `.env` is ignored, the raw token is present, and no secret appears in the
      request body, command arguments, source, logs, or evidence.
- [ ] The prompt contains only original Skyglass direction and excludes archive,
      artist, franchise, song, vocal, and personal-data references.
- [ ] The unique scratch directory, atomic `.part` handling, cancellation, and
      resume behavior are ready.

Before retaining a technically validated research candidate:

- [ ] The task succeeded and the selected clip has a final downloadable URL.
- [ ] Instrumental lyric metadata is empty or bracket-only with no lexical
      words; the acoustic suite still verifies that no voice is audible.
- [ ] Original bytes pass bounded download, full decode, metadata, waveform,
      SHA-256, and byte-count checks.
- [ ] Vocal metadata, transcription, waveform, similarity, originality, and mix
      validators record passed or rejected outcomes; no check remains pending
      on a retained candidate.
- [ ] Prompt, parameters, model, task, clip, plan-class evidence state, terms
      date, original hash, measurements, and disposition are recorded without
      secrets; missing plan evidence quarantines the provider output.
- [ ] The exact bytes and safe provenance are moved to the unique
      `skyglass-assets/music/` path and the research manifest is updated.

A candidate with an incomplete autonomous check remains research-only. It may
close an explicitly defined technical-inventory box, but it cannot enter the
production manifest.

Before marking an adaptive package production-accepted:

- [ ] Base, movement, threat, peak, transition, and intentional-silence roles
      are fully resolved.
- [ ] All layers are instrumental, vocally clean, synchronized, independently
      mixable, loop-tested, and provenance-accepted.
- [ ] Direction transitions, crossfades, mute, pause, visibility, restart,
      recovery, completion, death, and audio-off equivalence pass in-engine.
- [ ] Transfer, decoded memory, concurrency, and offline-package budgets pass.
- [ ] Production manifest, rights, originality, acoustic, and in-engine checks
      pass with machine-readable evidence.
