# [Choice] Ubuntu version
ARG VARIANT="kinetic"
FROM buildpack-deps:${VARIANT}-curl

# [Optional] Uncomment this section to install additional OS packages.
RUN export DEBIAN_FRONTEND=noninteractive \
    && apt-get update \
    && apt-get -y install --no-install-recommends wget software-properties-common \
    && wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
    && apt-add-repository "deb http://apt.llvm.org/kinetic/ llvm-toolchain-kinetic-15 main" \
    && apt-get update \
    && apt-get -y install --no-install-recommends clang-15 lldb-15 lld-15 libc++-15-dev libc++abi-15-dev git make cmake cppcheck valgrind

# https://code.visualstudio.com/remote/advancedcontainers/add-nonroot-user
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME

# [Optional] Set the default user. Omit if you want to keep the default as root.
USER $USERNAME
