# libuvc_server

Connects to a [UVC] compatible USB camera and publishes live frames over [ZeroMQ] socket.

## Problem

Let's say you want to use some specific webcam with Python to perform some image processing tasks.

You'll probably use OpenCV to perform most of these image processing tasks.

While using OpenCV, you can also grab images from, almost, any webcam via [VideoCapture] class **but** the problem is low level exposure / gain / brightness / FPS / etc. controls doesn't work for every webcam out there with [VideoCapture] methods.

Also, you still want to use Python.

## Solution

[libuvc] is a cross-platform C library for USB video devices, built on top of libusb. It enables fine-grained control over USB video devices exporting the standard USB Video Class (UVC) interface.

You can access all sort of low level (exposure, gain, brightness, gain, focus, specific frame rates etc.) details of your camera using [libuvc] directly.

So, I created this small `libuvc_server` executable as a glue program that:

* Connects to a *specific* camera defined by USB vendor id, product id and serial number pair
* Sets specific parameters of the camera at start
* Publishes each grabbed frame from the camera over [ZeroMQ] socket for other programs to receive

After you run `libuvc_server` as a process, you can receive the frames from your webcam with [ZeroMQ] using Python.

## Usage

After compilation, run the server from the command line:

	Usage: libuvc_server [--help] [--version] --width VAR --height VAR --fps VAR --exposure VAR --gain VAR --vid VAR --pid VAR [--serial VAR]

	Optional arguments:
	  -h, --help    shows help message and exits
	  -v, --version prints version information and exits
	  --width       Frame Wdith [required]
	  --height      Frame Height [required]
	  --fps         Frames Per Second [required]
	  --exposure    Exposure Time (miliseconds) [required]
	  --gain        Sensor Gain [1, 65335] [required]
	  --vid         USB Vendor ID [required]
	  --pid         USB Product ID [required]
	  --serial      USB Device Serial Number [default: ""]

Example run with Logitech C270 camera is as follows:

	sudo ./libuvc_server --vid 1133 --pid 2085 --width 1280 --height 960 --fps 5 --exposure 1.0 --gain 1

Then, you can `subscribe` to a [ZeroMQ] socket running at TCP port 8002 from your client application.

Check out `./client/imgrab.py` for an example client.

## What about 'pyuvc'?

[pyuvc] is a great library for using [libuvc] under Python.

But, I don't want to run python as root completely.

With my *solution* I only need to run this `libuvc_server` executable as root and I can run my python program as a standard user.

## Possible improvements

* Hotplug handling inside the server.
* Ability to control camera parameters while the server is running without restarting the process using another [ZeroMQ] socket.

[UVC]: https://en.wikipedia.org/wiki/USB_video_device_class
[ZeroMQ]: https://zeromq.org/
[VideoCapture]: https://docs.opencv.org/4.x/d8/dfe/classcv_1_1VideoCapture.html
[libuvc]: https://github.com/libuvc/libuvc
[pyuvc]: https://github.com/pupil-labs/pyuvc
