FROM rockylinux:8

# 기본 업데이트 및 EPEL 설치
RUN dnf -y update && \
    dnf -y install epel-release && \
    dnf clean all

# Development Tools 그룹 및 필수 패키지 설치
RUN dnf -y groupinstall "Development Tools" && \
    dnf -y install wget tar gzip unzip bzip2 python3 && \
    dnf clean all -y

# Ninja 설치 (v1.11.1)
RUN NINJA_VERSION=1.11.1 && \
    curl -L https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip -o /tmp/ninja.zip && \
    unzip /tmp/ninja.zip -d /usr/local/bin/ && \
    chmod +x /usr/local/bin/ninja && \
    rm -f /tmp/ninja.zip

# CMake 설치 (v.3.27.7)
RUN CMAKE_VERSION=3.27.7 && \
    curl -L https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh -o /tmp/cmake.sh && \
    sh /tmp/cmake.sh --skip-license --prefix=/usr/local && \
    rm -f /tmp/cmake.sh

# 버전 변수 설정
ARG BOOST_VERSION=1.87.0
ARG QL_VERSION=1.37

# Boost (v.${BOOST_VERSION}) 소스 빌드 및 설치
RUN BOOST_VERSION_UNDERSCORE=$(echo ${BOOST_VERSION} | tr . _) && \
    BOOST_INSTALL_PREFIX=/usr/lib/boost_${BOOST_VERSION_UNDERSCORE} && \
    echo "--- Installing Boost ${BOOST_VERSION} ---" && \
    cd /tmp && \
    curl -fL https://downloads.sourceforge.net/project/boost/boost/${BOOST_VERSION}/boost_${BOOST_VERSION_UNDERSCORE}.tar.gz -o boost.tar.gz && \
    tar -xzf boost.tar.gz && \
    cd boost_${BOOST_VERSION_UNDERSCORE} && \
    ./bootstrap.sh --prefix=${BOOST_INSTALL_PREFIX} && \
    ./b2 install -j$(nproc) && \
    echo "--- Install Boost Complete ---" && \
    rm -rf /tmp/boost*

# QuantLib (v.${QL_VERSION}) 소스 빌드 및 설치
RUN BOOST_VERSION_UNDERSCORE=$(echo ${BOOST_VERSION} | tr . _) && \
    BOOST_INSTALL_PREFIX=/usr/lib/boost_${BOOST_VERSION_UNDERSCORE} && \
    QL_INSTALL_PREFIX=/usr/lib/QuantLib_${QL_VERSION} && \
    echo "--- Installing QuantLib ${QL_VERSION} ---" && \
    cd /tmp && \
    curl -fL https://github.com/lballabio/QuantLib/releases/download/v${QL_VERSION}/QuantLib-${QL_VERSION}.tar.gz -o quantlib.tar.gz && \
    tar -xzf quantlib.tar.gz && \
    cd QuantLib-${QL_VERSION} && \
    mkdir build && cd build && \
    cmake .. \
        -DCMAKE_INSTALL_PREFIX=${QL_INSTALL_PREFIX} \
        -DBOOST_ROOT=${BOOST_INSTALL_PREFIX} \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=OFF \
        -DQL_BUILD_EXAMPLES=OFF \
        -DQL_BUILD_TEST_SUITE=OFF && \
    make -j$(nproc) install && \
    echo "--- Install QuantLib Complete ---" && \
    rm -rf /tmp/QuantLib*

# Custom config
RUN echo "alias ll='ls -l'" >> /etc/bashrc

WORKDIR /workspace

CMD ["/bin/bash"]