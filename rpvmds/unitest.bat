@echo off

echo RPVMds Evaluator Start
if "-console"=="%1" (
    echo Enter the data you want to analyze
    java -jar "%RPVMDS_HOME%\lib\rpvmds-evaluater-0.0.1.jar"
) else (
    echo UnitTest UI Open 
    java %RPVMS_J_OPTS% -classpath "%RPVMDS_HOME%\lib\rpvmds.jar" com.icis.RPVM.ds.test.RPVMSimul
)
