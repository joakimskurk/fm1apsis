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
image:
	@docker image inspect apsis-dev >/dev/null 2>&1 || docker build -q --platform linux/amd64 -t apsis-dev tools/docker
clean:
	rm -rf build
