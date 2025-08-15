# Ollama + Llama 3.2 Installation Guide for MUD NPCs

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
# Test basic API call
curl http://localhost:11434/api/generate -d '{
  "model": "llama3.2:1b",
  "prompt": "You are a grumpy dwarf merchant. A player asks what you sell. Respond briefly:",
  "stream": false
}' | python3 -m json.tool

# Test with optimized settings for speed
curl http://localhost:11434/api/generate -d '{
  "model": "llama3.2:1b",
  "prompt": "You are a goblin guard. An adventurer approaches. You say:",
  "stream": false,
  "options": {
    "num_predict": 30,
    "temperature": 0.7,
    "top_k": 40,
    "top_p": 0.9
  }
}' | python3 -m json.tool
```

### Step 6: Create Integration Test Script
```bash
# Create a Python test script
cat > ~/test_npc.py << 'EOF'
#!/usr/bin/env python3
"""
Test script for MUD NPC responses using Ollama
"""
import requests
import json
import time

def get_npc_response(character, situation, max_tokens=30):
    """
    Get an NPC response from Ollama
    
    Args:
        character: Description of the NPC (e.g., "a grumpy dwarf merchant")
        situation: The current situation/prompt
        max_tokens: Maximum response length (default 30 for speed)
    
    Returns:
        The NPC's response text
    """
    start_time = time.time()
    
    # Construct the prompt
    prompt = f"You are {character}. {situation} Respond in character briefly:"
    
    # API request payload
    payload = {
        'model': 'llama3.2:1b',
        'prompt': prompt,
        'stream': False,
        'options': {
            'num_predict': max_tokens,
            'temperature': 0.7,  # Creativity level (0.5-0.9)
            'top_k': 40,         # Token selection constraint
            'top_p': 0.9         # Nucleus sampling
        }
    }
    
    # Make API request
    response = requests.post('http://localhost:11434/api/generate', json=payload)
    result = response.json()
    
    # Calculate response time
    elapsed = time.time() - start_time
    
    # Print result with timing
    print(f"[{elapsed:.2f}s] {character}:")
    print(f"  {result['response']}\n")
    
    return result['response']

# Test various NPC types
if __name__ == "__main__":
    print("Testing MUD NPC responses...\n")
    print("=" * 50 + "\n")
    
    # Test different character types
    get_npc_response("a goblin guard", "An adventurer approaches your post")
    get_npc_response("a dwarf merchant", "A customer asks about your weapons")
    get_npc_response("a wise old wizard", "A young mage seeks your advice")
    get_npc_response("a drunk tavern patron", "Someone sits next to you")
    get_npc_response("a mysterious hooded figure", "Someone asks who you are")
EOF

# Make executable and run
chmod +x ~/test_npc.py
python3 ~/test_npc.py
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

## API Integration Examples

### Python Integration
```python
import requests
import json

class NPCBrain:
    """Simple NPC response generator using Ollama"""
    
    def __init__(self, model="llama3.2:1b", api_url="http://localhost:11434"):
        self.model = model
        self.api_url = f"{api_url}/api/generate"
        
    def get_response(self, character_desc, situation, max_tokens=30):
        """
        Generate an NPC response
        
        Args:
            character_desc: "a grumpy dwarf merchant"
            situation: "A player asks about your wares"
            max_tokens: Maximum response length
            
        Returns:
            NPC response string
        """
        prompt = f"You are {character_desc}. {situation} Respond in character:"
        
        payload = {
            "model": self.model,
            "prompt": prompt,
            "stream": False,
            "options": {
                "num_predict": max_tokens,
                "temperature": 0.7,
                "top_k": 40,
                "top_p": 0.9
            }
        }
        
        try:
            response = requests.post(self.api_url, json=payload, timeout=5)
            return response.json()["response"]
        except Exception as e:
            return f"*{character_desc} seems confused*"  # Fallback response

# Usage example
npc = NPCBrain()
response = npc.get_response(
    "a goblin guard",
    "An adventurer approaches your post"
)
print(response)
```

### C/C++ Integration (for CircleMUD/tbaMUD)
```c
#include <curl/curl.h>
#include <string.h>
#include <stdio.h>

// Response buffer structure
struct response_buffer {
    char *data;
    size_t size;
};

// Callback for CURL
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct response_buffer *resp = (struct response_buffer *)userp;
    
    char *ptr = realloc(resp->data, resp->size + realsize + 1);
    if(!ptr) return 0;
    
    resp->data = ptr;
    memcpy(&(resp->data[resp->size]), contents, realsize);
    resp->size += realsize;
    resp->data[resp->size] = 0;
    
    return realsize;
}

// Get NPC response from Ollama
char* get_npc_response(const char* character, const char* situation) {
    CURL *curl;
    CURLcode res;
    struct response_buffer resp = {0};
    
    // Build JSON payload
    char payload[1024];
    snprintf(payload, sizeof(payload),
        "{\"model\":\"llama3.2:1b\","
        "\"prompt\":\"You are %s. %s Respond briefly:\","
        "\"stream\":false,"
        "\"options\":{\"num_predict\":30,\"temperature\":0.7}}",
        character, situation);
    
    curl = curl_easy_init();
    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/generate");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&resp);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        
        res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    // Parse JSON response (simplified - use proper JSON parser in production)
    char *response_text = NULL;
    if(resp.data) {
        char *response_start = strstr(resp.data, "\"response\":\"");
        if(response_start) {
            response_start += 12;  // Skip past "response":"
            char *response_end = strchr(response_start, '"');
            if(response_end) {
                size_t len = response_end - response_start;
                response_text = malloc(len + 1);
                strncpy(response_text, response_start, len);
                response_text[len] = '\0';
            }
        }
        free(resp.data);
    }
    
    return response_text ? response_text : strdup("*grumbles*");
}
```

## Prompt Templates for Different NPC Types

### Combat NPCs
```
You are [a fierce orc warrior]. [A player challenges you to combat]. Give a SHORT threatening response:
```

### Merchant NPCs
```
You are [a dwarf blacksmith]. [A player asks about weapons]. List 2-3 items briefly:
```

### Quest Givers
```
You are [a mysterious sage]. [A player seeks a quest]. Give a cryptic hint about adventure:
```

### Tavern NPCs
```
You are [a drunk patron]. [Someone sits next to you]. Mumble something amusing:
```

### Guard NPCs
```
You are [a city guard]. [Someone approaches the gate at night]. State your challenge:
```

## Performance Optimization Settings

### Fast Responses (< 1 second)
```json
{
  "num_predict": 20,      // Very short responses
  "temperature": 0.5,     // More predictable
  "top_k": 20,           // Fewer token choices
  "repeat_penalty": 1.1   // Avoid repetition
}
```

### Balanced (1-2 seconds)
```json
{
  "num_predict": 30,      // Short responses
  "temperature": 0.7,     // Balanced creativity
  "top_k": 40,           // Moderate variety
  "top_p": 0.9           // Good sampling
}
```

### Quality Focus (2-3 seconds)
```json
{
  "num_predict": 50,      // Longer responses
  "temperature": 0.8,     // More creative
  "top_k": 50,           // More variety
  "top_p": 0.95          // Better sampling
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

## Support and Documentation

- **Ollama Documentation**: https://ollama.ai/
- **Model Library**: https://ollama.ai/library
- **API Reference**: https://github.com/ollama/ollama/blob/main/docs/api.md
- **Community Discord**: https://discord.gg/ollama

---

*This guide installs Ollama with Llama 3.2 1B model for MUD NPC responses. The model provides good roleplay capabilities with 1-2 second response times on a 4-core CPU with 8GB RAM.*
