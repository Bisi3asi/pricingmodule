@echo off
rem rpvm �л� ���� IP
set RPVMS_BIND_IP=192.168.0.171

rem rpvm �л� ���� ��Ʈ
set RPVMS_LISTENER_PORT=15678

rem �ڹ� ���� �ɼ�
set RPVMS_J_OPTS=-Xms512M -Xmx1024M -XX:+UseParallelGC -XX:-UseGCOverheadLimit -Dfile.encoding=UTF-8

rem rpvm �л� ���� Ȩ ���丮
set RPVMS_HOME=C:\Users\icis\Desktop\Storage\1_Work\1_Dev\1_Repository\pricingmodule\rpvmds