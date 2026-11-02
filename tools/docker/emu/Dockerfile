# fm1-live (CI's pinned build, artifact fm1-live-linux-x86_64) needs a recent glibc and its runtime libraries
FROM ubuntu:latest
RUN apt-get update -qq && apt-get install -y -qq libasound2t64 libx11-6 libxkbcommon0 libxcursor1 libxrandr2 libxi6 python3 > /dev/null && rm -rf /var/lib/apt/lists/*
