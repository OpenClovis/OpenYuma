[![Build Status](https://travis-ci.org/OpenClovis/OpenYuma.png?branch=master)](https://travis-ci.org/OpenClovis/OpenYuma)

OpenYuma
========

Fork of the defacto standard Yuma project since it went proprietary.  Netconf
is a management protocol (similar to SNMP) defined in RFC4741.


Testing with docker
-------------------

This repository has a `Dockerfile` that can be used to create a container that
builds openyuma and starts the service. You need a linux with working [docker]
installation to use it.

To build the container:
~~~
docker build -t openyuma .
~~~

To start it:
~~~
docker run -it --rm -p 8300:830 -p 2200:22 --name openyuma openyuma
~~~

The line above maps openyuma's netconf port to 8300 on the host. You can
connect to that port with ncclient.

To use *yangcli* as mentioned above, you `docker exec openyuma /bin/bash` to
enter the running container. Use *admin* as both the user and password.

