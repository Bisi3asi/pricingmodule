#!/bin/bash

. env.sh

if [ "-console" = "$1" ]; then
    echo "Enter the data you want to analyze"
    java $RPVMS_J_OPTS -jar $RPVMS_HOME/lib/rpvmds-evaluater-0.0.1-jar-with-dependencies.jar
else
    echo "simule"
    java $RPVMS_J_OPTS -classpath $RPVMS_HOME/lib/rpvmds.jar com.icis.RPVM.ds.test.RPVMSimul
fi
