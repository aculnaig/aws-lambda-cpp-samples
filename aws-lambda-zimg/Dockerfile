FROM archlinux:latest AS builder

RUN pacman-key --init
RUN pacman -Syu --noconfirm
RUN pacman -S --noconfirm gcc cmake git make zip curl pkgconf autoconf gettext automake libtool zlib
WORKDIR /home
RUN git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp.git
RUN cd aws-sdk-cpp && \
    mkdir -p build && \
    cd build && \
    cmake .. -DBUILD_ONLY="core" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCUSTOM_MEMORY_MANAGEMENT=OFF -DCMAKE_INSTALL_PREFIX=/usr && \
    make -j8 && \
    make install
WORKDIR /home
RUN git clone https://github.com/awslabs/aws-lambda-cpp.git
RUN cd aws-lambda-cpp && \
    mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr && \
    make -j8 && \
    make install
WORKDIR /home
RUN git clone https://github.com/sekrit-twc/zimg
RUN cd zimg && \
    git submodule init graphengine && \
    git submodule update && \
    ./autogen.sh && \
    ./configure --prefix=/usr && \
    make -j8 && \
    make install

WORKDIR /opt
COPY . .
RUN cmake . -DCMAKE_BUILD_TYPE=Release -DLOG_VERBOSITY=3
RUN make
RUN make aws-lambda-package-zimg-api

FROM archlinux:latest AS api
WORKDIR /opt
COPY --from=builder /opt/zimg-api.zip .
CMD ["sh"]
