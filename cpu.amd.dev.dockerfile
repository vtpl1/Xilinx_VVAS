FROM xilinx/v70-video-analytics-tf2-cpu:ubuntu2004-1.0.0

RUN apt update && apt install -y build-essential zlib1g-dev libncurses5-dev libgdbm-dev libnss3-dev libreadline-dev libffi-dev wget \
        indent git autoconf autopoint libtool bison flex yasm \
		libssl-dev python3 python3-pip python3-setuptools python3-wheel \
		ninja-build cmake libxext-dev libpango1.0-dev libgdk-pixbuf2.0-dev libavfilter-dev

# RUN mkdir ./python \
#     && cd ./python \
#     && wget https://www.python.org/ftp/python/3.11.0/Python-3.11.0.tgz \
#     && tar -xvf Python-3.11.0.tgz \
#     && cd Python-3.11.0 \
#     && sh ./configure --enable-optimizations \
#     && make install
    
# RUN update-alternatives --install /usr/local/bin/python python /usr/local/bin/python3.11 3

RUN pip3 install -U pip
RUN apt-get -y install --no-install-recommends libfreeimage-dev python3-pip
RUN pip install -U pip
RUN pip install -U poetry
RUN apt -y install --no-install-recommends libopencv-dev python3-opencv
RUN pip install -U pynng
ENV SHELL /bin/bash
