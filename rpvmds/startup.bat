REM ȯ�溯�� ����.

rem �ڹ� ���� �ɼ�
set RPVMS_J_OPTS=-Xms32M -Xmx2048M -XX:+UseParallelGC -XX:-UseGCOverheadLimit -Dfile.encoding=UTF-8

echo RPVMDs Daemon Start
start /B java %RPVMS_J_OPTS% -jar %RPVMDS_HOME%\lib\rpvmds-main-1.0.1.jar 2>&1
