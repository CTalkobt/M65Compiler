#!/bin/bash
#
## Pull the model
mode=codellama:34b

ollama pull $model

# Start your ollama server, increasing the context window to 8k tokens
OLLAMA_CONTEXT_LENGTH=8192 ollama serve

aider --model ollama_chat/$model
