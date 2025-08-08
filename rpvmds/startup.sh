#!/bin/bash

#자바 실행 옵션
RPVMS_J_OPTS="-Xms32M -Xmx2048M -XX:+UseParallelGC -XX:-UseGCOverheadLimit -Dfile.encoding=UTF-8"

echo "RPVMDs Daemon Start"
nohup java $RPVMS_J_OPTS -jar $RPVMDS_HOME/lib/rpvmds-main-1.0.1.jar /dev/null 2>&1 &

