FROM ubuntu:24.04

# Set up apt and get tools and SDKs necessary for projects
RUN apt upgrade && apt update

# Get HW build tools
RUN apt-get update && apt-get install -y yosys nextpnr-ecp5 fpga-icestorm iverilog git fpga-trellis fpga-trellis-database libusb-1.0-0-dev pkg-config

# Get SW build tools & PICO SDK
RUN apt-get update && apt-get install -y openocd gdb-multiarch
RUN mkdir /pico
WORKDIR /pico
RUN git clone https://github.com/raspberrypi/pico-sdk.git --branch master
WORKDIR /pico/pico-sdk
RUN git submodule update --init
WORKDIR /pico
RUN git clone https://github.com/raspberrypi/pico-examples.git --branch master
RUN apt update
RUN apt-get update && apt-get install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
RUN apt-get update && apt-get install -y g++ libstdc++-arm-none-eabi-newlib
WORKDIR  /pico/pico-sdk
RUN git pull
RUN git submodule update

ENV PICO_SDK_PATH=/pico/pico-sdk

RUN mkdir -p /myprojects
WORKDIR /myprojects

# ENTRYPOINT [ "tail", "-f", "/dev/null"]

CMD ["/bin/bash"]