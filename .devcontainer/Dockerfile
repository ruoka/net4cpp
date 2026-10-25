# syntax=docker/dockerfile:1.7
FROM debian:bookworm-slim

# Install Clang 21 + libc++ + tools (required for C++23 modules and std::flat_*).
RUN apt-get update && \
    apt-get install -y --no-install-recommends ca-certificates gnupg wget && \
    wget -qO - https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /usr/share/keyrings/llvm.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/llvm.gpg] http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm-21 main" \
        > /etc/apt/sources.list.d/llvm21.list && \
    rm -rf /var/lib/apt/lists/*

RUN DEBIAN_FRONTEND=noninteractive apt-get update && \
    apt-get install -y --no-install-recommends \
        git file jq \
        clang-21 libc++-21-dev libc++abi-21-dev lld-21 clang-tools-21 lldb-21 && \
    rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-21 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-21 100 && \
    update-alternatives --install /usr/bin/lld lld /usr/bin/lld-21 100

ENV CC=clang-21 \
    CXX=clang++-21 \
    LDFLAGS="-Wl,--push-state,--no-as-needed -lc++ -lc++abi -lc++experimental -Wl,--pop-state -pthread -ldl"

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN existing_group="$(getent group "$USER_GID" | cut -d: -f1 || true)" && \
    if [ -z "$existing_group" ]; then \
        groupadd --gid "$USER_GID" "$USERNAME"; \
    elif [ "$existing_group" != "$USERNAME" ]; then \
        groupmod -n "$USERNAME" "$existing_group"; \
    fi && \
    existing_user="$(getent passwd "$USER_UID" | cut -d: -f1 || true)" && \
    if [ -n "$existing_user" ] && [ "$existing_user" != "$USERNAME" ]; then \
        usermod -l "$USERNAME" "$existing_user" && \
        usermod -d "/home/$USERNAME" -m "$USERNAME"; \
    fi && \
    if ! id -u "$USERNAME" >/dev/null 2>&1; then \
        useradd --uid "$USER_UID" --gid "$USER_GID" -m "$USERNAME"; \
    fi

USER $USERNAME