package org.jna;

import com.sun.jna.Platform;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Arrays;
import java.util.Scanner;

public class LibraryRunner {
    private static final Logger logger = LoggerFactory.getLogger(LibraryRunner.class);

    public static void runNativeLibrary(Config config) {
        if (config == null) {
            logger.error("Configuration is missing.");
            return;
        }

        String fileNameWithExt;
        if (Platform.isWindows()) {
            fileNameWithExt = config.getExecFileName() + ".dll";
            logger.info("::::: [DYNAMIC LIBRARY RUNNER] for Windows :::::");
        } else if (Platform.isLinux()) {
            fileNameWithExt = config.getExecFileName() + ".so";
            logger.info("::::: [DYNAMIC LIBRARY RUNNER] for Linux :::::");
        } else {
            logger.error("호환되지 않는 운영체제입니다.");
            return;
        }

        logger.info("Loading native library : {}", fileNameWithExt);

        NativeLibraryLoader loader;
        if (config.getExecFileDir() == null || config.getExecFileDir().isEmpty()) {
            loader = new NativeLibraryLoader("./" + fileNameWithExt);
        } else {
            loader = new NativeLibraryLoader("./" + config.getExecFileDir() + "/" + fileNameWithExt);
        }
        Scanner scanner = new Scanner(System.in);
        logger.info("Successfully loaded library : {}", fileNameWithExt);

        while (true) {
            logger.info("---------------------------------------------------------");
            logger.info("명령어를 입력해주세요. [-help, -list, -{functionName}, -exit]");
            String input = scanner.nextLine().trim();

            if ("-exit".equals(input)) {
                logger.info("시스템을 종료합니다.");
                break;
            }

            if ("-list".equals(input)) {
                logger.info("[{}] 사용 가능 함수", config.getExecFileName());
                for (String function : loader.getFunctions())
                    logger.info("{}. {}", loader.getFunctions().indexOf(function) + 1, function);
                continue;
            }
            if ("-help".equals(input)) {
                logger.info("사용 가능한 명령어");
                logger.info("-list : 호출 가능한 함수를 출력합니다.");
                logger.info("-exit : 시스템을 종료합니다.");
                logger.info("-{함수명} : 함수를 호출합니다.");
                continue;
            }

            // printStruct
            if ("-printStruct".equals(input) && loader.getFunctions().contains("printStruct")) {
                execPrintStruct(loader, scanner);
                continue;
            }

            // printType (String arr[], int arr[], double arr[], long arr[], double *)
            if ("-printType".equals(input) && loader.getFunctions().contains("printType")) {
                execPrintType(loader, scanner);
                continue;
            }

            // Div, Sub, Mul, Sum (double a, double b)
            if (("-updateDoubleArr".equals(input) && loader.getFunctions().contains("updateDoubleArr"))) {
                execUpdateDoubleArr(loader, scanner);
                continue;
            }

            // Div, Sub, Mul, Sum (double a, double b)
            if (("-Div".equals(input) || "-Sub".equals(input) || "-Mul".equals(input) || "-Sum".equals(input))
            && loader.getFunctions().contains("Div")) {
                execMathFunction(loader, scanner, input);
                continue;
            }

            // OtStock - stockPricing
            if ("-stockPricing".equals(input) && loader.getFunctions().contains("stockPricing")
                    && (config.getExecFileName().equals("libOtStock") || config.getExecFileName().equals("OtStock"))) {
                execStockPricing(loader);
                continue;
            }

            // FxFoward - pricing
            if ("-pricing".equals(input) && loader.getFunctions().contains("pricing") && (config.getExecFileName().equals("fxForward"))) {
                execPricing(loader);
                continue;
            }

            logger.warn("알 수 없는 명령어입니다. -help를 입력해 보세요.");
        }
    }

    private static void execMathFunction(NativeLibraryLoader loader, Scanner scanner, String input) {
        String functionName = input.replaceFirst("^-+", "");

        logger.info("[{}] 입력값 1", functionName);
        String param1 = scanner.nextLine().trim();
        double a = Double.parseDouble(param1);

        logger.info("[{}] 입력값 2", functionName);
        String param2 = scanner.nextLine().trim();
        double b = Double.parseDouble(param2);

        logger.info("[{}] 계산 결과 : {} ", functionName, loader.callMathFunction(functionName, a, b));
    }

    private static void execPrintType(NativeLibraryLoader loader, Scanner scanner) {
        logger.info("입력값 1 (쉼표로 구분된 문자열 배열) ex) apple,banana,grape : ");
        String param1 = scanner.nextLine().trim();
        String[] strArr = param1.isEmpty() ? new String[0] : param1.split(",");

        logger.info("입력값 2 (쉼표로 구분된 정수 배열) ex) 1,2,3,4 : ");
        String param2 = scanner.nextLine().trim();
        int[] intArr = param2.isEmpty() ? new int[0] : Arrays.stream(param2.split(",")).mapToInt(Integer::parseInt).toArray();

        logger.info("입력값 3 (쉼표로 구분된 실수 배열) ex) 1.1,2.2,3.3,4.4 : ");
        String param3 = scanner.nextLine().trim();
        double[] doubleArr = param3.isEmpty() ? new double[0] : Arrays.stream(param3.split(",")).mapToDouble(Double::parseDouble).toArray();

        logger.info("입력값 4 (쉼표로 구분된 long 배열) ex) 10000000000,20000000000,300000000000 : ");
        String param4 = scanner.nextLine().trim();
        long[] longArr = param4.isEmpty() ? new long[0] : Arrays.stream(param4.split(",")).mapToLong(Long::parseLong).toArray();

        logger.info("입력값 5 (포인터에 넣을 double 값) ex) 123.12 : ");
        String param5 = scanner.nextLine().trim();
        double value = param5.isEmpty() ? 0.0 : Double.parseDouble(param5);

        loader.callPrintType("printType",
                strArr, intArr, doubleArr, longArr, value,
                strArr.length, intArr.length, doubleArr.length, longArr.length
        );
    }

    private static void execPrintStruct(NativeLibraryLoader loader, Scanner scanner) {
        logger.info("입력값 1 (positionNm) ex) TRF240123 : ");
        String param1 = scanner.nextLine().trim();
        String positionNm = param1.isEmpty() ? "Default Position" : param1;

        logger.info("입력값 2 (bucketNm) ex) ABC,DEF,GHI,XYZ : ");
        String param2 = scanner.nextLine().trim();
        String[] bucket = param2.isEmpty() ? new String[0] : param2.split(",");

        logger.info("입력값 3 (bsdt) ex) 20341223 : ");
        String param3 = scanner.nextLine().trim();
        String bsdt = param3.isEmpty() ? "99991231" : param3;

        logger.info("입력값 3 (tenor) ex) 365 : ");
        String param4 = scanner.nextLine().trim();
        int tenor = param4.isEmpty() ? 365 : Integer.parseInt(param4);

        logger.info("입력값 5 (value) ex) 9999.00 : ");
        String param5 = scanner.nextLine().trim();
        double value = param5.isEmpty() ? 0.0 : Double.parseDouble(param5);

        loader.callPrintStruct("printStruct", positionNm, bucket, bucket.length, bsdt, tenor, value);
    }

    private static void execUpdateDoubleArr(NativeLibraryLoader loader, Scanner scanner) {
        logger.info("업데이트할 double 배열 값을 입력해주세요. (쉼표로 구분, 예: 1.1,2.2,3.3,4.4,5.5): ");
        String input = scanner.nextLine().trim();
        double[] doubleArr = input.isEmpty() ? new double[0] : Arrays.stream(input.split(","))
                .mapToDouble(Double::parseDouble)
                .toArray();

        loader.callUpdateDoubleArr("updateDoubleArr", doubleArr);
    }

    private static void execStockPricing(NativeLibraryLoader loader) {
        loader.callStockPricing("stockPricing",
                1200.0, 1000.0, 1000.0, 1.2, 0.0, new double[]{0.0, 0.0, 0.0, 0.0, 0.0}, new double[]{0.0});
    }

    private static void execPricing(NativeLibraryLoader loader) {
        // 입력값 정의
        long maturityDate = 46164;
        long revaluationDate = 45657;
        double exchangeRate = 1532.578;

        String buySideCurrency = "EUR";
        double notionalForeign = 4576279.99;
        int buySideDCB = 3;  // java short -> int로 명시적 변환
        String buySideDcCurve = "IREUR-CRS";

        String sellSideCurrency = "KRW";
        double notionalDomestic = 6820853799.5;
        int sellSideDCB = 3; // java short -> int로 명시적 변환
        String sellSideDcCurve = "IRKRW-CRS";

        int curveSize = 10;
        double[] buyCurveYearFrac = { 0.25, 0.5, 1, 2, 3, 5, 10, 15, 20, 30 };
        double[] buyMarketData = { 2.64438703, 2.38058648, 2.10763173, 1.97593133, 1.98563969, 2.07148214, 2.25037149, 2.36128877, 2.34768987, 2.2255283 };
        double[] sellCurveYearFrac = { 0.25, 0.5, 1, 2, 3, 5, 10, 15, 20, 30 };
        double[] sellMarketData = { 3.08, 2.58, 2.33, 2.19, 2.19, 2.23, 2.24, 2.12, 2.04, 2.04 };

        int calType = 2;
        int logYn = 1;

        // 결과 배열 초기화
        double[] resultNetPvFxSensitivity = new double[2];
        double[] resultGirrDelta = new double[25];  // 충분히 크게

        // 함수 호출
        loader.callPricing(
                "pricing",
                maturityDate,
                revaluationDate,
                exchangeRate,
                buySideCurrency,
                notionalForeign,
                buySideDCB,
                buySideDcCurve,
                curveSize,
                buyCurveYearFrac,
                buyMarketData,
                sellSideCurrency,
                notionalDomestic,
                sellSideDCB,
                sellSideDcCurve,
                curveSize,
                sellCurveYearFrac,
                sellMarketData,
                calType,
                logYn,
                resultNetPvFxSensitivity,
                resultGirrDelta
        );

        // 결과 출력
        if (calType == 1) {
            System.out.printf("Net PV: %.2f%n", resultNetPvFxSensitivity[0]);
            System.out.printf("FX Sensitivity: %.2f%n", resultNetPvFxSensitivity[1]);
        } else if (calType == 2) {
            int size = (int) resultGirrDelta[0];
            System.out.println("GIRR Delta Size: " + size);
            for (int i = 1; i <= size; i++) {
                System.out.printf("%d. Tenor: %.2f%n", i, resultGirrDelta[i]);
            }
            for (int i = size + 1; i <= size * 2; i++) {
                System.out.printf("%d. Sensitivity: %.2f%n", i - size, resultGirrDelta[i]);
            }
        }
    }
}
