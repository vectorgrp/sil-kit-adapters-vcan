FROM ubuntu:latest

SHELL ["/bin/bash", "-c"]

ENV DEBIAN_FRONTEND=noninteractive

ENV TZ=Europe/Paris

RUN echo "[Installing packages on Docker image...]"
RUN apt update && apt install -y clang iproute2 unzip cmake findutils can-utils kmod

WORKDIR /adapter-demo
