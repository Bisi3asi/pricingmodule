FROM rockylinux:8

# 기본 업데이트 및 EPEL 설치
RUN dnf -y update && \
    dnf -y install epel-release && \
    dnf clean all

# Development Tools 그룹 및 필수 패키지 설치
RUN dnf -y groupinstall "Development Tools" && \
    dnf -y install cmake gcc-c++ git make gdb which && \
    dnf clean all

WORKDIR /workspace

CMD ["/bin/bash"]