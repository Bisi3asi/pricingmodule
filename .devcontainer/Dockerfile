FROM rockylinux:8

# 기본 업데이트 및 EPEL 설치
RUN dnf -y update && \
    dnf -y install epel-release && \
    dnf clean all

# Development Tools 그룹 및 필수 패키지 설치
RUN dnf -y groupinstall "Development Tools" && \
    # 포함 패키지 : cmake gcc-c++ make gdb which
    dnf clean all

# Ninja 설치 (v1.11.1)
RUN curl -L https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip -o /tmp/ninja.zip && \
    unzip /tmp/ninja.zip -d /usr/local/bin/ && \
    chmod +x /usr/local/bin/ninja && \
    rm -f /tmp/ninja.zip

# CMake 설치 (v.3.27.7)
RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7-linux-x86_64.sh -o /tmp/cmake.sh && \
    sh /tmp/cmake.sh --skip-license --prefix=/usr/local && \
    rm -f /tmp/cmake.sh

# Custom config
RUN echo "alias ll='ls -l'" >> /etc/bashrc

WORKDIR /workspace

CMD ["/bin/bash"]