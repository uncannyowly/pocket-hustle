# The toolchain image plus qemu-user, to run the Miyoo (32-bit ARM, glibc 2.28) build on an x86 host.
# Built by scripts/emu-play.sh on first use as pocket-hustle-emu; nothing is installed on the host.
FROM gta1-miyoo-toolchain
RUN apt-get update \
 && apt-get install -y --no-install-recommends qemu-user \
 && rm -rf /var/lib/apt/lists/*
