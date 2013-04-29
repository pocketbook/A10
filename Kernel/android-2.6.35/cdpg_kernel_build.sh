#!/bin/bash
#

CPU_JOB_NUM=$(grep processor /proc/cpuinfo | awk '{field=$NF};END{print field+1}')

START_TIME=`date +%s`
make distclean
#make android_3621edp1_tft_defconfig
make omap3621_evt1a_defconfig
make uImage -j$CPU_JOB_NUM
make modules
END_TIME=`date +%s`
let "ELAPSED_TIME=$END_TIME-$START_TIME"


ls arch/arm/boot/uImage &> /dev/null
if [ $? != 0 ]; then
  echo "Total kernel built time = $ELAPSED_TIME seconds, FAIL !!"
else
  echo "Total kernel built time = $ELAPSED_TIME seconds, Success"
fi
