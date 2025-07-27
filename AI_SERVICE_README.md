# LuminariMUD AI Service Integration

## Overview

The AI Service integration adds OpenAI-powered natural language processing to LuminariMUD, enabling dynamic NPC dialogue, procedural content generation, and content moderation. This implementation is designed to be secure, performant, and easy to manage.

## Features

- **Dynamic NPC Dialogue**: NPCs with the AI_ENABLED flag can respond intelligently to player messages
- **Response Caching**: Reduces API calls and improves response times
- **Rate Limiting**: Prevents excessive API usage and cost overruns
- **Secure API Key Storage**: Encrypted storage of API credentials
- **Administrative Controls**: Full control over AI features through in-game commands
- **Performance Monitoring**: Track usage statistics and response times

## Installation

### 1. Install Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev libjson-c-dev libssl-dev

# CentOS/RHEL
sudo yum install libcurl-devel json-c-devel openssl-devel
```

### 2. Compile the MUD

The AI service files are automatically included in the build:

```bash
make clean
make
```

### 3. Set Up Database

Run the migration script to create necessary tables:

```bash
mysql -u your_user -p your_database < ai_service_migration.sql
```

### 4. Configure API Key

Copy the example .env file to the lib directory and add your API key:

```bash
# Copy the example file to lib directory
cp .env.example lib/.env

# Set secure permissions
chmod 600 lib/.env

# Edit lib/.env and add your OpenAI API key
# Look for the line: OPENAI_API_KEY=
# Add your key after the equals sign
```

Your lib/.env file should contain:
```
OPENAI_API_KEY=sk-proj-your-actual-api-key-here
```

The .env file in lib/ is automatically ignored by git for security.

## Configuration

### Environment Variables (.env)

The AI service reads configuration from the .env file:

- `OPENAI_API_KEY`: Your OpenAI API key (required)
- `AI_MODEL`: Model to use (default: gpt-4.1-mini)
  - gpt-4.1-mini: Fastest and most cost-effective
  - gpt-4o-mini: Very affordable ($0.15/1M input tokens)
  - gpt-4.1: Best quality with 1M token context
- `AI_MAX_TOKENS`: Maximum response length (default: 500)
- `AI_TEMPERATURE`: Response creativity 0-10 (default: 7, divided by 10)
- `AI_REQUESTS_PER_MINUTE`: Rate limit (default: 60)
- `AI_REQUESTS_PER_HOUR`: Rate limit (default: 1000)
- `AI_CONTENT_FILTER_ENABLED`: Enable content filtering (default: true)

### In-Game Configuration

Use the `ai` command as an administrator (LVL_GRGOD+):

```
ai                    - Show AI service status
ai enable             - Enable AI service
ai disable            - Disable AI service
ai cache clear        - Clear response cache
ai cache cleanup      - Remove expired cache entries
ai test               - Test AI connectivity
ai reload             - Reload configuration from .env
ai reset              - Reset rate limits
```

## Usage

### Enabling AI for NPCs

1. Use `medit` to edit a mobile
2. Toggle the AI_ENABLED flag
3. Save the mobile

### Testing AI NPCs

```
tell <npc_name> Hello, how are you today?
```

The NPC will respond with a contextually appropriate message after a short delay.

### Monitoring Usage

View usage statistics with the stored procedure:

```sql
CALL get_ai_usage_stats(7);  -- Last 7 days
```

## Security Considerations

1. **API Key Protection**: 
   - Store in .env file (not in version control)
   - Keys are encrypted in memory
   - Never logged or displayed

2. **Rate Limiting**: Configure appropriate limits based on your OpenAI plan

3. **Content Filtering**: Enable content filtering for public servers

4. **Access Control**: Only high-level administrators can manage AI settings

## Performance Optimization

1. **Caching**: Responses are cached for 1 hour by default
2. **Async Processing**: AI responses are queued to avoid blocking
3. **Batch Requests**: Consider implementing batch processing for multiple NPCs
4. **Database Indexes**: Ensure indexes are maintained for optimal query performance

## Troubleshooting

### AI Service Not Working

1. Check if service is enabled: `ai`
2. Verify API key is in .env file
3. Reload configuration: `ai reload`
4. Test connectivity: `ai test`
5. Check logs for errors

### NPCs Not Responding

1. Verify NPC has AI_ENABLED flag
2. Check if service is enabled
3. Review system logs for errors
4. Ensure database tables exist

### Performance Issues

1. Monitor cache hit rate
2. Adjust cache expiration time
3. Review rate limiting settings
4. Consider using a faster model (gpt-4o-mini vs gpt-4.1)

## Cost Management

OpenAI API usage incurs costs. To manage expenses:

1. Use gpt-4.1-mini or gpt-4o-mini for most NPCs
   - gpt-4.1-mini: 83% cheaper than gpt-4o
   - gpt-4o-mini: $0.15/1M input, $0.60/1M output tokens
2. Implement aggressive caching
3. Set conservative rate limits
4. Monitor usage regularly
5. Disable AI for non-essential NPCs

Note: Pricing has dropped 99% since 2022 models while improving quality!

## Future Enhancements

- Quest generation assistance
- Dynamic room descriptions
- Player behavior analysis
- Automated event narratives
- Multi-language support
- Voice integration

## Files Created

- `ai_service.c/h` - Core AI service implementation
- `ai_cache.c` - Response caching
- `ai_security.c` - API key encryption
- `ai_events.c` - Event system integration
- `dotenv.c/h` - .env file parser
- `ai_service_migration.sql` - Database schema

## Support

For issues or questions:
1. Check the system logs
2. Review this documentation
3. Consult the LuminariMUD forums
4. Submit issues to the GitHub repository

## Credits

AI Service Integration developed by the LuminariMUD team.
Based on OpenAI's GPT models.