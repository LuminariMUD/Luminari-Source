# LuminariMUD AI Service Integration

## Quick Start

### 1. Install Dependencies
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev libjson-c-dev libssl-dev

# CentOS/RHEL
sudo yum install libcurl-devel json-c-devel openssl-devel
```

### 2. Setup
```bash
# Build
make clean && make

# Database
mysql -u your_user -p your_database < ai_service_migration.sql

# Configure API key
cp .env.example lib/.env
chmod 600 lib/.env
# Edit lib/.env and add: OPENAI_API_KEY=sk-proj-your-key-here
```

## Configuration

### Environment Variables (lib/.env)
| Variable | Default | Description |
|----------|---------|-------------|
| `OPENAI_API_KEY` | *required* | Your OpenAI API key |
| `AI_MODEL` | gpt-4o-mini | Model selection (gpt-4o-mini, gpt-4o, gpt-4) |
| `AI_MAX_TOKENS` | 500 | Maximum response length |
| `AI_TEMPERATURE` | 7 | Creativity (0-10, divided by 10) |
| `AI_REQUESTS_PER_MINUTE` | 60 | Rate limit |
| `AI_REQUESTS_PER_HOUR` | 1000 | Rate limit |
| `AI_CONTENT_FILTER_ENABLED` | true | Content filtering |

### Admin Commands (LVL_GRSTAFF+)
```
ai                    - Show status
ai enable/disable     - Toggle service
ai cache clear        - Clear cache
ai test              - Test connectivity
ai reload            - Reload config
ai reset             - Reset rate limits
```

## Usage

### Enable AI for NPCs
1. `medit <mob>` - Edit mobile
2. Toggle `AI_ENABLED` flag
3. Save changes

### Test
```
tell <npc> Hello, how are you?
```

## Cost Management

**Model Pricing** (per 1M tokens):
- `gpt-4o-mini`: $0.15 input / $0.60 output (recommended)
- `gpt-4o`: $2.50 input / $10 output
- `gpt-4`: $30 input / $60 output

**Tips**:
- Use gpt-4o-mini for most NPCs
- Enable aggressive caching
- Set conservative rate limits
- Monitor usage: `CALL get_ai_usage_stats(7);`

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Service not working | Check `ai` status, verify API key, run `ai reload` |
| NPCs not responding | Verify AI_ENABLED flag, check logs |
| High costs | Switch to gpt-4o-mini, increase caching |
| Performance issues | Review cache hit rate, adjust rate limits |

## Security
- API keys stored in .env (gitignored)
- Keys encrypted in memory
- Content filtering available
- Admin-only access control

## Files
- **Core**: `ai_service.c/h` - Main implementation
- **Support**: `ai_cache.c`, `ai_security.c`, `ai_events.c`
- **Config**: `dotenv.c/h` - Environment parser
- **Database**: `ai_service_migration.sql`

## Features
- Dynamic NPC dialogue with context awareness
- Response caching (1-hour default)
- Rate limiting and usage tracking
- Async processing (non-blocking)
- Content moderation options