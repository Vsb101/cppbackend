# Не просто создаём образ, но даём ему имя build
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

WORKDIR /app
COPY conanfile.txt /app/
RUN mkdir build && cd build && \
    conan install .. --build=missing -s compiler.libcxx=libstdc++11 -s build_type=Release

COPY src /app/src
COPY tests /app/tests
COPY CMakeLists.txt /app/CMakeLists.txt

RUN cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

ENTRYPOINT ["./build/collision_detection_tests"]