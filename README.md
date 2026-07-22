# Zephyr Video Capture with GMSL Example Application

This repository contains a Zephyr example application for retrieving video frames
from a capture device and displaying them on a display, while using
[Analog Device's GMSL](https://www.analog.com/en/product-category/serdes-gmsl.html)
transport for the MIPI-CSI2 data.

This example is based on the Zephyr [Capture-to-LVGL](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/video/capture_to_lvgl)
example project, with modifications to support initialization of the GMSL link.

There are no native Zephyr drivers for the GMSL devices, rather they are
initialized from application code directly to I2C using pre-defined register
initialization tables and simple device tree entries for specifying the I2C bus
and bus addresses.

## Hardware Setup

This application has been configured for and tested with the following hardware
components:
 - [STM32N6570-DK](https://www.st.com/en/evaluation-tools/stm32n6570-dk.html) -
    Discovery Kit for the STM32N657X0 MCU
 - [B-CAMS-IMX](https://www.st.com/en/evaluation-tools/b-cams-imx.html) -
    IMX335 based camera module for STM32 boards
 - [MAX96274F-BAK-EVK](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/max96724f-bak-evk.html) -
    Evaluation Kit for Analog Devices MAX96724 GMSL Deserializer
 - [MAX96717F-AAK-EVK](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/max96717f-aak-evk.html) -
    Evaluation kit for Analog Devices MAX96717 GMSL Serializer
 - [AD-GMSLCAMRPI-ADP](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/ad-gmslcamrpi-adp.html) -
    Analog Devices GMSL EvKit Adapter Board

For a detailed description of the hardware setup, interconnect and board modifications
see the [Hardware Setup](doc/HARDWARE.md) documentation.


## Getting Started

Before getting started, make sure you have a proper Zephyr development
environment. Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

In addition, you'll need to install the STM32 specific tools for signing and flashing
the firmware images. Follow the official Zephyr [STM32 Flash & Debug Host Tools](https://docs.zephyrproject.org/latest/develop/flash_debug/host-tools.html#stm32cubeprog-flash-host-tools)
guide for instructions on installation.

### Initialization

The first step is to initialize the workspace folder (``my-workspace``) where
the ``zephyr-videocapture-gmsl`` and all Zephyr modules will be cloned. Run the following
command:

```shell
# initialize my-workspace for the zephyr-videocapture-gmsl (main branch)
west init -m https://github.com/brentk-adi/zephyr-videocapture-gmsl --mr main my-workspace
# update Zephyr modules
cd my-workspace
west update
```

### Building

To build the application, run the following command:

```shell
cd zephyr-videocapture-gmsl
west build -b stm32n6570_dk//fsbl --shield st_b_cams_imx_mb1854 app
```

### Flashing
Once you have built the application, setup the STM32N6570-DK for flashing.  Set
the ``BOOT0`` switch to 0, and ``BOOT1`` to 1.  Connect the Discovery Kit
(STLINK Connector) to your PC, or if already connected, press the board's reset
button.

Run the following command to flash it:

```shell
west flash
```

Once flashing is completed. Return ``BOOT0`` and ``BOOT1`` to the 0 position.

> Additional information on programming and debugging the STM32N6570-DK can be
found on the Zephyr [STM32N6570-DK Board](https://docs.zephyrproject.org/latest/boards/st/stm32n6570_dk/doc/index.html#programming-and-debugging) page

### Running

Verify the hardware connections as described in [Hardware Setup](doc/HARDWARE.md).
Apply power to the Deserializer via the 12VDC adapter, and verify the power switches
of both the MAX96717F-AAK and MAX96724F-BAK are in the ``ON`` position.  Both boards
should have illuminated status and power LEDs.

Connect the STM32N6570-DK (STLINK) to your PC, or if already connected, press the
board's reset button.  Live camera video should be displayed on the LCD at startup.

#### Zephyr Shell

The Zephyr shell is available via the STM32 STLINK virtual UART.  The application
includes the Video Shell which provides command line access to adjust the camera
parameters including exposure and analog gain.

## Porting to Other Platforms

The application leverages the existing Zephyr device drivers and infrastructure
for video capture and display.  Boards and shields compatible with the Zephyr
[Capture-to-LVGL](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/video/capture_to_lvgl) example can be used with this project.

To support the GMSL link, the camera device (and possibly related devices) must
be initialized at run time by the application _after_ the GMSL link is ready. This
is performed by the `zephyr,deferred-init` device tree property, which can be seen
in the [app.overlay](app/app.overlay) file, and corresponding `device_init` calls
in [main.c](app/src/main.c).

The GMSL configuration is tailored for this device, include number of CSI lanes
used as well as clock rates based on the camera Pixel clock.  These may need to
be adjusted for use with different sensors. For detailed information on modifying
the GMSL configuration registers, please consult the User's Guides and
Configuration GUI's listed on the GMSL device product pages.