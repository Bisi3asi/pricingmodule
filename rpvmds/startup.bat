REM 환경변수 설정.
call env.bat

echo RPVMDs Daemon Start %RPVMS_BIND_IP%:%RPVMS_LISTENER_PORT%
start /B java %RPVMS_J_OPTS% -Dlog4j2.configurationFile=%RPVMS_HOME%\lib\log4j2.xml -jar %RPVMS_HOME%\lib\rpvmds.jar -port=%RPVMS_LISTENER_PORT% -bindip=%RPVMS_BIND_IP% 2>&1
