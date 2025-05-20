@echo off
rem rpvm 분산 서버 IP
set RPVMS_BIND_IP=192.168.0.171

rem rpvm 분산 서버 포트
set RPVMS_LISTENER_PORT=15678

rem 자바 실행 옵션
set RPVMS_J_OPTS=-Xms512M -Xmx1024M -XX:+UseParallelGC -XX:-UseGCOverheadLimit -Dfile.encoding=UTF-8

rem rpvm 분산 서버 홈 디렉토리
set RPVMS_HOME=C:\Users\icis\Desktop\Storage\1_Work\1_Dev\1_Repository\pricingmodule\rpvmds