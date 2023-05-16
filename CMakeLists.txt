# -----------------------------------------------------------------------------
# 
# -----------------------------------------------------------------------------

# ...
cmake_minimum_required(VERSION 3.0)

# ...
project(libuvc_server)
set(CMAKE_CXX_STANDARD 17)

# ...
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ_PKG IMPORTED_TARGET libzmq)
pkg_check_modules(CZMQ_PKG IMPORTED_TARGET libczmq)
pkg_check_modules(USB_PKG IMPORTED_TARGET libusb-1.0)
pkg_check_modules(JPEG_PKG IMPORTED_TARGET libjpeg)

# ...
include_directories(
  "./lib/libuvc/include"
  "./lib/argparse/include"
)

# ...
add_executable(${PROJECT_NAME}
  "src/main.cpp"
  "./lib/libuvc/src/ctrl-gen.c"
  "./lib/libuvc/src/ctrl.c"
  "./lib/libuvc/src/device.c"
  "./lib/libuvc/src/diag.c"
  "./lib/libuvc/src/frame-mjpeg.c"
  "./lib/libuvc/src/frame.c"
  "./lib/libuvc/src/init.c"
  "./lib/libuvc/src/misc.c"
  "./lib/libuvc/src/stream.c"
)

# ...
target_link_libraries(
  ${PROJECT_NAME}
  PkgConfig::USB_PKG
  PkgConfig::ZMQ_PKG
  PkgConfig::CZMQ_PKG
  PkgConfig::JPEG_PKG
)
