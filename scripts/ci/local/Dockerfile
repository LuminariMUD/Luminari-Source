FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive LANG=C.UTF-8 LC_ALL=C.UTF-8
RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf automake build-essential ca-certificates ccache clang cmake curl gcc-14 g++-14 libclang-rt-18-dev \
    git iproute2 libcurl4-openssl-dev libevent-dev libgd-dev libjson-c-dev \
    libmariadb-dev libssl-dev libtool mariadb-client mariadb-server netcat-openbsd \
    pandoc pkg-config procps python3 python3-pip python3-venv ripgrep sudo time \
    valgrind zlib1g-dev && rm -rf /var/lib/apt/lists/*
RUN python3 -m venv /opt/ci-python
ENV PATH=/usr/lib/ccache:/opt/ci-python/bin:$PATH PRE_COMMIT_HOME=/opt/pre-commit
COPY scripts/help-sync/requirements.txt /tmp/help-requirements.txt
RUN pip install 'gcovr==8.6' pre-commit PyYAML -r /tmp/help-requirements.txt
COPY .pre-commit-config.yaml /tmp/hooks/.pre-commit-config.yaml
RUN cd /tmp/hooks && git init -q && pre-commit install-hooks && \
    chown -R 1000:1000 /opt/pre-commit && rm -rf /tmp/hooks
RUN curl -fsSL https://github.com/gitleaks/gitleaks/releases/download/v8.21.2/gitleaks_8.21.2_linux_x64.tar.gz \
    -o /tmp/gitleaks.tar.gz && tar -xzf /tmp/gitleaks.tar.gz -C /usr/local/bin gitleaks && \
    rm /tmp/gitleaks.tar.gz
USER 1000:1000
