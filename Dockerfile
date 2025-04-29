FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    build-essential \
    ca-certificates \
    libstdc++6 \
    gcc \
    make \
    wget \
    tar \
    libssl-dev \
    file \
    curl

WORKDIR /sdk

RUN wget -q https://github.com/trimui/toolchain_sdk_smartpro/releases/download/20231018/aarch64-linux-gnu-7.5.0-linaro.tgz && \
    tar -xzf aarch64-linux-gnu-7.5.0-linaro.tgz -C /sdk && \
    rm aarch64-linux-gnu-7.5.0-linaro.tgz

RUN wget -q https://github.com/trimui/toolchain_sdk_smartpro/releases/download/20231018/SDK_usr_tg5040_a133p.tgz && \
    tar -xzf SDK_usr_tg5040_a133p.tgz -C /sdk && \
    rm SDK_usr_tg5040_a133p.tgz

RUN wget -q https://github.com/trimui/toolchain_sdk_smartpro/releases/download/20231018/SDL2-2.26.1.GE8300.tgz && \
    tar -xzf SDL2-2.26.1.GE8300.tgz -C /sdk && \
    rm SDL2-2.26.1.GE8300.tgz

ENV PATH="/sdk/aarch64-linux-gnu-7.5.0-linaro/bin:$PATH"

ENV SYSROOT="/sdk/usr"
ENV SDL_DIR="/sdk/SDL2-2.26.1"

WORKDIR /app

CMD ["/bin/bash"]