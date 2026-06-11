# cybercamp
Cybercamp files


# Fixing ollama

sudo systemctl edit ollama.service

## Add the following lines
[Service]
Environment="OLLAMA_HOST=0.0.0.0:11434"

# Restart ollama service
sudo systemctl daemon-reload
sudo systemctl restart ollama


## nano commands
1. Make changes
2. Save CTRL+o
3. Exit CTRL+X
