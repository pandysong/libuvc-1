#include "libuvc/libuvc.h"
#include <stdio.h>
#include <unistd.h>

//#define DEBUG

#ifdef DEBUG
#define debug(msg) puts(msg)
#else
#define debug(msg)
#endif

void print_usage(const char *program)
{
    printf("usage: %s [command]\n", program);
    printf("  command: \n");
    printf("    loader: enter to loader mode\n");
    printf("    version: display firmware version\n");
    printf("    reboot: just reboot\n");
    printf("    (eptz cmds):\n");
    printf("       on\n");
    printf("       paused\n");
    printf("       off\n");
}

int main(int argc, char **argv) {
  uvc_context_t *ctx;
  uvc_device_t *dev;
  uvc_device_handle_t *devh;
  uvc_error_t res;
  uint8_t data[60];
  uint8_t xucmd;
  unsigned int command;
  int ret;

  if (argc != 2)
  {
      print_usage(argv[0]);
      return 0;
  }

  /* Initialize a UVC service context. Libuvc will set up its own libusb
   * context. Replace NULL with a libusb_context pointer to run libuvc
   * from an existing libusb context. */
  res = uvc_init(&ctx, NULL);

  if (res < 0) {
    uvc_perror(res, "uvc_init");
    return res;
  }

  debug("UVC initialized");

  /* Locates the first attached UVC device, stores in dev */
  res = uvc_find_device(
      ctx, &dev,
      0, 0, NULL); /* filter devices: vendor_id, product_id, "serial_num" */

  if (res < 0) {
    uvc_perror(res, "uvc_find_device"); /* no devices found */
  } else {
    debug("Device found");

    /* Try to open the device: requires exclusive access */
    res = uvc_open(dev, &devh);

    if (res < 0) {
      uvc_perror(res, "uvc_open"); /* unable to open device */
    } else {
      debug("Device opened");

      /* Print out a message containing all the information that libuvc
       * knows about the device */
      //uvc_print_diag(devh, stderr);

      const uvc_extension_unit_t * eu = uvc_get_extension_units(devh);

      if (eu == NULL)
      {
          printf("error: could not find proper control unit\n");
          goto release_dev;
      }

      const uint8_t guid[]= {
      0xa2, 0x9e, 0x76, 0x41, 0xde, 0x04, 0x47, 0xe3,
      0x8b, 0x2b, 0xf4, 0x34, 0x1a, 0xff, 0x00, 0x3b };

      //printf("first xu: bUnitID %d\n", eu->bUnitID);
      for (int i=0; i < 16; i ++)
          if (guid[i] != eu->guidExtensionCode[i])
          {
              printf("error: guid not matched\n");
              goto release_dev;
          }

      //printf("bmControls:%16llx\n", eu->bmControls);

      if ('v' == argv[1][0] )
      {
          ret = uvc_get_ctrl(devh,eu->bUnitID, 0x2, data, 60, UVC_GET_CUR);

          if (60 == ret)
              printf("Firmware Version: %s\n",data);
          else
              printf("failed to get version, err %d\n",ret);
      }
      else
      {
          if ('o' == argv[1][0] ||  'p' == argv[1][0])
          {
              xucmd = 0xa;
              if ('p' == argv[1][0] )
              {
                  command = 0x2;
              }
              else if ('n' == argv[1][1] )
              {
                  command = 0x1;
              }
              else if ('f' == argv[1][1] )
              {
                  command = 0x0;
              }
          }
          else {

              xucmd = 0x1;
              if ('l' == argv[1][0] )
              {
                  command = 0xFFFFFFFF;
              }
              else if ('r' == argv[1][0] )
              {
                  command = 0xFFFFFFF1;
              }
              else
              {
                  print_usage(argv[0]);
                  goto release_dev;
              }
          }

          ret = uvc_set_ctrl(devh,eu->bUnitID, xucmd, &command, sizeof(command));

          if (4 == ret)
              printf("command %s successfully\n", argv[1]);
          else
              printf("failed to run command %s, err %d\n",argv[1], ret);
      }
      goto release_dev;
    }

    goto unref_dev;
  }

  goto close_ctx;

release_dev:
  /* Release our handle on the device */
  uvc_close(devh);
  debug("Device closed");

unref_dev:
    /* Release the device descriptor */
    uvc_unref_device(dev);

close_ctx:
  /* Close the UVC context. This closes and cleans up any existing device handles,
   * and it closes the libusb context if one was not provided. */
  uvc_exit(ctx);
  debug("UVC exited");

  return 0;
}
