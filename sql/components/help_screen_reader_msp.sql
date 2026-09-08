-- Screen-reader setup and optional MSP player help.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('SCREEN-READER', 'Screen-reader output

Usage: screenreader on | off | status

Character creation asks whether you want screen-reader-friendly output before
identity selection. Yes hides automatic ASCII maps and repeated gameplay prompts
from the first room onward. Room descriptions and normal game messages remain.

The choice is saved with your character. Before the initial character save,
a disconnected creation attempt starts again and asks the question again.

screenreader on hides automatic maps and gameplay prompts without changing your
underlying automap or prompt settings. screenreader off uses those settings
again. Changes to automap or prompt while mode is on take effect when it is off.
Explicitly requested maps remain available. Pager and editor instructions remain.

Useful text commands:
hp - hit points
moves - movement points
tnl - experience needed for your next level
survey - wilderness information and navigation

You can also use toggle automap and prompt none independently of this mode.
This setting does not change brief mode, colors, channels, or combat-roll output.
Sound is separate and off by default. See help sound for optional audio.
Report remaining accessibility gaps with bug or idea.

See also: SOUND, PROMPT, TOGGLE, SURVEY
', 0, 0) ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SCREEN-READER', 'SCREEN-READER');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SCREEN-READER', 'SCREENREADER');
INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('SOUND', 'Optional sound

Usage: sound on | off | status | test

Sound is off by default. sound on saves your consent to sound effects;
sound off stops future effects. This setting is independent of screenreader.
Your client must also negotiate MUD Sound Protocol (MSP). General GMCP or MSDP
support alone does not enable audio. Reconnecting does not undo your choice.

sound status reports your preference, MSP capability, and whether playback is
enabled. sound test sends a short test cue only when both settings permit it.
A successful door opening sends a short cue to the player opening the door;
normal text feedback is unchanged. Failed opens and container opens do not cue.

Install luminari-test.wav and luminari-door-open.wav from the repository''s
lib/sounds directory into your client''s MSP sound directory. The files must
have those exact names. See lib/sounds/README.md for format and setup details.
There is no automatic download URL supplied by the server.

If a test is silent, check your client''s MSP setting, installed sound files,
volume/mute, and audio output device. Sending a cue does not prove your client
played it. Missing files never prevent play; sound is supplementary to text.
PREFEDIT Sound changes the same saved preference, not client capabilities.

See also: SCREEN-READER, PREFEDIT
', 0, 0) ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SOUND', 'SOUND');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SOUND', 'MSP');

COMMIT;
