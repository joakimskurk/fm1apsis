# APSIS: the everyday commands on a Mac (Linux works the same without Docker). CI runs all of it too.
#   make test      APSIS host tests, clang (native), plain + ASan/UBSan
#   make linux     the same tests with gcc and clang MSan, and the firmware's sound in host mode (Docker)
#   make sound-update   re-pin tests/sound.sha256 after a deliberate sound change
#   make fw        the real FM-1 package build/apsis.fwsc (JieLi toolchain in ~/.jieli, SDK in ~/fw-AC79_AIoT_SDK, Docker)
#   make image     the Docker image the Linux targets use
PY ?= $(shell for p in python3 /usr/bin/python3; do $$p -c 'import PIL' 2>/dev/null && { echo $$p; break; }; done)
DOCKER = docker run --rm --platform linux/amd64 -v "$(CURDIR)":/w -w /w apsis-dev

.PHONY: test linux sound sound-update fw image clean
test:
	sh tests/apsis/run.sh
linux: image
	$(DOCKER) sh -c 'sh tests/apsis/run.sh && CC=clang SAN=0 MSAN=1 sh tests/apsis/run.sh && sh tests/sound.sh'
sound: image
	$(DOCKER) sh tests/sound.sh
sound-update: image
	$(DOCKER) sh -c 'UPDATE=1 sh tests/sound.sh'
fw:
	PYTHON=$(PY) sh build.sh
	shasum -a 256 build/apsis.fwsc
# the firmware on fm1-emu here: CI's pinned fm1-live (artifact fm1-live-linux-x86_64 in build/emu-bin) in Docker.
# make emu [STRESS=1] [PROFILE=1] [FW=build/apsis.fwsc ELF=build/apsis.elf]
FW ?= build/apsis.fwsc
ELF ?= build/apsis.elf
EMU_OUT ?= build/emu-local
emu:
	@test -x build/emu-bin/fm1-live || { echo "fetch it: gh run download <run> -n fm1-live-linux-x86_64 -D build/emu-bin"; exit 1; }
	@docker image inspect apsis-emu >/dev/null 2>&1 || docker build -q --platform linux/amd64 -t apsis-emu tools/docker/emu
	docker run --rm --platform linux/amd64 -v "$(CURDIR)":/w -v $(HOME)/.jieli/toolchain:/jieli:ro -w /w \
	    -e STRESS=$(STRESS) -e PROFILE=$(PROFILE) -e EMU_MS=$(EMU_MS) -e EMU_ARGS="$(EMU_ARGS)" -e PROF_FROM=$(PROF_FROM) -e PROF_TO=$(PROF_TO) -e JIELI_TOOLCHAIN=/jieli -e EMU=build/emu-bin/fm1-live \
	    apsis-emu sh tests/emu.sh $(FW) $(ELF) $(EMU_OUT)
image:
	@docker image inspect apsis-dev >/dev/null 2>&1 || docker build -q --platform linux/amd64 -t apsis-dev tools/docker
clean:
	rm -rf build
