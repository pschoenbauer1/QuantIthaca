FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

ARG NODE_VERSION=20

RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 \
    python3-pip \
    python3-venv \
    ripgrep \
    fd-find \
    jq \
    tree \
    less \
    zip \
    unzip \
    clangd \
    clang-format \
    clang-tidy \
    lldb \
    valgrind \
    pkg-config \
    ccache \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://deb.nodesource.com/setup_${NODE_VERSION}.x | bash - \
    && apt-get install -y nodejs \
    && npm install -g @anthropic-ai/claude-code \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /home/vscode/.claude \
    && chown -R vscode:vscode /home/vscode/.claude

USER vscode