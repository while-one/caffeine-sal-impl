#!/bin/bash
TARGET_STAGE=${1:-native}
CMAKE_TARGET=${2:-all}
shift 2
EXTRA_ARGS="$@" # All remaining arguments

IMAGE="build-$TARGET_STAGE"

# The image is built and pushed by the caffeine-hal repository
REPO_OWNER=${GITHUB_REPOSITORY_OWNER:-while-one}
DOCKER_IMAGE="ghcr.io/${REPO_OWNER}/caffeine-hal/$IMAGE:latest"

# Pull the image to ensure it's up-to-date
echo "Pulling Docker image: $DOCKER_IMAGE"
docker pull "$DOCKER_IMAGE" || { echo "Failed to pull image $DOCKER_IMAGE. Please ensure it's built and pushed from caffeine-hal."; exit 1; }

# Run build in container
docker run --rm -v "$(pwd)":/work -w /work "$DOCKER_IMAGE" \
    bash -c "rm -rf build && cmake -B build -DFETCHCONTENT_FULLY_DISCONNECTED=OFF $EXTRA_ARGS && cmake --build build --target $CMAKE_TARGET"
