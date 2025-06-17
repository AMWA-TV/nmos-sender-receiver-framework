FROM ubuntu:22.04

# Versions
ENV CONAN_VERSION=2.4.0
ENV CMAKE_VERSION=3.28.4

# To make it easier for build and release pipelines to run apt-get,
# configure apt to not require confirmation (assume the -y argument by default)
ENV DEBIAN_FRONTEND=noninteractive
RUN echo "APT::Get::Assume-Yes "true";" > /etc/apt/apt.conf.d/90assumeyes

RUN apt-get update

RUN apt-get install -y \
    vim clang-format rsync gdb gdbserver x11-apps xauth iproute2 build-essential
RUN apt-get install -y libgtk-3-dev ninja-build
ENV PATH="/opt/cmake/bin:${PATH}"
RUN apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x \
    gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
RUN apt-get install avahi-utils htop make

# GCC
RUN apt -y install gcc-12 g++-12
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100
RUN apt-get autoremove


COPY scripts /opt/nmos/scripts
RUN /bin/bash /opt/nmos/scripts/build-tools/add-user-nmos.sh
RUN /bin/bash /opt/nmos/scripts/build-tools/install-fish.sh
RUN /opt/nmos/scripts/build-tools/install-git.sh
RUN /opt/nmos/scripts/build-tools/install-conan.sh
RUN /opt/nmos/scripts/build-tools/install-cmake-x86.sh
