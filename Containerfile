# Container image for building ESP-IDF projects with Podman

ARG BASE_IMAGE=debian:bookworm
FROM ${BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive \
    LC_ALL=C.UTF-8 \
    LANG=C.UTF-8 \
    PYTHONIOENCODING=utf-8 \
    IDF_PATH=/workspace/software/esp_idf \
    WORKSPACE=/workspace

# System dependencies for ESP-IDF toolchain and general build tools
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       ca-certificates \
       git \
       wget \
       curl \
       build-essential \
       cmake \
       ninja-build \
       ccache \
       flex \
       bison \
       gperf \
       libffi-dev \
       libssl-dev \
       dfu-util \
       libusb-1.0-0 \
       usbutils \
       udev \
       xz-utils \
       python3 \
       python3-venv \
       python3-pip \
       python3-setuptools \
       python3-wheel \
       python3-serial \
       unzip \
       tar \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user matching typical host uid/gid; use with --userns=keep-id
ARG USERNAME=builder
ARG USER_UID=1000
ARG USER_GID=1000
RUN groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} \
    && usermod -aG dialout ${USERNAME}

# Workdir for the mounted repository
RUN mkdir -p ${WORKSPACE} && chown -R ${USERNAME}:${USERNAME} ${WORKSPACE}
WORKDIR ${WORKSPACE}

# Copy and register entrypoint
COPY container/esp-idf-entrypoint.sh /usr/local/bin/esp-idf-entrypoint.sh
RUN chmod +x /usr/local/bin/esp-idf-entrypoint.sh

ENTRYPOINT ["/usr/local/bin/esp-idf-entrypoint.sh"]
CMD ["bash"]


