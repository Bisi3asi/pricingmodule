#!/bin/bash

. env.sh

echo "RPVMDs Daemon Start"
nohup java $RPVMS_J_OPTS -Dlog4j2.configurationFile=$RPVMS_HOME/lib/log4j2.xml -jar $RPVMS_HOME/lib/rpvmds.jar -port=$RPVMS_LISTENER_PORT -bindip=$RPVMS_BIND_IP &

