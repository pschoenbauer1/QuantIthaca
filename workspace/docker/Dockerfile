FROM ubuntu:24.04

ARG PYTHON_VERSION=3.12
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    clang \
    clangd \
    clang-format \
    clang-tidy \
    lldb \
    gdb \
    valgrind \
    cmake \
    ninja-build \
    pkg-config \
    ccache \
    curl \
    git \
    ripgrep \
    fd-find \
    jq \
    tree \
    less \
    zip \
    unzip \
    ca-certificates \
    python3-dev \
    && rm -rf /var/lib/apt/lists/*

RUN ln -s /usr/bin/fdfind /usr/local/bin/fd

RUN curl -LsSf https://astral.sh/uv/install.sh | sh

ENV PATH="/root/.local/bin:${PATH}"
ENV CC=clang
ENV CXX=clang++

# Optional: make uv globally available
RUN ln -s /root/.local/bin/uv /usr/local/bin/uv

# Lightweight non-root user
RUN useradd -m -s /bin/bash claude

WORKDIR /workspace

RUN mkdir -p /workspace && \
    chown -R claude:claude /workspace

USER claude

RUN uv python install ${PYTHON_VERSION}

CMD ["bash"]