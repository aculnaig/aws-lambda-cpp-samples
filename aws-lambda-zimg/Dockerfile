FROM archlinux:latest AS builder

RUN pacman-key --init
RUN pacman -Syu --noconfirm
RUN pacman -S --noconfirm gcc cmake git make zip curl pkgconf autoconf gettext automake libtool
WORKDIR /home
RUN git clone https://github.com/awslabs/aws-lambda-cpp.git && \
    cd aws-lambda-cpp && \
    mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr && \
    make && \
    make install
WORKDIR /home
RUN git clone https://github.com/sekrit-twc/zimg && \
    cd zimg && \
    git submodule init graphengine && \
    git submodule update && \
    ./autogen.sh && \
    ./configure --prefix=/usr && \
    make && \
    make install

WORKDIR /opt
COPY . .
RUN cmake . -DCMAKE_BUILD_TYPE=Release -DLOG_VERBOSITY=3
RUN make
RUN make aws-lambda-package-zimg-api

FROM scratch
WORKDIR /opt
COPY --from=builder /opt/zimg-api.zip .
