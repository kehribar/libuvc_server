// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <czmq.h>
#include "libuvc/libuvc.h"
#include <argparse/argparse.hpp>
// ----------------------------------------------------------------------------
static void* frame_sock_raw;
static int s_interrupted = 0;
static void s_catch_signals(void);
static void s_signal_handler(int signal_value);

// ----------------------------------------------------------------------------
void cb(uvc_frame_t* frame, void* ptr)
{
  // ...
  uvc_error_t ret;
  uvc_frame_t* rgb;

  // ...
  rgb = uvc_allocate_frame(frame->width * frame->height * 3);
  if(rgb == NULL)
  {
    printf("unable to allocate rgb frame!\n");
    return;
  }

  if(frame->frame_format == UVC_COLOR_FORMAT_MJPEG)
  {
    ret = uvc_mjpeg2rgb(frame, rgb);
    if(ret)
    {
      uvc_perror(ret, "uvc_mjpeg2rgb");
      uvc_free_frame(rgb);
      return;
    }
  }
  else
  {
    ret = uvc_any2rgb(frame, rgb);
    if(ret)
    {
      uvc_perror(ret, "uvc_any2rgb");
      uvc_free_frame(rgb);
      return;
    }
  }

  // ...
  zmq_send(
    frame_sock_raw,
    rgb->data,
    frame->width * frame->height * 3,
    ZMQ_DONTWAIT
  );

  // ...
  uvc_free_frame(rgb);
}

// ----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // ...
  uvc_error_t res;
  uvc_device_t* dev;
  uvc_context_t* ctx;
  uvc_stream_ctrl_t ctrl;
  uvc_device_handle_t* devh;

  // ...
  argparse::ArgumentParser program("libuvc_server");

  try
  {
    // ...
    program.add_argument("--width")
      .help("Frame Wdith")
      .required()
      .scan<'i', int>();

    // ...
    program.add_argument("--height")
      .help("Frame Height")
      .required()
      .scan<'i', int>();

    // ...
    program.add_argument("--fps")
      .help("Frames Per Second")
      .required()
      .scan<'i', int>();

    // ...
    program.add_argument("--exposure")
      .help("Exposure Time (miliseconds)")
      .required()
      .scan<'g', float>();

    // ...
    program.add_argument("--gain")
      .help("Sensor Gain [1, 65335]")
      .required()
      .scan<'i', int>();

    // ...
    program.add_argument("--vid")
      .help("USB Vendor ID")
      .required()
      .scan<'d', int>();

    // ...
    program.add_argument("--pid")
      .help("USB Product ID")
      .required()
      .scan<'d', int>();

    // ...
    program.add_argument("--serial")
      .help("USB Device Serial Number")
      .default_value("");

    // ...
    program.parse_args(argc, argv);

    // ...
    int32_t WIDTH = program.get<int>("width");
    int32_t HEIGHT = program.get<int>("height");
    int32_t FPS = program.get<int>("fps");

    // ...
    float EXPOSURE_MS = program.get<float>("exposure");
    int32_t GAIN = program.get<int>("gain");

    // ...
    int32_t VENDOR_ID = program.get<int>("vid");
    int32_t PRODUCT_ID = program.get<int>("pid");

    // ...
    std::string SERIAL_NUM_STR = program.get("serial");
    char* SERIAL_NUM = NULL;
    if(SERIAL_NUM_STR.length() != 0)
    {
      SERIAL_NUM = (char*)(SERIAL_NUM_STR.c_str());
    }

    // ...
    int major; int minor; int patch;
    zmq_version(&major, &minor, &patch);
    printf("Current 0MQ version is %d.%d.%d\n", major, minor, patch);

    // ...
    zsock_t* frame_sock = zsock_new_pub("@tcp://*:8002");
    frame_sock_raw = zsock_resolve(frame_sock);
    printf("Streaming from port %d ...\n", 8002);

    // ...
    res = uvc_init(&ctx, NULL);
    if(res < 0)
    {
      uvc_perror(res, "uvc_init");
      return res;
    }

    // ...
    res = uvc_find_device(ctx, &dev, VENDOR_ID, PRODUCT_ID, SERIAL_NUM);
    if(res < 0)
    {
      uvc_perror(res, "uvc_find_device");
      return -1;
    }

    // ...
    printf("Device found!\n");

    // Try to open the device: requires exclusive access
    res = uvc_open(dev, &devh);
    if(res < 0)
    {
      uvc_perror(res, "uvc_open");
      return -1;
    }

    // ...
    #if 0
      FILE* fp = fopen("device_log.txt", "w+");
      uvc_print_diag(devh, fp);
      fclose(fp);
    #endif

    // ...
    res = uvc_get_stream_ctrl_format_size(
        devh, &ctrl, UVC_FRAME_FORMAT_ANY,
        WIDTH, HEIGHT, FPS
    );
    if(res < 0)
    {
      uvc_perror(res, "uvc_get_stream_ctrl_format_size");
      return -1;
    }

    // ...
    res = uvc_start_streaming(devh, &ctrl, cb, (void*)NULL, 0);
    if(res < 0)
    {
      uvc_perror(res, "uvc_start_streaming");
      return -1;
    }

    // ...
    s_catch_signals();

    // ...
    while(1)
    {
      // ...
      const uint8_t UVC_AUTO_EXPOSURE_MODE_MANUAL = 1;
      res = uvc_set_ae_mode(devh, UVC_AUTO_EXPOSURE_MODE_MANUAL);
      if(res != UVC_SUCCESS)
      {
        uvc_perror(res, "uvc_set_ae_mode");
        return -1;
      }

      // Each unit is 0.1 ms
      const float EXPOSURE_MS_TO_RAW = 10.0;
      res = uvc_set_exposure_abs(devh, EXPOSURE_MS * EXPOSURE_MS_TO_RAW);
      if(res != UVC_SUCCESS)
      {
        uvc_perror(res, "uvc_set_exposure_abs");
        return -1;
      }

      // ...
      res = uvc_set_gain(devh, GAIN);
      if(res != UVC_SUCCESS)
      {
        uvc_perror(res, "uvc_set_gain");
        return -1;
      }

      usleep(1000 * 1000);
      if(s_interrupted)
      {
        printf("\n");
        printf("Interrupt received. Exiting.\n");
        break;
      }
    }
  }
  catch(const std::runtime_error& err)
  {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return -1;
  }

  //
  printf("libuvc_server exits.\n");
  return 0;
}

// ----------------------------------------------------------------------------
static void s_signal_handler(int signal_value)
{
  s_interrupted = 1;
}

// ----------------------------------------------------------------------------
static void s_catch_signals(void)
{
  struct sigaction action;
  action.sa_handler = s_signal_handler;
  action.sa_flags = 0;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
}