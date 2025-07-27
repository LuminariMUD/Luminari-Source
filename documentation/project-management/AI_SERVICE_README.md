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

Create a file to store your encrypted API key:

```bash
# Create the API key file (ensure proper permissions)
touch lib/ai_api_key.txt
chmod 600 lib/ai_api_key.txt

# Store your OpenAI API key (the system will encrypt it)
echo "your-openai-api-key-here" > lib/ai_api_key.txt
```

## Configuration

### In-Game Configuration

Use the `ai` command as an administrator (LVL_GRGOD+):

```
ai                    - Show AI service status
ai enable             - Enable AI service
ai disable            - Disable AI service
ai cache clear        - Clear response cache
ai cache cleanup      - Remove expired cache entries
ai test               - Test AI connectivity
ai reload             - Reload configuration
ai reset              - Reset rate limits
```

### Database Configuration

Key configuration values are stored in the `ai_config` table:

- `enabled`: Enable/disable the service
- `model`: OpenAI model to use (default: gpt-3.5-turbo)
- `max_tokens`: Maximum response length (default: 500)
- `temperature`: Response creativity (0-1, default: 0.7)
- `requests_per_minute`: Rate limit (default: 60)
- `requests_per_hour`: Rate limit (default: 1000)

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

1. **API Key Protection**: Never commit API keys to version control
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
2. Verify API key is loaded: Check logs for "API key loaded"
3. Test connectivity: `ai test`
4. Check rate limits: May be exceeded

### NPCs Not Responding

1. Verify NPC has AI_ENABLED flag
2. Check if service is enabled
3. Review system logs for errors
4. Ensure database tables exist

### Performance Issues

1. Monitor cache hit rate
2. Adjust cache expiration time
3. Review rate limiting settings
4. Consider using a faster model (gpt-3.5-turbo vs gpt-4)

## Cost Management

OpenAI API usage incurs costs. To manage expenses:

1. Use gpt-3.5-turbo for most NPCs (cheaper than gpt-4)
2. Implement aggressive caching
3. Set conservative rate limits
4. Monitor usage regularly
5. Disable AI for non-essential NPCs

## Future Enhancements

- Quest generation assistance
- Dynamic room descriptions
- Player behavior analysis
- Automated event narratives
- Multi-language support
- Voice integration

## Support

For issues or questions:
1. Check the system logs
2. Review this documentation
3. Consult the LuminariMUD forums
4. Submit issues to the GitHub repository

## Credits

AI Service Integration developed by the LuminariMUD team.
Based on OpenAI's GPT models.