# FROM fedora:rawhide
FROM ghcr.io/fuquery/fedora_p2996:latest

# Install build tools and dependencies
RUN dnf install -y \
    cmake \
    git \
    python3-pip \
    zsh \
    tmux \
    && dnf clean all

# Install conan
RUN pip3 install --no-cache-dir conan

# Set zsh as default shell
RUN chsh -s /bin/zsh root
