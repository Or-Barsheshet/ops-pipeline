FROM ubuntu:24.04

RUN apt update && apt install -y \
    build-essential \
    gcc \
    make \
    bash \
    vim \
    wget \
    curl \
    git \
    libpthread-stubs0-dev \
    sudo

RUN useradd -ms /bin/bash vscode && \
    echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

WORKDIR /project

CMD ["/bin/bash"]