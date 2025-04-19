FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

# Install clangd.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends clangd
