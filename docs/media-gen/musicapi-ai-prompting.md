# MusicAPI.ai Sonic prompting and request-variable guide

<!-- cspell:words audiopipe autochorus autoformat autofill autotitle backcompat CDN HMAC infill lyrics metatag metatags MusicAPI neosoul nonterminal Nuro overpainting prechorus Riffusion singback Suno synthwave webhook webhooks -->

Status: Proposed

Audience: MusicAPI.ai integrators, prompt authors, audio producers, and quality
automation maintainers

Scope: Provider-focused guidance for every currently documented request-body
variable on `POST /api/v1/sonic/create`, plus the prompt-bearing adjacent Sonic
endpoints; excludes application-specific creative direction, runtime integration,
account setup, and the separate Producer, Nuro, Studio, and deprecated Riffusion
models

Authority: Evidence

Last verified: 2026-07-15

Evidence: [Sonic instructions](https://docs.musicapi.ai/sonic-instructions),
[unified Sonic create endpoint](https://docs.musicapi.ai/concat-music),
[Sonic task results](https://docs.musicapi.ai/get-sonic-music),
[MusicAPI.ai FAQ](https://docs.musicapi.ai/faq),
[webhook guide](https://docs.musicapi.ai/webhook-guide),
[credit guide](https://docs.musicapi.ai/credits-usage-guide),
[error guide](https://docs.musicapi.ai/error-handling),
[uploaded-audio scenarios](https://docs.musicapi.ai/special-scenarios), and the
live validation record described below, checked 2026-07-15

## Purpose and evidence labels

This is a MusicAPI.ai Sonic API guide, not a guide to the Suno website. It does
not invent UI-only fields such as "Audience," "Subject," "My Taste," or "Style
of Music." It covers the JSON fields that MusicAPI.ai currently accepts and
shows where genre, mood, instrumentation, lyrics, exclusions, and arrangement
instructions actually belong.

The API and its documentation can change without a versioned schema release.
Interpret claims in this document using these labels:

- **Documented** means the current official MusicAPI.ai pages or OpenAPI export
  state the behavior.
- **Live-validated** means a 2026-07-15 request confirmed validation or response
  behavior against `sonic-v5-5`.
- **Observed** means a live response exhibited the behavior, but the test did
  not prove why the model produced it.
- **Heuristic** means a useful prompting practice, not an API guarantee.
- **Inconsistent** means current official sources disagree or live behavior
  contradicts the published schema.

Do not turn a heuristic or one generated song into a causal claim. Sonic exposes
no seed, and each create request normally returns two stochastic variants.

## Choose the generation mode first

The meaning of `prompt` changes with the mode. Select the mode before writing
any content:

| Goal                                 | Required mode fields                                                                |
| ------------------------------------ | ----------------------------------------------------------------------------------- |
| Supply exact lyrics                  | `custom_mode: true`, `prompt: "...lyrics..."`                                       |
| Describe a song and let AI write it  | `custom_mode: false`, `gpt_description_prompt: "...description..."`                 |
| Use `prompt` as an auto-lyrics brief | `custom_mode: true`, `auto_lyrics: true`, `prompt: "...song description..."`        |
| Generate an instrumental             | Usually description mode plus `make_instrumental: true`                             |
| Extend a platform clip               | `task_type: "extend_music"`, `continue_clip_id`, `continue_at`, and one prompt mode |
| Cover a platform clip                | `task_type: "cover_music"`, `continue_clip_id`, and one prompt mode                 |
| Use a persona                        | `task_type: "persona_music"`, `persona_id`, and one prompt mode                     |
| Join an extension to its source      | `task_type: "concat_music"`, `continue_clip_id`                                     |

The safest create request always sends an explicit `task_type`, even though
omitting it live-defaulted to `create_music`. Explicit intent is easier to
validate, audit, and migrate.

Concepts sometimes presented as separate music-prompt categories map to the API
like this:

| Creative concept                        | MusicAPI.ai field                                                        |
| --------------------------------------- | ------------------------------------------------------------------------ |
| Genre, mood, instruments, vocal style   | `tags`                                                                   |
| Tempo feel, era, texture, production    | `tags`, with important arrangement context in the active prompt field    |
| Subject, story, setting, audience, use  | `gpt_description_prompt`, auto-lyrics `prompt`, or the lyrics themselves |
| Exact words and section order           | Ordinary custom-mode `prompt`                                            |
| Unwanted styles, instruments, or vocals | `negative_tags`                                                          |
| Track name                              | `title`                                                                  |

## Endpoint, cost, and asynchronous result

```text
POST https://api.musicapi.ai/api/v1/sonic/create
Authorization: Bearer <raw API key>
Content-Type: application/json
```

The documented cost is 15 credits for create, extend, cover, persona, remaster,
add-instrumental, or add-vocals work and 2 credits for `concat_music`. A normal
create task returns two song variants. Recheck the
[credit guide](https://docs.musicapi.ai/credits-usage-guide) before spending.

The submit call returns a `task_id`; poll
`GET /api/v1/sonic/task/{task_id}` every 15 to 25 seconds until every returned
clip is terminal. Store the accepted task ID before polling. A lost poll or
download is not a reason to submit and pay for a duplicate task.

Live submit responses contained only `message` and `task_id`, even though the
OpenAPI success schema requires `code`. Treat `code` as optional at submission.
Live task responses first used a `not_ready` object, then a `data` array with
clip states, and included numeric `code` only in the later response shape.

## Complete `/sonic/create` variable inventory

The current OpenAPI export lists 23 request-body properties. The tables below
cover every one.

### Operation and mode fields

| Field          | Type    | Required or default                                           | Meaning and cautions                                                                            |
| -------------- | ------- | ------------------------------------------------------------- | ----------------------------------------------------------------------------------------------- |
| `task_type`    | string  | Defaults to `create_music`; live-validated                    | Selects create, edit, source, or utility behavior. Prefer sending it explicitly.                |
| `custom_mode`  | boolean | Conditional; required for prompt-bearing generation           | `true` selects custom lyrics, unless `auto_lyrics` changes `prompt` into a description.         |
| `mv`           | string  | Required except for `concat_music`                            | Sonic model version. Validate against the current endpoint enum, not an old guide or UI label.  |
| `use_suno_cdn` | boolean | Documented as optional, but the schema also marks it required | Chooses a requested delivery host only. Live behavior did not honor the documented distinction. |

### Creative-content fields

| Field                    | Type    | Documented range or limit                   | Meaning                                                                                                  |
| ------------------------ | ------- | ------------------------------------------- | -------------------------------------------------------------------------------------------------------- |
| `prompt`                 | string  | 3,000 characters on v3.5/v4; 5,000 on v4.5+ | Exact lyrics in custom mode; a song description when `auto_lyrics: true`; lyrics for `add_vocals`.       |
| `gpt_description_prompt` | string  | 400 characters                              | Natural-language song brief when `custom_mode: false`.                                                   |
| `title`                  | string  | 80 characters                               | Caller-supplied track title. Behavior differs when the provider is allowed to generate metadata.         |
| `tags`                   | string  | 200 characters on v3.5/v4; 1,000 on v4.5+   | Style conditioning: genre, mood, instruments, vocal character, tempo feel, era, and production texture.  |
| `negative_tags`          | string  | No current maximum is published             | Comma-separated styles or elements to avoid. It is conditioning, not a hard filter.                      |
| `make_instrumental`      | boolean | Optional                                    | Requests a vocal-free result. Run waveform classification; metadata alone cannot prove that no voice is audible. |
| `auto_lyrics`            | boolean | Optional; requires `custom_mode: true`      | Makes `prompt` a brief from which AI generates lyrics and, when omitted, title and tags.                 |
| `vocal_gender`           | string  | `f` or `m`                                  | Vocal-gender request for supported models. No neutral or additional enum is documented.                  |
| `style_weight`           | number  | 0 through 1                                 | Higher values request stronger adherence to `tags`. Provider default is not documented.                  |
| `weirdness_constraint`   | number  | 0 through 1                                 | Higher values request more unusual or experimental output. Provider default is not documented.           |

### Source and edit fields

| Field                  | Type   | Applies to                                  | Meaning and cautions                                                                                    |
| ---------------------- | ------ | ------------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| `continue_clip_id`     | string | Extend, cover, concat, remaster, add tasks  | Source clip. Use the task type appropriate to a generated clip versus uploaded audio.                   |
| `continue_at`          | number | Extend                                      | Source timestamp in seconds where extension begins. Validate it against the actual source duration.     |
| `persona_id`           | string | `persona_music`                             | Persona created by the persona endpoint. Use only voices and source audio for which use is authorized.  |
| `audio_weight`         | number | Cover; also mentioned for add tasks/uploads | 0-1 source influence or blend. Current official descriptions disagree on exactly which tasks accept it. |
| `variation_category`   | string | `remaster` with `sonic-v5`                  | `subtle`, `normal`, or `high`. The current page does not promise this for `sonic-v5-5`.                 |
| `overpainting_start_s` | number | `add_instrumental`, `add_vocals`            | Optional start of the affected time range in seconds. Locally require a nonnegative value.              |
| `overpainting_end_s`   | number | `add_instrumental`, `add_vocals`            | Optional end of the affected range. Locally require it to exceed the nonnegative start.                 |

### Delivery fields

| Field            | Type   | Requirement | Meaning and cautions                                                                                |
| ---------------- | ------ | ----------- | --------------------------------------------------------------------------------------------------- |
| `webhook_url`    | string | Optional    | HTTPS callback for task events. The receiver must handle retries, duplicates, and terminal failure. |
| `webhook_secret` | string | Optional    | HMAC secret used to verify callbacks. It is a credential and must never be logged or committed.     |

There are no Sonic-create fields named `audience`, `subject`, `mood`, `genre`,
`bpm`, `key`, `duration`, `seed`, `lyrics_strength`, or `audio_influence`.
Encode creative concepts in the supported description, lyrics, or tag strings.
Use `audio_weight` for the documented source-influence cases. Do not send UI
labels as invented JSON fields.

## Task types and conditional requirements

Current official pages do not expose one perfectly consistent task-type enum.
The endpoint schema lists eight values; the Sonic instructions separately list
two uploaded-audio variants.

| `task_type`           | Purpose                                       | Required core fields                                   | Current evidence                                              |
| --------------------- | --------------------------------------------- | ------------------------------------------------------ | ------------------------------------------------------------- |
| `create_music`        | Create a new song                             | `custom_mode`, `mv`, and the matching prompt field     | Endpoint schema and live-validated                            |
| `extend_music`        | Extend a platform-generated clip              | `continue_clip_id`, `continue_at`, `custom_mode`, `mv` | Endpoint schema                                               |
| `cover_music`         | Re-style a platform-generated clip            | `continue_clip_id`, `custom_mode`, `mv`                | Endpoint schema                                               |
| `concat_music`        | Join an extension to its source               | `continue_clip_id`                                     | Endpoint schema; 2 credits                                    |
| `persona_music`       | Generate with an existing persona             | `persona_id`, `custom_mode`, `mv`                      | Endpoint schema                                               |
| `remaster`            | Upgrade or vary an existing clip              | `continue_clip_id`, `mv`                               | Endpoint schema; variation enum only documented for v5        |
| `add_instrumental`    | Add accompaniment to uploaded audio           | `continue_clip_id`, `mv`                               | Endpoint schema; official narrative says uploaded clips only  |
| `add_vocals`          | Add generated vocals to uploaded instrumental | `continue_clip_id`, `mv`, `prompt`                     | Endpoint schema; official narrative says uploaded clips only  |
| `extend_upload_music` | Extend an uploaded clip                       | `continue_clip_id`, `continue_at`, `custom_mode`, `mv` | Sonic instructions, but absent from the current endpoint enum |
| `cover_upload_music`  | Cover an uploaded clip                        | `continue_clip_id`, `custom_mode`, `mv`                | Sonic instructions, but absent from the current endpoint enum |

For new uploaded-audio integrations, prefer the separately documented
`/api/v1/sonic/upload-extend` or `/api/v1/sonic/upload-cover` endpoint. They
accept a source `url`, perform the upload step, and avoid guessing which unified
endpoint enum is deployed. Never upload audio that the caller does not own or
have permission to transform.

## Model versions and effective limits

| Model             | `prompt` maximum | `tags` maximum | `vocal_gender` status                                       |
| ----------------- | ---------------: | -------------: | ----------------------------------------------------------- |
| `sonic-v3-5`      |            3,000 |            200 | Not documented                                              |
| `sonic-v4`        |            3,000 |            200 | Not documented                                              |
| `sonic-v4-5`      |            5,000 |          1,000 | Documented                                                  |
| `sonic-v4-5-plus` |            5,000 |          1,000 | Documented                                                  |
| `sonic-v5`        |            5,000 |          1,000 | Documented                                                  |
| `sonic-v5-5`      |            5,000 |          1,000 | Documented in instructions; request acceptance live-tested  |
| `sonic-v4-5-all`  |              N/A |            N/A | Do not use: listed on one page but live-rejected as invalid |

The v5.5 tag limit is omitted from one current documentation table, but a live
1,001-character v5.5 tag string was rejected with a 1,000-character limit
message. The title limit is 80 across modes, and the description-mode limit is
400 across the currently documented models.

Sonic has no exact-duration request field. The FAQ describes model-dependent
duration tendencies and recommends shorter lyrics, fewer sections, ending tags,
or extension, but those are indirect controls. Do not promise an exact runtime
from the prompt.

## Description mode

Use description mode when the model should write the lyrics and composition:

```json
{
  "task_type": "create_music",
  "custom_mode": false,
  "mv": "sonic-v5-5",
  "gpt_description_prompt": "Warm nocturnal neo-soul about choosing patience over urgency; brushed drums, electric piano, restrained lead vocal, intimate verses, open chorus, concise bridge, and a resolved ending.",
  "make_instrumental": false,
  "use_suno_cdn": false
}
```

### Write a high-information 400-character brief

Use this order as a heuristic:

1. primary genre or hybrid;
2. emotional direction and subject;
3. instrumentation and vocal character;
4. tempo feel and groove;
5. arrangement arc or section behavior;
6. production texture and ending behavior.

Prefer concrete musical descriptions over marketing language. "Brushed drums,
warm electric piano, intimate verses, wider chorus" gives the model more usable
information than "an amazing professional hit." Avoid artist, producer,
franchise, or copyrighted-song names; the provider documents moderation errors
for artist names, producer tags, and copyrighted lyrics.

Do not repeat a long exclusion list inside the description when
`negative_tags` can express it separately. Description mode may generate or
normalize metadata. A live description-mode request returned a shortened title
rather than preserving the complete caller suffix.

## Custom lyrics mode

In ordinary custom mode, `prompt` means the words and structural markers to be
performed:

```json
{
  "task_type": "create_music",
  "custom_mode": true,
  "mv": "sonic-v5-5",
  "title": "Paper Constellations",
  "tags": "indie soul, intimate, warm electric piano, brushed drums, restrained vocal",
  "negative_tags": "metal, choir, spoken word, aggressive drums",
  "prompt": "[Intro]\nSoft keys and brushed drums\n\n[Verse]\nStreetlights settle on the rain\nWe choose a slower road again\n\n[Pre-Chorus]\nLeave the hurried hours behind\n\n[Chorus]\nHold the quiet, hold the line\nLet the patient rhythm shine\n\n[Bridge]\nWe find the words we meant to say\n\n[Outro]\nThe final chord resolves",
  "vocal_gender": "f",
  "style_weight": 0.8,
  "weirdness_constraint": 0.25,
  "use_suno_cdn": false
}
```

### Structure tags

The core Sonic instructions explicitly document:

- `[Intro]`
- `[Verse]`
- `[Pre-Chorus]`
- `[Chorus]`
- `[Hook]`
- `[Break]`
- `[Bridge]`
- `[Outro]`

The FAQ also recommends `[End]`, `[Fade Out]`, `[Short Instrumental Outro]`,
`[Instrumental]`, and `[Guitar Solo]` as indirect arrangement or length cues.
Live auto-lyrics responses used `[Verse 1]` and `[Verse 2]`, confirming numbered
section labels in provider-authored output.

Bracketed directions remain soft conditioning. Parameterized tags such as
`[Bridge: stripped back, electric piano only]` are an upstream prompting
heuristic, not a formally enumerated MusicAPI.ai syntax. They can be useful, but
the model may ignore them, reinterpret them, or sing unfamiliar text aloud.

### Lyrics formatting

- Put one section marker on its own line.
- Separate sections with a blank line.
- Keep performance directions short and musical.
- Write only lyrics that the caller owns or is authorized to use.
- Do not depend on timestamps inside lyrics for clock-accurate placement.
- Do not claim exact BPM, key, effect percentage, or frequency control from a
  lyric metatag.
- Use `continue_at` or overpainting fields for supported time-based edits rather
  than embedding clock ranges in lyrics.

There is no evidence for a required lyric line count. Fewer sections generally
produce less material, but the generated duration remains stochastic.

## Auto-lyrics mode

Auto lyrics is a special case: `custom_mode` remains `true`, but
`auto_lyrics: true` changes `prompt` from exact lyrics into a song brief.

Let the provider generate lyrics, title, and tags:

```json
{
  "task_type": "create_music",
  "custom_mode": true,
  "auto_lyrics": true,
  "mv": "sonic-v5-5",
  "prompt": "An upbeat acoustic soul song about finding a forgotten letter, with a concise verse-chorus-bridge arc.",
  "negative_tags": "metal, choir, spoken word, aggressive drums",
  "style_weight": 0.75,
  "weirdness_constraint": 0.25,
  "use_suno_cdn": false
}
```

Live behavior resolved an ambiguity in the official example:

- when `title` and `tags` were supplied, both returned variants preserved them
  exactly while AI generated the lyrics;
- when both fields were omitted, both variants received the same generated
  title, long generated style description, and generated lyrics; and
- therefore, "AI generates title and tags" means it fills missing fields rather
  than necessarily overriding caller-supplied values.

Do not use auto lyrics when exact words, legal-policy verification,
pronunciation, or section text must remain unchanged. Run generated lyrics
through originality, appropriateness, pronunciation, and unwanted-reference
validators before use.

## Instrumental mode

The clearest instrumental request uses description mode:

```json
{
  "task_type": "create_music",
  "custom_mode": false,
  "mv": "sonic-v5-5",
  "make_instrumental": true,
  "gpt_description_prompt": "Spacious ambient jazz with felt piano, brushed cymbals, upright bass, gradual harmonic motion, and a clean resolved ending; no vocal role.",
  "tags": "ambient jazz, felt piano, brushed cymbals, upright bass, spacious",
  "negative_tags": "vocals, lyrics, spoken word, choir, vocal chops",
  "use_suno_cdn": false
}
```

`make_instrumental: true` is stronger and clearer than relying on an
`[Instrumental]` lyric tag for the whole track. It is still a model request, not
a proof. A live instrumental response returned a bracket-only instrumental
marker in `lyrics` rather than an empty string. Always scan for singing, speech,
chants, vocal chops, or lyric-like artifacts with the autonomous waveform and
transcription classifiers.

## Titles

`title` is a string with a documented 80-character maximum. An 81-character
live probe was rejected with HTTP 400 without consuming credits.

Treat the title as both caller metadata and possible model context; the provider
does not document it as "cosmetic only." Live behavior varied by mode:

- custom lyrics preserved the exact supplied title in both variants;
- auto lyrics preserved a supplied title in both variants;
- auto lyrics generated a title when it was omitted; and
- description mode previously normalized a longer caller title.

Use a concise, content-appropriate title. Keep internal IDs, filenames, version
numbers, and acceptance state outside the creative title. After polling, store the
returned title separately from the submitted title so a provider rewrite does
not destroy provenance.

## Tags

`tags` is one string, not an array. The official guide describes
comma-separated values and recognizes these dimensions:

| Dimension           | Examples                                                       |
| ------------------- | -------------------------------------------------------------- |
| Genre or hybrid     | `neo-soul`, `ambient jazz`, `synthwave`, `orchestral folk`     |
| Mood                | `uplifting`, `melancholic`, `dreamy`, `aggressive`, `peaceful` |
| Instruments         | `felt piano`, `electric guitar`, `saxophone`, `analog synth`   |
| Vocal character     | `female vocal`, `falsetto`, `harmonies`, `spoken word`         |
| Tempo feel          | `slow`, `mid-tempo`, `fast`, `downtempo`, `upbeat`             |
| Era or production   | `80s`, `vintage`, `modern`, `futuristic`, `cinematic`, `lo-fi` |
| Texture or movement | `brushed drums`, `wide chorus`, `dry verses`, `gradual build`  |

A controlled tag string usually names one primary genre, one compatible
secondary influence, mood, key instrumentation, vocal character, tempo feel,
and one or two production traits:

```text
neo-soul, nocturnal, warm electric piano, brushed drums, restrained female vocal, mid-tempo
```

The API does not document positional weighting. Put the most important tags
first for readability and safe truncation, not because front-loading is proven
to increase model weight. Avoid long lists of competing genres, mutually
exclusive tempos, or incompatible vocal instructions.

Current upstream guidance says newer models can interpret conversational style
descriptions, and a live auto-lyrics response generated a long prose-like tag
string. That does not make prose mandatory. Compact comma-separated tags remain
easier to diff, validate, and vary in controlled experiments.

## Negative tags

`negative_tags` is a comma-separated string of unwanted styles or elements:

```text
metal, distorted guitar, choir, spoken word, aggressive drums
```

Use positive `tags` for what should lead the arrangement and `negative_tags`
for exclusions. Do not write "no drums" inside the positive tag string. The
provider publishes no current maximum for `negative_tags`; apply a conservative
local bound and keep the list shorter than the positive direction.

Negative tags are not a deterministic content filter. Live responses echoed
the submitted negative string, but the experiments did not acoustically score
the resulting audio, so they do not prove that every exclusion was obeyed.
The autonomous acoustic validation suite remains mandatory.

## Style weight and weirdness

Both controls accept numeric values from 0 through 1. Live `1.01` and `-0.01`
probes were rejected with HTTP 400, while valid v5.5 values were accepted and
echoed exactly in both terminal clip records.

The provider does not publish defaults or a quantitative mapping from either
number to musical outcomes. These are heuristic starting regions, not defaults:

| Goal                       | `style_weight` | `weirdness_constraint` |
| -------------------------- | -------------: | ---------------------: |
| Tight, conservative brief  |       0.75-0.9 |                0.1-0.3 |
| Balanced exploration       |       0.55-0.8 |               0.25-0.5 |
| Deliberately unusual takes |       0.35-0.7 |                0.6-0.9 |

To compare them responsibly:

1. keep model, lyrics or description, title, tags, negative tags, and vocal
   fields unchanged;
2. change only one weight;
3. generate multiple paid batches because there is no Sonic seed;
4. blind-score both variants with the versioned acoustic evaluator for the
   intended musical traits; and
5. record failures as well as preferred outcomes.

One pair at each setting cannot separate the variable from random generation.

## Vocal gender and personas

`vocal_gender` accepts only `f` or `m`. Official descriptions disagree on the
complete supported model list, but a v5.5 custom request with `f` and a v5.5
auto-lyrics request with `m` were both accepted. The response does not echo a
vocal-gender field, and the research did not perform acoustic classification,
so acceptance does not prove audible compliance.

`persona_id` is a separate source control for `persona_music`. Create it through
the persona endpoint from authorized source audio. Persona creation has its own
fields: `name`, `clip_id`, optional `describe`, optional `styles`, and optional
VOX/time-range controls. These are not `/sonic/create` fields. Treat voice
identity, consent, retention, and reuse as high-risk rights constraints.

## Extension, cover, remaster, and add operations

### Extend

```json
{
  "task_type": "extend_music",
  "continue_clip_id": "source-clip-id",
  "continue_at": 120,
  "custom_mode": true,
  "mv": "sonic-v5-5",
  "prompt": "[Bridge]\nNew original lyrics\n\n[Outro]\nA final resolving line",
  "tags": "match the source style"
}
```

`continue_at` is seconds, not a lyric timestamp. Keep it within the source clip
and choose a musically sensible boundary. Use `concat_music` on the returned
extension clip when a full source-plus-extension file is required.

### Cover

```json
{
  "task_type": "cover_music",
  "continue_clip_id": "source-clip-id",
  "custom_mode": false,
  "mv": "sonic-v5-5",
  "gpt_description_prompt": "Reinterpret as sparse acoustic jazz with brushed drums, upright bass, and intimate vocals.",
  "audio_weight": 0.6
}
```

Higher `audio_weight` is documented as staying closer to the original on a
cover. The API cannot grant rights to transform source material; authorization
must exist before the request.

### Remaster

`remaster` requires `continue_clip_id` and `mv`. The current endpoint page says
`variation_category` accepts `subtle`, `normal`, or `high` only with
`sonic-v5`. Do not assume the field works with v5.5 merely because v5.5 is newer.

### Add instrumental or vocals

The current endpoint page states that `add_instrumental` and `add_vocals` work
only with audio uploaded by the caller. They may use `audio_weight` as a blend
control and optional `overpainting_start_s`/`overpainting_end_s` to bound a
region. `add_vocals` requires lyrics in `prompt`. Official schema prose is
inconsistent about `audio_weight`, so validate the exact deployed task before
building a batch workflow.

## Webhooks and CDN selection

When `webhook_url` is present, MusicAPI.ai documents POST callbacks for
streaming, success, and failure events. If `webhook_secret` is supplied, verify:

```text
X-Webhook-Signature = HMAC-SHA256(secret, timestamp + "." + rawBody)
```

Use the exact raw request body before JSON parsing, check
`X-Webhook-Timestamp` within a short replay window, compare signatures in
constant time, and deduplicate using the event or idempotency header plus
`task_id`. Return 2xx only after durable acceptance. Never expose the webhook
secret in client code, logs, examples, or stored payloads.

The current OpenAPI description says `use_suno_cdn: true` selects Suno CDN URLs
and false or omission selects MusicAPI.ai delivery URLs with identical bytes.
Live behavior contradicted that claim:

- an earlier omitted-field request returned `cdn1.suno.ai` audio;
- a paid `false` custom request returned `cdn1.suno.ai` audio and
  `cdn2.suno.ai` images;
- a paid `true` auto-lyrics request returned the same host pattern; and
- a second paid `false` request again returned the Suno hosts.

Treat `use_suno_cdn` as accepted but operationally unreliable until the provider
fixes or clarifies it. Do not depend on `false` for data residency, privacy,
firewall, or hostname policy. Validate the actual HTTPS URL and downloaded bytes
from every result.

## Response fields that affect prompting workflows

Each terminal clip currently exposes these fields:

| Field                    | Handling                                                                                       |
| ------------------------ | ---------------------------------------------------------------------------------------------- |
| `clip_id`                | Stable provider reference for follow-up operations; do not derive meaning from it.             |
| `state`                  | Accept `succeeded` or `failed` as terminal; wait while other variants remain nonterminal.      |
| `title`                  | Compare with the submitted title and retain both when provenance matters.                      |
| `tags`                   | May echo caller tags or contain provider-generated prose in auto-lyrics mode.                  |
| `lyrics`                 | Exact custom lyrics, generated lyrics, or bracket-only instrumental metadata.                  |
| `negative_tags`          | Live responses echoed the submitted value; absence or echo does not prove acoustic compliance. |
| `style_weight`           | Live responses echoed valid submitted values.                                                  |
| `weirdness_constraint`   | Live responses echoed valid submitted values.                                                  |
| `gpt_description_prompt` | `null` in the live custom and auto-lyrics tasks.                                               |
| `mv`                     | Record the returned model and compare it with the request.                                     |
| `duration`               | Live v5.5 responses used decimal strings; parse and validate rather than assuming a number.    |
| `audio_url`, `image_url` | Untrusted remote URLs; validate scheme, host policy, redirects, MIME, size, and bytes.         |
| `video_url`              | `null` in all live tests; do not require video for audio success.                              |

Do not declare a two-variant create task complete when only one clip has
succeeded. The final auto-lyrics test spent several minutes in `not_ready`, then
returned two running clips, then one succeeded clip, and only later both
succeeded. Poll the accepted task instead of resubmitting.

## 2026-07-15 live validation evidence

### Zero-credit boundary probes

Eight requests were intentionally invalid. All returned HTTP 400, no `task_id`,
and an observed total credit delta of zero:

| Probe                                  | Live result                                                |
| -------------------------------------- | ---------------------------------------------------------- |
| 81-character `title`                   | Rejected for title length                                  |
| 401-character `gpt_description_prompt` | Rejected for description length                            |
| 5,001-character v5.5 `prompt`          | Rejected with a 5,000-character model limit                |
| 1,001-character v5.5 `tags`            | Rejected with a 1,000-character model limit                |
| `style_weight: 1.01`                   | Rejected; must be between 0 and 1                          |
| `weirdness_constraint: -0.01`          | Rejected; must be between 0 and 1                          |
| String `use_suno_cdn: "false"`         | Rejected; must be boolean                                  |
| `mv: "sonic-v4-5-all"`                 | Rejected as invalid; accepted list ended with `sonic-v5-5` |

These probes verify rejection just beyond the documented maxima. They did not
submit exact-boundary values, so the official inclusive maxima remain the
contract rather than a separate live boundary claim.

### Paid behavior tests

Three paid create tasks produced six succeeded v5.5 clips. The documented and
observed total debit was 45 credits, below the authorized 1,000-credit research
ceiling. No fourth research task was submitted.

| Test                                       | Key observed result                                                                                              |
| ------------------------------------------ | ---------------------------------------------------------------------------------------------------------------- |
| Custom lyrics with omitted `task_type`     | Defaulted to create; exact title, tags, negative tags, weights, lyrics, and section tags returned for both clips |
| Auto lyrics with supplied title and tags   | Generated lyrics but preserved the supplied title and tags for both clips                                        |
| Auto lyrics with title and tags omitted    | Generated a title, detailed tags, and six-section lyrics shared by both clips                                    |
| `use_suno_cdn: false`, `true`, and omitted | All observed final audio URLs used `cdn1.suno.ai`                                                                |
| Valid v5.5 style and weirdness controls    | Echoed exactly in terminal clip data                                                                             |
| v5.5 `vocal_gender` requests               | Accepted, but audible compliance was not evaluated                                                               |
| Submit response                            | `message` plus `task_id`; no `code`                                                                              |
| Terminal duration                          | Decimal string on all six clips                                                                                  |

The experiment did not download or acoustically score these six research
outputs.
Therefore it supports request, validation, metadata, cost, and lifecycle claims
only. It does not prove tag strength, negative-tag compliance, vocal gender,
musical quality, originality, or similarity.

This bounded prompt-field experiment is separate from the later instrumental
Skyglass completion batch. The completion batch's source, stem, failure, refund,
and packaging evidence is recorded in the [project MusicAPI.ai
guide](musicapi-ai.md#live-batch-reliability-and-refund-accounting) and [music
provenance](../../../skyglass-assets/music/musicapi-ai-provenance.json).

## Prompting practices supported by current evidence

### Reliable practices

- Choose the correct mode before writing `prompt`.
- Validate every string length and numeric range locally.
- Use clear genre, mood, instrumentation, vocal, tempo-feel, and arrangement
  language.
- Put exact lyrics only in ordinary custom mode.
- Use short, recognized section labels and blank lines between sections.
- Keep wanted and unwanted traits in `tags` and `negative_tags` respectively.
- Record all submitted values and returned metadata because the provider may
  normalize or generate title, tags, and lyrics.
- Generate multiple variants and score them through the autonomous suite; no
  prompt guarantees compliance.

### Unsupported or overstated practices to avoid

- Do not claim that the title is cosmetic or never influences generation.
- Do not claim that the first tag has a documented weighting advantage.
- Do not invent 250-character limits for mood, subject, or audience categories;
  those are not API fields.
- Do not claim a required number of lyric lines.
- Do not treat metatags as deterministic commands or exact timestamps.
- Do not promise exact BPM, key, duration, mix percentage, or effect settings
  from prose.
- Do not use artist names as shorthand for style; describe musical traits.
- Do not equate `make_instrumental`, `negative_tags`, or returned lyrics metadata
  with proof that no vocal sound exists.
- Do not assume `use_suno_cdn: false` controls the returned hostname.
- Do not blindly retry an accepted task because polling is slow.

## Controlled iteration workflow

1. Pick one mode and model.
2. Write a valid minimal request and save the exact JSON without credentials.
3. Generate one paid batch and retain both variants.
4. Score the same audible traits for both clips: genre adherence, mood,
   instrumentation, lyric accuracy, vocal character, exclusions, arrangement,
   ending, artifacts, and originality concerns.
5. Change one field or one phrase only.
6. Generate enough repeated batches to distinguish the change from stochastic
   variation.
7. Keep rejected outputs and reasons in the evaluation record.
8. Promote only files that pass the autonomous acceptance suite; a successful
   API task is not production acceptance.

For title or metadata tests, compare exact returned strings. For subjective
controls such as style weight and weirdness, use blinded acoustic-model scoring
and more than one batch per setting.

## Adjacent prompt-bearing Sonic endpoints

These endpoints have their own schemas. Their unique variables must not be sent
to `/sonic/create` unless that endpoint also documents them:

| Endpoint                        | Unique or additional variables                                                                  |
| ------------------------------- | ----------------------------------------------------------------------------------------------- |
| `/api/v1/sonic/upload-extend`   | `url`, `continue_at`, `auto_concat`, plus mode, prompt, metadata, weights, and webhook fields   |
| `/api/v1/sonic/upload-cover`    | `url`, `audio_weight`, plus mode, prompt, metadata, weights, and webhook fields                 |
| `/api/v1/sonic/sample`          | `url` or `sample_clip_id`, `chop_sample_start_s`, `chop_sample_end_s`, and generation fields    |
| `/api/v1/sonic/replace-section` | `clip_id`, `infill_start_s`, `infill_end_s`, original `prompt`, and `infill_lyrics`             |
| `/api/v1/sonic/persona`         | `name`, `clip_id`, `describe`, `styles`, optional `vox_audio_id`, and matching vocal time range |
| `/api/v1/sonic/upsample-tags`   | `tags`; returns an expanded `upsampled_tags` string and costs credits                           |
| Lyrics generation endpoint      | `description`; returns candidate titles and lyrics                                              |

The official navigation and some endpoint exports currently contain naming or
schema mismatches. Read the target endpoint's current OpenAPI export immediately
before integrating it; do not copy variables from a neighboring endpoint.

## Preflight checklist

- [ ] The endpoint, model, task type, cost, and output count were rechecked in
      current official documentation.
- [ ] The request uses exactly one prompt mode and the correct conditional
      fields.
- [ ] `title`, `prompt`, `gpt_description_prompt`, and `tags` fit the selected
      model limits.
- [ ] Numeric controls are finite numbers from 0 through 1 where required.
- [ ] Source clip, time range, uploaded-audio ownership, persona consent, and
      task-type compatibility are verified.
- [ ] Artist names, copyrighted lyrics, producer tags, personal data, and
      unauthorized source material are absent.
- [ ] The maximum paid task count and credit ceiling are explicit.
- [ ] An accepted `task_id` is persisted before polling or webhook handling.
- [ ] Webhook secrets and API credentials remain outside request logs and
      committed files.
- [ ] Both returned variants will be scored by the autonomous acceptance suite;
      no metadata field is treated as acoustic or rights evidence.

## Source interpretation notes

The current endpoint OpenAPI export is the best inventory of request properties,
but it is not internally perfect:

- its required array includes `use_suno_cdn` while its own description says the
  field is optional, and live requests succeed without it;
- it omits uploaded-audio task types documented on the Sonic instructions page;
- the Sonic instructions list `sonic-v4-5-all`, while the live endpoint rejects
  it;
- one tag-limit table omits v5.5, while live v5.5 validation enforces 1,000;
- the success schema requires `code`, while three paid submits and an earlier
  paid submit omitted it; and
- the CDN-selector description does not match observed hosts.

Older generic error pages also retain superseded 200-character description and
older model messages. Prefer the current endpoint export, current Sonic
instructions, and a zero-credit validation probe over a stale example. When
sources still conflict, document the uncertainty instead of guessing.

Upstream Suno material can help with qualitative prompt vocabulary, particularly
[detailed style instructions](https://help.suno.com/en/articles/5782849),
[context in lyrics](https://help.suno.com/en/articles/5782977), and
[excluding elements](https://help.suno.com/en/articles/3161921), but it does not
define MusicAPI.ai's JSON schema. Treat upstream UI behavior as a heuristic until
the MusicAPI.ai endpoint or a bounded live test confirms it.
