# 矩形检测
- 环境：Ubuntu 22.04
- OpenCV：4.9.0（源码编译安装到 /usr/local）
- 编译器：g++ 11.4，C++17
- 依赖：build-essential cmake pkg-config libgtk-3-dev libavcodec-dev libavformat-dev libswscale-dev libjpeg-dev libpng-dev libtiff-dev
## 编译
g++ -std=c++17 main.cpp -o rect_detect `pkg-config --cflags --libs opencv4`
## 运行
./rect_detect /input /out