# Ollama + Llama 3.2 Installation Guide for LuminariMUD NPCs

## Quick Start for LuminariMUD

This guide installs Ollama with Llama 3.2 to provide AI-powered NPCs in LuminariMUD. The integration provides:
- **Always-on AI**: NPCs respond intelligently even when OpenAI is disabled
- **Fast responses**: 0.5-1 second response times with local processing
- **Zero cost**: Runs on your server hardware, no API fees
- **128K context window**: Supports future conversation history features

After installation, NPCs with the `MOB_AI_ENABLED` flag will automatically use Ollama when:
1. OpenAI is disabled (`ai disable` command)
2. OpenAI fails or times out (automatic fallback)
3. Testing with `./test_ollama` utility

## System Requirements
- **OS**: Ubuntu 20.04+ (or compatible Linux distribution)
- **RAM**: Minimum 4GB (8GB recommended)
- **Storage**: 10GB free space
- **CPU**: 2+ cores (4+ recommended)
- **Architecture**: x86_64 with AVX2 support (most modern CPUs)

## Installation Steps

### Step 1: Install Ollama (as root)
```bash
# Switch to root user
sudo su -

# Download and install Ollama
curl -fsSL https://ollama.com/install.sh | sh

# Verify installation was successful
ollama --version
# Expected output: ollama version is 0.11.x

# Exit root session
exit
```

**What this does:**
- Installs Ollama to `/usr/local/bin/ollama`
- Creates `ollama` system user and group
- Sets up systemd service
- Starts Ollama service on port 11434

### Step 2: Configure User Permissions
```bash
# Add your MUD user to the ollama group (replace 'luminari' with your username)
sudo usermod -a -G ollama luminari

# Verify group membership
groups luminari
# Should show: luminari : luminari users ollama
```

**Note:** You may need to log out and back in for group changes to take effect.

### Step 3: Setup Model Storage Directory (as MUD user)
```bash
# Switch to your MUD user
su - luminari

# Create directory for model storage
mkdir -p ~/ollama/models

# Add environment variable to .bashrc
echo 'export OLLAMA_MODELS=$HOME/ollama/models' >> ~/.bashrc

# Load the new environment variable
source ~/.bashrc

# Verify the setting
echo $OLLAMA_MODELS
# Should output: /home/luminari/ollama/models
```

### Step 4: Download Llama 3.2 Model
```bash
# Pull the Llama 3.2 1B model (optimized for roleplay)
ollama pull llama3.2:1b
# This downloads ~1.3GB, may take 2-5 minutes

# Verify model is installed
ollama list
# Should show: llama3.2:1b with size ~1.3GB

# Quick test of the model
ollama run llama3.2:1b "You are a goblin guard. Say something threatening:"
# Type /bye to exit the chat interface
```

### Step 5: Test API Endpoint
```bash
# Test basic API call (using jq for JSON formatting if available, or just view raw)
curl -s http://localhost:11434/api/generate -d '{
  "model": "llama3.2:1b",
  "prompt": "You are a grumpy dwarf merchant. A player asks what you sell. Respond briefly:",
  "stream": false
}' | jq -r '.response' 2>/dev/null || curl -s http://localhost:11434/api/generate -d '{
  "model": "llama3.2:1b",
  "prompt": "You are a grumpy dwarf merchant. A player asks what you sell. Respond briefly:",
  "stream": false
}'

# Test with LuminariMUD's optimized settings
curl -s http://localhost:11434/api/generate -d '{
  "model": "llama3.2:1b",
  "prompt": "You are an NPC in a fantasy RPG game. You are a janni bladesman in the desert. A player greets you. Respond in character briefly:",
  "stream": false,
  "options": {
    "num_predict": 100,
    "temperature": 0.7,
    "top_k": 40,
    "top_p": 0.9
  }
}' | jq -r '.response' 2>/dev/null || echo "Response received"
```

### Step 6: Test LuminariMUD Integration
```bash
# Compile the standalone test program from our codebase
cd /path/to/luminari-source
gcc -o test_ollama test_ollama_ai.c -lcurl

# Test with default prompt
./test_ollama

# Test with custom NPC scenarios
./test_ollama "You are a janni bladesman. A player asks about the desert"
./test_ollama "You are a drow assassin. Someone discovers you hiding"
./test_ollama "You are a dragon. Adventures enter your lair"

# Create a simple test script for multiple NPCs
cat > ~/test_luminari_npcs.sh << 'EOF'
#!/bin/bash
# Test various LuminariMUD NPC types

echo "Testing LuminariMUD NPC responses..."
echo "===================================="
echo

# Function to test NPC response
test_npc() {
    local npc_type="$1"
    local situation="$2"
    
    echo "Testing: $npc_type"
    curl -s http://localhost:11434/api/generate -d "{
        \"model\": \"llama3.2:1b\",
        \"prompt\": \"You are an NPC in a fantasy RPG game. You are $npc_type. $situation Respond in character briefly:\",
        \"stream\": false,
        \"options\": {
            \"num_predict\": 100,
            \"temperature\": 0.7,
            \"top_k\": 40,
            \"top_p\": 0.9
        }
    }" | jq -r '.response' 2>/dev/null || echo "Error getting response"
    echo "---"
    echo
}

# Test various NPC types from LuminariMUD
test_npc "a janni bladesman in the desert" "A player asks about desert dangers"
test_npc "a drow merchant in the underdark" "Someone inquires about poison"
test_npc "a gnome tinkerer" "A player asks about your inventions"
test_npc "an orc warrior" "Someone challenges you to combat"
test_npc "a temple priest" "A wounded adventurer seeks healing"
test_npc "a thieves guild fence" "Someone wants to sell stolen goods"
EOF

chmod +x ~/test_luminari_npcs.sh
~/test_luminari_npcs.sh
```

### Step 7: (Optional) Configure as Custom Service
If you need custom service configuration:

```bash
# Create service file
sudo nano /etc/systemd/system/ollama-mud.service
```

Add this content:
```ini
[Unit]
Description=Ollama for MUD NPCs
After=network.target

[Service]
Type=simple
User=luminari
Group=luminari
WorkingDirectory=/home/luminari
Environment="OLLAMA_MODELS=/home/luminari/ollama/models"
Environment="OLLAMA_HOST=127.0.0.1:11434"
Environment="OLLAMA_NUM_PARALLEL=4"
ExecStart=/usr/local/bin/ollama serve
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

Activate the service:
```bash
# Reload systemd configurations
sudo systemctl daemon-reload

# Enable service to start on boot
sudo systemctl enable ollama-mud

# Start the service
sudo systemctl start ollama-mud

# Check service status
sudo systemctl status ollama-mud
```

## LuminariMUD Integration Examples

### Testing from Within the Game
```bash
# As an admin character in-game:

# Check AI service status
ai

# Test with AI disabled (Ollama-only mode)
ai disable
tell guard hello there

# Enable dual-backend mode (OpenAI + Ollama fallback)
ai enable
tell guard what brings you here?

# Check cache statistics
ai cache

# Monitor responses in syslog
# In another terminal:
tail -f syslog | grep "AI \["
```

### C Integration Example (Simplified from LuminariMUD)
```c
/* From ai_service.c - Ollama request function */
static char *make_ollama_request(const char *prompt) {
    CURL *curl;
    CURLcode res;
    struct curl_response response = {0};
    struct curl_slist *headers = NULL;
    char *json_request;
    char *result = NULL;
    long http_code;
    
    /* Build JSON request with proper escaping */
    json_request = build_ollama_json_request(prompt);
    if (!json_request) {
        return NULL;
    }
    
    /* Initialize CURL */
    curl = curl_easy_init();
    if (!curl) {
        free(json_request);
        return NULL;
    }
    
    /* Configure request */
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/generate");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_request);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ai_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000L); /* 10 sec timeout */
    
    /* Execute request */
    res = curl_easy_perform(curl);
    
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code == 200 && response.data) {
            result = parse_ollama_json_response(response.data);
        }
    }
    
    /* Cleanup */
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(json_request);
    if (response.data) free(response.data);
    
    return result;
}

/* Example usage in NPC dialogue */
void ai_npc_dialogue_async(struct char_data *npc, struct char_data *ch, const char *input) {
    char prompt[MAX_STRING_LENGTH];
    
    /* Build NPC-specific prompt */
    snprintf(prompt, sizeof(prompt),
        "You are %s in a fantasy RPG world. "
        "Respond to the player's message in character. "
        "Keep response under 100 words. "
        "Player says: %s",
        GET_NAME(npc), input);
    
    /* Queue async request (non-blocking) */
    queue_ai_request(npc, ch, prompt);
}
```

## LuminariMUD NPC Prompt Templates

### Standard LuminariMUD Format
```
You are an NPC in a fantasy RPG game. You are [NPC description]. [Situation]. Respond in character briefly:
```

### Specific NPC Examples

**Desert NPCs (Sanctus)**
```
You are an NPC in a fantasy RPG game. You are a janni bladesman guarding the desert oasis. A traveler approaches. Respond in character briefly:
```

**Underdark NPCs**
```
You are an NPC in a fantasy RPG game. You are a drow merchant in the underdark city. Someone asks about your wares. Respond in character briefly:
```

**City Guards**
```
You are an NPC in a fantasy RPG game. You are a Palanthas city guard at the north gate. Someone approaches at night. Respond in character briefly:
```

**Temple NPCs**
```
You are an NPC in a fantasy RPG game. You are a priest of Paladine in the temple. A wounded adventurer seeks healing. Respond in character briefly:
```

**Thieves Guild**
```
You are an NPC in a fantasy RPG game. You are a fence in the thieves guild. Someone wants to sell stolen goods. Respond in character briefly:
```

## Performance Optimization Settings

### LuminariMUD Default (Balanced)
```json
{
  "num_predict": 100,     // Standard NPC response length
  "temperature": 0.7,     // Balanced creativity
  "top_k": 40,           // Moderate variety
  "top_p": 0.9           // Good sampling
}
```

### Fast Combat Responses
```json
{
  "num_predict": 30,      // Short combat taunts
  "temperature": 0.5,     // More predictable
  "top_k": 20,           // Fewer choices for speed
  "top_p": 0.8           // Focused sampling
}
```

### Detailed Quest/Lore NPCs
```json
{
  "num_predict": 150,     // Longer explanations
  "temperature": 0.8,     // More creative
  "top_k": 50,           // More variety
  "top_p": 0.95          // Better sampling
}
```

### Merchant/Shop NPCs
```json
{
  "num_predict": 80,      // Item descriptions
  "temperature": 0.6,     // Consistent responses
  "top_k": 30,           // Limited variety
  "top_p": 0.85          // Focused responses
}
```

## Service Management Commands

```bash
# Start/stop/restart service
sudo systemctl start ollama
sudo systemctl stop ollama
sudo systemctl restart ollama

# Check service status
sudo systemctl status ollama

# View logs
journalctl -u ollama -f          # Follow logs in real-time
journalctl -u ollama -n 100      # View last 100 lines
journalctl -u ollama --since "1 hour ago"  # Recent logs

# Model management
ollama list                       # List installed models
ollama pull MODEL_NAME            # Download a model
ollama rm MODEL_NAME              # Remove a model
ollama show MODEL_NAME            # Show model details
```

## Troubleshooting Guide

### Issue: "Command not found"
```bash
# Add to PATH
export PATH=$PATH:/usr/local/bin
echo 'export PATH=$PATH:/usr/local/bin' >> ~/.bashrc
source ~/.bashrc
```

### Issue: "Permission denied"
```bash
# Ensure user is in ollama group
sudo usermod -a -G ollama $USER
# Log out and back in for changes to take effect
```

### Issue: "Connection refused" on API calls
```bash
# Check if service is running
sudo systemctl status ollama

# Start if not running
sudo systemctl start ollama

# Check if port is listening
netstat -tlnp | grep 11434
```

### Issue: Slow responses
```bash
# Check CPU usage
htop

# Reduce token count in API calls
# Set "num_predict": 20 in options

# Check available memory
free -h

# Restart service to clear cache
sudo systemctl restart ollama
```

### Issue: Model gives "helpful AI" responses instead of roleplay
- Wrong model (some models are over-safety-trained)
- Solution: Use llama3.2:1b or tinyllama models
- Add to prompts: "Stay in character" or "Respond ONLY as the character"

### Issue: High memory usage
```bash
# Check memory usage
free -h

# Limit concurrent requests in your application
# Consider using smaller model (tinyllama)
# Restart service periodically to clear memory
```

## Resource Usage Expectations

- **Model Size on Disk**: ~1.3GB for llama3.2:1b
- **RAM Usage**: ~1.2GB when model is loaded
- **CPU Usage**: 50-70% during generation (2-3 seconds)
- **Response Time**: 1-2 seconds with optimized settings
- **Concurrent Requests**: Can handle 3-4 simultaneous requests

## Security Considerations

1. **Local Only**: Ollama binds to 127.0.0.1 by default (not accessible externally)
2. **User Permissions**: Runs with MUD user permissions, not root
3. **Model Storage**: Stored in user home directory with appropriate permissions
4. **API Access**: No authentication by default (safe for local-only access)

## Useful Model Alternatives

### For Faster Responses
```bash
ollama pull tinyllama        # 1.1B, optimized for speed
```

### For Better Quality
```bash
ollama pull llama3.2:3b      # 3B parameters, better responses
ollama pull mistral:7b-instruct-q4_K_M  # 7B, excellent roleplay
```

### For Specific Use Cases
```bash
ollama pull dolphin-phi:2.7b # Less censored, good for fantasy
ollama pull gemma:2b         # Google's model, good reasoning
```

## Complete Installation Time
- Ollama installation: ~1 minute
- Model download: 2-5 minutes (depending on connection)
- Configuration: ~2 minutes
- Testing: 2-3 minutes
- **Total: ~10 minutes**

## Quick Verification Script
```bash
#!/bin/bash
# Save as verify_setup.sh

echo "=== Ollama MUD Setup Verification ==="
echo ""

# Check Ollama installation
echo "1. Checking Ollama installation..."
if command -v ollama &> /dev/null; then
    echo "   ✓ Ollama installed: $(ollama --version)"
else
    echo "   ✗ Ollama not found"
fi

# Check service status
echo "2. Checking service status..."
if systemctl is-active --quiet ollama; then
    echo "   ✓ Ollama service is running"
else
    echo "   ✗ Ollama service is not running"
fi

# Check models
echo "3. Checking installed models..."
if ollama list | grep -q "llama3.2:1b"; then
    echo "   ✓ llama3.2:1b model installed"
else
    echo "   ✗ Model not found"
fi

# Test API
echo "4. Testing API endpoint..."
if curl -s http://localhost:11434/api/generate -d '{"model":"llama3.2:1b","prompt":"Hi","stream":false}' | grep -q "response"; then
    echo "   ✓ API is responding"
else
    echo "   ✗ API not responding"
fi

echo ""
echo "=== Verification Complete ==="
```

## LuminariMUD-Specific Notes

### Integration Points
- **Primary code**: `src/ai_service.c` - Contains Ollama integration
- **Configuration**: `src/ai_service.h` - Model and endpoint settings
- **Test utility**: `test_ollama_ai.c` - Standalone testing tool
- **Documentation**: `docs/systems/AI_SERVICE_README.md` - Complete AI system docs

### In-Game Usage
```
# Admin commands
ai                  # Show AI service status
ai disable         # Switch to Ollama-only mode
ai enable          # Use OpenAI with Ollama fallback
ai test            # Test connectivity

# Player interaction
tell guard hello   # Guard responds via Ollama when AI disabled
say hi             # NPCs in room may respond if AI-enabled
```

### Monitoring
```bash
# Watch AI responses in real-time
tail -f syslog | grep "AI \["

# Check for Ollama-specific responses
grep "AI \[Ollama\]" syslog | tail -20

# Monitor service health
systemctl status ollama
journalctl -u ollama -f
```

## Support and Documentation

- **LuminariMUD AI Docs**: `/docs/systems/AI_SERVICE_README.md`
- **Ollama Documentation**: https://ollama.ai/
- **Model Library**: https://ollama.ai/library
- **API Reference**: https://github.com/ollama/ollama/blob/main/docs/api.md

---

*This guide configures Ollama with Llama 3.2 1B model for LuminariMUD NPC responses. The integration provides always-on AI capability with automatic fallback from OpenAI, ensuring NPCs can always respond intelligently to players.*
