# Complete Aider + OpenRouter Setup Guide
*From zero to AI-powered coding with free models*

## Prerequisites
- Ubuntu/Linux system (tested on Ubuntu 24.04)
- Python 3 installed
- Git repository for your project
- Internet connection

## Step 1: Install Python pip and venv
```bash
# Update package list
sudo apt update

# Install pip and venv
sudo apt install python3-pip python3-venv

# Note: You'll get an "externally-managed-environment" error if you try to use pip directly
# This is normal on Ubuntu 24.04 - we'll use a virtual environment instead
```

## Step 2: Create Virtual Environment
```bash
# Navigate to your project directory
cd ~/your-project-directory

# Create a virtual environment for aider
python3 -m venv aider-env

# Activate the virtual environment
source aider-env/bin/activate

# Your prompt should now show (aider-env) at the beginning
```

## Step 3: Install Aider
```bash
# With virtual environment activated, install aider
pip install aider-chat

# Verify installation
aider --version
```

## Step 4: Get OpenRouter API Key
1. Go to https://openrouter.ai
2. Sign up for a free account
3. Navigate to the **Keys** section
4. Click **Create Key**
5. Name your key (e.g., "Aider Development")
6. Copy the key (starts with `sk-or-v1-...`)
7. **Save this key securely!**

## Step 5: Configure Git (if needed)
```bash
# Set your git identity
git config user.name "Your Name"
git config user.email "your-email@example.com"
```

## Step 6: Create Aider Configuration File
```bash
# In your project directory, create the config file
cat > .aider.conf.yml << 'EOF'
openai-api-base: https://openrouter.ai/api/v1
api-key: openrouter=sk-or-v1-YOUR_ACTUAL_KEY_HERE
model: openrouter/moonshotai/kimi-k2:free
EOF

# IMPORTANT: Replace sk-or-v1-YOUR_ACTUAL_KEY_HERE with your actual OpenRouter API key!
```

### Alternative Free Models
If Kimi K2 isn't available, use one of these in your config:
- `openrouter/deepseek/deepseek-r1:free` (Excellent reasoning model)
- `openrouter/deepseek/deepseek-chat:free` (Great for coding)
- `openrouter/google/gemini-2.0-flash-exp:free` (Fast and capable)
- `openrouter/qwen/qwen-2.5-coder-32b-instruct:free` (Specialized for code)

## Step 7: Add to .gitignore
```bash
# Add aider files and virtual environment to gitignore
echo "
# Aider
aider-env/
.aider*
" >> .gitignore
```

## Step 8: Test the Connection

### Quick Test
```bash
# Test with a simple message
aider --message "Say hello if you're working"
```

### List Available Models
```bash
# See all OpenRouter models
aider --list-models openrouter/

# See only free models
aider --list-models free
```

## Step 9: Start Using Aider

### Basic Usage
```bash
# Start aider in your project
aider

# You'll see something like:
# Aider v0.85.5
# Model: openrouter/moonshotai/kimi-k2:free with whole edit format
# Git repo: .git with 581 files
# > 

# Now you can interact with it:
> /help                          # Show all commands
> /add src/main.c                # Add a file to work with
> explain this code              # Ask questions
> add error handling to line 42  # Request code changes
> /commit                        # Commit changes with AI-generated message
> /exit                          # Exit aider
```

### Working with Specific Files
```bash
# Start aider with specific files
aider src/main.c src/utils.c

# Or add files after starting
> /add src/*.c
> /add README.md
```

## Step 10: Verify Free Tier Usage
1. Go to https://openrouter.ai/dashboard
2. Check your **Credits/Balance** - should be unchanged
3. Look at **Usage/Activity** - should show $0.00 for requests
4. Ensure model names have `:free` suffix

## Common Commands Reference

| Command | Description |
|---------|-------------|
| `/add <file>` | Add files to the chat context |
| `/drop <file>` | Remove files from context |
| `/ls` | List files in context |
| `/diff` | Show pending changes |
| `/commit [message]` | Commit changes |
| `/undo` | Undo last AI change |
| `/clear` | Clear chat history |
| `/help` | Show all commands |
| `/exit` | Exit aider |

## Troubleshooting

### "LLM Provider NOT provided" Error
- Make sure model name starts with `openrouter/`
- Correct: `openrouter/deepseek/deepseek-chat:free`
- Wrong: `deepseek/deepseek-chat:free`

### API Key Issues
- Ensure you replaced the placeholder with your actual key
- Check key is set correctly: `cat .aider.conf.yml`
- Try setting environment variable: `export OPENROUTER_API_KEY="your-key"`

### Model Not Available
- Run `aider --list-models free` to see available free models
- Update your config to use an available model
- Check OpenRouter dashboard for regional restrictions

### Virtual Environment Not Active
- Make sure you see `(aider-env)` in your prompt
- Reactivate with: `source aider-env/bin/activate`
- To make permanent, add to `~/.bashrc`

## Daily Workflow
```bash
# Navigate to project
cd ~/your-project

# Activate virtual environment
source aider-env/bin/activate

# Start aider
aider

# Work with AI assistance
> /add src/feature.c
> implement the TODO on line 45
> /commit

# Exit when done
> /exit
```

## Free Tier Limits
- Usually 10-200 requests per day
- Rate limits around 10 requests/minute
- No cost as long as within limits
- Check dashboard regularly to monitor usage

## Tips for Success
1. Start with simple requests to test the connection
2. Be specific in your prompts for better results
3. Use `/add` to give context before asking questions
4. Review changes with `/diff` before committing
5. Keep your API key secure - never commit it to git
6. Monitor your OpenRouter dashboard for usage

## Next Steps
- Explore aider's advanced features: https://aider.chat/docs/
- Try different free models to find your favorite
- Set up custom prompts and templates
- Integrate with your development workflow

---
*Remember: Always verify you're using free tier models (with `:free` suffix) and monitor your OpenRouter dashboard to ensure no charges.*