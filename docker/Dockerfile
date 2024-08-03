# 使用Ubuntu 18.04作为基础镜像
FROM ubuntu:18.04

# 避免交互
ENV DEBIAN_FRONTEND=noninteractive

# 安装必要的软件包，‌包括SSH服务器
RUN apt-get update && apt-get install -y \
    openssh-server \
    # 其他需要的软件包
    build-essential \
    gdb \
    cmake \
    libpcl-dev \
    libvtk6-dev \
    libnlopt-dev \
    libopencv-dev \
    libmysqlclient-dev \
    mysql-server \
    mysql-client \
    git

RUN apt-get update && apt-get install -y \
    unzip \
    vim

# RUN git clone https://github.com/redis/hiredis.git
# RUN git clone https://github.com/sewenew/redis-plus-plus.git
COPY hiredis /home/hiredis
COPY redis-plus-plus /home/redis-plus-plus

WORKDIR /home/hiredis
RUN make
RUN make install
RUN apt-get install -y libhiredis-dev

WORKDIR /home/redis-plus-plus/
RUN mkdir build
WORKDIR build
RUN cmake ..
RUN make
RUN make install

# 暴露SSH端口
EXPOSE 22

# 配置SSH服务器
RUN mkdir /var/run/sshd
RUN echo 'root:320510' | chpasswd
RUN sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config

WORKDIR /
 
# 开机指令
CMD /usr/sbin/sshd; \
    chmod +x /tmp/start.sh; \
    /tmp/start.sh;