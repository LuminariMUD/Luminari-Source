-- Intermud3 in-game help entries for help_entries and help_keywords.
--
-- The help system is database-first. This script is the reviewable,
-- repeatable deployment source for the I3 overview and every command
-- registered in src/interpreter.c. It is safe to rerun: existing entries are
-- updated and existing keyword mappings are preserved.
--
-- Player topics are level 0. I3ADMIN matches LVL_IMMORT (31).

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INTERMUD3', 'INTERMUD3 (I3)

Intermud3 connects LuminariMUD to other MUDs through a shared network.
You can send private tells, talk on shared channels, find remote players,
and inspect the MUDs and channels currently known to the gateway.

Player commands:
  i3mudlist                         - list known MUDs
  i3channels                        - show known channels
  i3channels list                   - request a fresh channel list
  i3channels join <channel>         - subscribe this MUD to a channel
  i3channels leave <channel>        - unsubscribe this MUD from a channel
  i3chat <channel> <message>        - speak on a shared channel
  i3tell <user>@<mud> <message>     - send a private remote tell
  i3who <mud>                       - request a remote who list
  i3finger <user>@<mud>             - request remote player information
  i3locate <user>                   - search the network for a player
  i3config                          - show I3 feature switches

Most commands require an active I3 connection. Network requests and
messages are asynchronous: a local confirmation means the request was
queued, not that the remote MUD has answered or received it.

Immortals can use I3ADMIN to inspect and manage the connection.

See also: I3MUDLIST, I3CHANNELS, I3CHAT, I3TELL, I3WHO, I3FINGER,
I3LOCATE, I3CONFIG, I3ADMIN', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INTERMUD3', 'I3');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INTERMUD3', 'I3-COMMANDS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INTERMUD3', 'INTERMUD');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INTERMUD3', 'INTERMUD3');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('I3TELL', 'Usage: i3tell <user>@<mud> <message>

Sends a private tell to a player on another Intermud3 MUD.

The destination must use the user@mud form. The MUD name must match an
online entry in the local I3 MUD list.

Example:
  i3tell Aria@ExampleMUD Hello from LuminariMUD!

After the request is queued, you see a local copy of the tell. That
confirmation does not guarantee delivery; the remote player or MUD may no
longer be available.

Use I3MUDLIST to find exact MUD names. Use I3LOCATE if you know the player
name but not the MUD.

See also: INTERMUD3, I3MUDLIST, I3LOCATE, I3CHAT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3TELL', 'I3-TELL');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3TELL', 'I3TELL');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3TELL', 'INTERMUD-TELL');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('I3CHAT', 'Usage: i3chat <channel> <message>
       i3chat <single-word-message>

Sends a message to an Intermud3 channel.

For a normal multi-word message, name the channel explicitly:
  i3chat intermud Greetings from LuminariMUD!

If the command receives exactly one word, that word is sent as the message
on the configured default channel. With two or more words, the first word
is always treated as the channel name and the rest as the message.

The MUD must be subscribed to the channel and I3 channel traffic must be
enabled. A local echo confirms that the message was queued. Incoming
messages on subscribed channels are shown to all online players.

See also: INTERMUD3, I3CHANNELS, I3CONFIG, I3TELL', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3CHAT', 'I3-CHAT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3CHAT', 'I3CHAT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3CHAT', 'INTERMUD-CHAT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('I3WHO', 'Usage: i3who <mud>

Requests the visible player list from another Intermud3 MUD.

Example:
  i3who ExampleMUD

The request is asynchronous. If the remote MUD supports who requests, its
reply is displayed when it arrives. You must still be online to receive
the reply.

Use I3MUDLIST to find the exact remote MUD name. I3 who queries must also
be enabled in I3CONFIG.

See also: INTERMUD3, I3MUDLIST, I3FINGER, I3LOCATE, I3CONFIG', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3WHO', 'I3-WHO');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3WHO', 'I3WHO');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3WHO', 'INTERMUD-WHO');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('I3FINGER', 'Usage: i3finger <user>@<mud>

Requests information about a player on another Intermud3 MUD.

Example:
  i3finger Aria@ExampleMUD

The destination must use the user@mud form. The named MUD must be present
and online in the local I3 MUD list. If the remote MUD supports finger
requests, the reply may include the player name, title, real name, email,
level, and idle time. Remote MUDs decide which fields they provide.

The request is asynchronous, and you must still be online when the reply
arrives. I3 who queries must be enabled in I3CONFIG.

See also: INTERMUD3, I3WHO, I3LOCATE, I3MUDLIST, I3CONFIG', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3FINGER', 'I3-FINGER');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3FINGER', 'I3FINGER');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3FINGER', 'INTERMUD-FINGER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('I3LOCATE', 'Usage: i3locate <user>

Searches the Intermud3 network for a player name.

Example:
  i3locate Aria

Unlike I3FINGER and I3TELL, this command does not require you to know the
remote MUD first. A successful reply reports the matching player, MUD,
idle time, and any status supplied by the network.

The request is asynchronous, and you must still be online when the reply
arrives. A player who is hidden, offline, or on a MUD without locate
support may not be found. I3 who queries must be enabled in I3CONFIG.

See also: INTERMUD3, I3FINGER, I3WHO, I3TELL, I3CONFIG', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3LOCATE', 'I3-LOCATE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3LOCATE', 'I3LOCATE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3LOCATE', 'INTERMUD-LOCATE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('I3MUDLIST', 'Usage: i3mudlist

Shows the Intermud3 MUD list currently cached by LuminariMUD.

For each known MUD, the display includes its name, online or offline
status, MUD type, and connection port. Use the exact displayed name with
commands such as I3WHO, I3FINGER, and I3TELL.

The list is refreshed when the I3 session connects. If the cached copy is
more than one hour old, this command also queues a refresh for a later
display. The current output is still the cached list.

See also: INTERMUD3, I3WHO, I3FINGER, I3TELL, I3CHANNELS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3MUDLIST', 'I3-MUDLIST');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3MUDLIST', 'I3MUDLIST');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3MUDLIST', 'INTERMUD-MUDLIST');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('I3CHANNELS', 'Usage: i3channels
       i3channels list
       i3channels join <channel>
       i3channels leave <channel>

Manages this MUD session''s Intermud3 channel subscriptions.

With no argument, I3CHANNELS displays the locally cached channel list and
marks subscribed channels. The LIST subcommand requests a fresh list from
the gateway; run I3CHANNELS again after the reply arrives to see it.

JOIN and LEAVE queue a subscription change for the named channel. The
subscription belongs to the MUD session, not to one character. Incoming
traffic from subscribed channels is displayed to all online players.

Use I3CHAT to speak on a subscribed channel. Channel traffic must be
enabled in I3CONFIG.

See also: INTERMUD3, I3CHAT, I3CONFIG, I3MUDLIST', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3CHANNELS', 'I3-CHANNELS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3CHANNELS', 'I3CHANNELS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3CHANNELS', 'INTERMUD-CHANNELS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('I3CONFIG', 'Usage: i3config
       i3config tells
       i3config channels
       i3config who

Shows or toggles the running Intermud3 feature switches.

With no argument, I3CONFIG displays whether tells, channels, and who-style
queries are enabled. Naming a setting toggles its current value:
  tells     - outgoing I3 tells
  channels  - channel sends, joins, and leaves
  who       - who, finger, and locate requests

These are server-wide runtime settings, not character preferences. The
current command toggles the named setting; it does not set an explicit
on or off value. A reload or restart can replace runtime values with the
saved I3 configuration. Immortals can inspect or save that configuration
with I3ADMIN.

See also: INTERMUD3, I3TELL, I3CHAT, I3WHO, I3ADMIN', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3CONFIG', 'I3-CONFIG');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3CONFIG', 'I3CONFIG');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3CONFIG', 'INTERMUD-CONFIG');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('I3ADMIN', 'Usage: i3admin
       i3admin status
       i3admin stats
       i3admin reconnect
       i3admin reload
       i3admin save

Immortal command for inspecting and managing the Intermud3 client.

Subcommands:
  status     - show state, MUD name, gateway, session, authentication,
               and connection uptime
  stats      - show message, error, reconnect, queue, channel, and MUD
               counters
  reconnect  - disconnect and start a new gateway connection
  reload     - reload the runtime settings from lib/i3_config
  save       - write the current runtime settings to lib/i3_config

RECONNECT, RELOAD, and SAVE affect the server-wide I3 client. Review the
current status and coordinate disruptive changes before using them. If
RELOAD changes connection details, use RECONNECT to establish a session
with the new settings.

See also: INTERMUD3, I3CONFIG, I3MUDLIST, I3CHANNELS', 31, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3ADMIN', 'I3-ADMIN');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3ADMIN', 'I3ADMIN');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('I3ADMIN', 'INTERMUD-ADMIN');

COMMIT;
