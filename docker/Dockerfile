FROM ubuntu:24.04

ARG PYTHON_VERSION=3.12
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    clang \
    cmake \
    curl \
    git \
    ninja-build \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN curl -LsSf https://astral.sh/uv/install.sh | sh
ENV PATH="/root/.local/bin:${PATH}"

WORKDIR /workspace
COPY . .

RUN uv python install ${PYTHON_VERSION}
RUN uv build --wheel --python ${PYTHON_VERSION} --out-dir wheelhouse

CMD ["bash"]
