#!/bin/sh

arm-linux-gnueabihf-g++ draw_text.c cJSON.c -o draw_text -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc

