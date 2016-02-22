FROM ubuntu:14.04

# install required packages
ENV DEBIAN_FRONTEND noninteractive
RUN ["apt-get", "update"]
RUN ["apt-get", "install", "-y", "git", "autoconf", "gcc", "libtool", "libxml2-dev", "libssl-dev", "make", "libncurses5-dev", "libssh2-1-dev", "openssh-server"]

# setup admin user that openyuma will use
RUN set -x -e; \
    mkdir /var/run/sshd; \
    adduser --gecos '' --disabled-password admin; \
    echo "admin:admin" | chpasswd

# copy and build openyuma, setup the container to start it by default
COPY . /usr/src/OpenYuma
WORKDIR /usr/src/OpenYuma
RUN set -x -e; \
    make; \
    sudo make install; \
    touch /tmp/startup-cfg.xml; \
    printf 'Port 830\nSubsystem netconf "/usr/sbin/netconf-subsystem --ncxserver-sockname=830@/tmp/ncxserver.sock"\n' >> /etc/ssh/sshd_config; \
    printf '#!/bin/bash\nset -e -x\n/usr/sbin/netconfd --startup=/tmp/startup-cfg.xml --superuser=admin &\nsleep 1\n/usr/sbin/sshd -D &\nwait\nkill %%\n' > /root/start.sh; \
    chmod 0755 /root/start.sh
CMD ["/root/start.sh"]

# finishing touches
EXPOSE 22
EXPOSE 830
