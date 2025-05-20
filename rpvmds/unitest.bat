REM 환경변수 설정.
call env.bat

echo RPVMDs Evaluator Start

if "-console"=="%1" (
    echo Enter the data you want to analyze
    java -jar "%RPVMS_HOME%\lib\rpvmds-evaluater-0.0.1-jar-with-dependencies.jar"
) else (
    echo simule
    java %RPVMS_J_OPTS% -classpath "%RPVMS_HOME%\lib\rpvmds.jar" com.icis.RPVM.ds.test.RPVMSimul
)
