#!/bin/bash

if [ "-console" = "$1" ]; then
    echo "Enter the data you want to analyze"
    java $RPVMS_J_OPTS -jar $RPVMDS_HOME/lib/rpvmds-evaluater-1.0.1.jar
else
    echo "UnitTest UI Open"
    java $RPVMS_J_OPTS -classpath $RPVMDS_HOME/lib/rpvmds-main-1.0.1.jar com.icis.RPVM.ds.test.RPVMSimul
fi
