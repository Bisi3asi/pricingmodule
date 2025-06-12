package org.jna;

import com.sun.jna.Platform;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
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

        String baseDir = config.getExecFileDir();
        if (baseDir == null || baseDir.isEmpty()) {
            baseDir = System.getProperty("user.dir");
            loader = new NativeLibraryLoader(baseDir + File.separator + fileNameWithExt);
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
                execFxForwardPricing(loader);
                continue;
            }

            // Bond - pricing
            if ("-pricing".equals(input) && loader.getFunctions().contains("pricing") && (config.getExecFileName().equals("bond"))) {
                execFixedRateBondPricing(loader);
                continue;
            }

            // KIS HiFiveSwapMC
            if ("-KISP_CalcHiFiveSwapMC".equals(input)
                    && loader.getFunctions().contains("KISP_CalcHiFiveSwapMC")
                    && (config.getExecFileName().equals("KISPEQ_HiFiveSwapMC64"))
            ) {
                execKISPCalcHiFiveSwapMC(loader);
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

    private static void execFxForwardPricing(NativeLibraryLoader loader) {
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
        loader.callFxFowardPricing(
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

    private static void execFixedRateBondPricing(NativeLibraryLoader loader) {
        // 입력값 정의 (테스트 데이터 참고)
        int evaluationDate = 45657;
        int settlementDays = 0;
        int issueDate = 44175;
        int maturityDate = 47827;
        double notional = 6000000000.0;
        double couponRate = 0.015;
        int couponDayCounter = 5; // Actual/Actual(Bond)
        int numberOfCoupons = 12;
        int[] paymentDates = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };
        int[] realStartDates = { 45636, 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644 };
        int[] realEndDates = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };

        int numberOfGirrTenors = 10;
        int[] girrTenorDays = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };
        double[] girrRates = { 0.0337, 0.0317, 0.0285, 0.0272, 0.0269, 0.0271, 0.0278, 0.0272, 0.0254, 0.0222 };
        int girrDayCounter = 1;      // Actual/365
        int girrInterpolator = 1;    // Linear
        int girrCompounding = 1;     // Continuous
        int girrFrequency = 1;       // Annual

        double spreadOverYield = 0.001389;
        int spreadOverYieldCompounding = 1;  // Continuous
        int spreadOverYieldDayCounter = 1;   // Actual/365

        int numberOfCsrTenors = 5;
        int[] csrTenorDays = { 183, 365, 1095, 1825, 3650 };
        double[] csrRates = { 0.0, 0.0, 0.0, 0.0005, 0.001 };

        int calType = 3;  // GIRR/CSR 민감도 산출
        int logYn = 1;    // 로그 출력 여부

        // 결과 배열 초기화
        double[] resultGirrDelta = new double[50];
        double[] resultCsrDelta = new double[50];

        // 네이티브 함수 호출
        double resultNetPV =
        loader.callFixedRateBondPricing(
                "pricing",
                evaluationDate,
                settlementDays,
                issueDate,
                maturityDate,
                notional,
                couponRate,
                couponDayCounter,
                numberOfCoupons,
                paymentDates,
                realStartDates,
                realEndDates,
                numberOfGirrTenors,
                girrTenorDays,
                girrRates,
                girrDayCounter,
                girrInterpolator,
                girrCompounding,
                girrFrequency,
                spreadOverYield,
                spreadOverYieldCompounding,
                spreadOverYieldDayCounter,
                numberOfCsrTenors,
                csrTenorDays,
                csrRates,
                calType,
                logYn,
                resultGirrDelta,
                resultCsrDelta
        );

        // 결과 출력
        if (calType == 1 || calType == 3) {
            System.out.printf("Net PV: %.10f%n", resultNetPV);
        }
        if (calType == 3) {
            int size = (int) resultGirrDelta[0];
            System.out.println("GIRR Delta Size: " + size);
            for (int i = 1; i <= size; i++) {
                System.out.printf("%d. GIRR Tenor: %.2f%n", i, (double) resultGirrDelta[i]);
            }
            for (int i = size + 1; i <= size * 2; i++) {
                System.out.printf("%d. GIRR Sensitivity: %.10f%n", i, (double) resultGirrDelta[i]);
            }

            size = (int) resultCsrDelta[0];
            for (int i = 1; i <= size; i++) {
                System.out.printf("%d. CSR Tenor: %.2f%n", i, (double) resultCsrDelta[i]);
            }
            for (int i = size + 1; i <= size * 2; i++) {
                System.out.printf("%d. CSR Sensitivity: %.10f%n", i, (double) resultCsrDelta[i]);
            }
        }
    }

    private static void execKISPCalcHiFiveSwapMC(NativeLibraryLoader loader) {
        // 입력값 정의 (테스트 데이터 참고)
        int PriceDate = 20170808;
        double NotionalAMT = 10000;
        double[] UnderlyingPrice = new double[] {130,150,130,150,130,150};
        double[] TCurveInfo = new double[] {1,2,3,1,2,3,1,2,3,1,2,3};
        double[] CurveInfo = new double[] {0.035138,0.035138,0.035138,-0.0052,-0.0052,-0.0052,-0.0001,-0.0001,-0.0001,0.035138,0.035138,0.035138};
        int[] NT = new int[] {3,3,3,3};
        double[] TVol = new double[] {1,2,1,2,1,2};
        double[] Parity = new double[] {0.9,1,1.1,0.9,1,1.1,0.9,1,1.1};
        double[] LocalVolSurface = new double[] {0.2,0.2,0.2w,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2};
        int[] NVol = new int[] {2,2,2};
        int[] NParity = new int[] {3,3,3};
        double[] UnderlyingCorr = new double[] {1,0,0,0,1,0,0,0,1};
        int[] TDiv = new int[] {100,100,200,100,200,300};
        double[] Div = new double[] {0.035138,0.035138,0.035138,5,5,5};
        int[] NDiv = new int[] {1,2,3};
        int[] DivTypeFlag = new int[] {0,1,2};
        double[] FX_TVol = new double[] {1,2,3,1,2,3,1,2,3};
        double[] FX_Vol = new double[] {0.035138,0.035138,0.035138,0.035138,0.035138,0.035138,0.035138,0.035138,0.035138};
        int[] FX_NVol = new int[] {3,3,3};
        double[] FX_Corr = new double[] {0.5,0.5,0.5};
        int[] QuantoFlag = new int[] {0,1,2};
        int NStock = 3;
        int NStrike = 6;
        double[] X = new double[] {
                0.9,0.9,0.9,0.9,0.9,0.9,0.8,0.8,0.8,0.8,0.8,0.8,0.7,0.7,0.7,0.7,0.7,0.7,
                0.9,0.9,0.9,0.9,0.9,0.9,0.8,0.8,0.8,0.8,0.8,0.8,0.7,0.7,0.7,0.7,0.7,0.7,
                0.9,0.9,0.9,0.9,0.9,0.9,0.8,0.8,0.8,0.8,0.8,0.8,0.7,0.7,0.7,0.7,0.7,0.7,
        };
        double[] KO_Barrier = new double[] {0.9,0.9,0.9,0.9,0.9,0.9};
        double[] KI_Barrier = new double[] {0.5,0.5,0.5};
        double[] PayOffSlope = new double[] {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1};
        double[] PayOffAmount = new double[] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        int[] DatesInfo = new int[] {
                20170810,20180210,20180810,20190210,20190810,20200210,
                20170813,20180213,20180813,20190213,20190813,20200213,
                1,1,1,1,1,1,1,1,1,1,1,1
        };
        double[] RedemCoupon = new double[] {0,0,0,0,0};
        int[] KnockBarrierFlag = new int[] {0,0};
        int BestWorstFlag = 1;
        int[] BeKO_Hitted = new int[] {0,0,0,1};
        int BeKI_Hitted = 0;
        int NCoupon = 6;
        double[] CouponBarrier = new double[] {0,0,0,0,0,0};
        double[] Coupon = new double[] {0.01,0.01,0.01,0.01,0.01,0.01};
        int[] CouponDatesInfo = new int[] {
                20170810,20180210,20180810,20190210,20190810,20200210,
                20170813,20180213,20180813,20190213,20190813,20200213
        };
        int CouponBarrierFlag = 0;
        int BeCouponBarrierHitted = 0;
        double NTrials = 10000;
        double[] PayOffLimit = new double[] {1,100};
        double[] SC = new double[] {244.09,244.09,244.09,0};
        double[] Coupon_SC = new double[] {244.09,244.09,244.09,0};
        int[] FLLongInfo = new int[] {1,1,33,33,96,0,2,1,0,12};
        double[] FLCurveTerm = new double[] {
                0.00277777777777778,0.0305555555555556,0.0444444444444444,0.0694444444444444,
                0.0944444444444444,0.180555555555556,0.266666666666667,0.35,0.436111111111111,
                0.519444444444444,0.605555555555556,0.691666666666667,0.769444444444444,
                0.855555555555556,0.938888888888889,1.025,1.28055555555556,1.53333333333333,
                1.78055555555556,2.03611111111111,3.05,4.06111111111111,5.07222222222222,
                6.08333333333333,7.09722222222222,8.10833333333333,9.11944444444445,
                10.1305555555556,11.1444444444444,12.1555555555556,15.1916666666667,
                20.25,25.3083333333333,0.00277777777777778,0.0305555555555556,4.44444444444444,
                0.0694444444444444,0.0944444444444444,0.180555555555556,0.266666666666667,
                0.35,0.436111111111111,0.519444444444444,0.605555555555556,0.691666666666667,
                0.769444444444444,0.855555555555556,0.938888888888889,1.025,1.28055555555556,
                1.53333333333333,1.78055555555556,2.03611111111111,3.05,4.06111111111111,
                5.07222222222222,6.08333333333333,7.09722222222222,8.10833333333333,
                9.11944444444445,10.1305555555556,11.1444444444444,2.1555555555556,
                15.1916666666667,20.25,25.3083333333333
        };
        double[] FLCurveRate = new double[] {
                0.000199999944454987, 0.000247999402094017, 0.000253973827461869, 0.000261417484391979, 0.000263593201030217,
                0.000309603332498718, 0.000318054959593167, 0.00032995743123133, 0.000342155904429004, 0.000358541052950071,
                0.000371402197953929, 0.000379037316291144, 0.000387205256902421, 0.000435551859318853, 0.000443391895247618,
                0.000428842049914623, 0.000524619289484804, 0.000666865851465922, 0.000860128196610481, 0.00111350106037003,
                0.00262895780164227, 0.00463910894529559, 0.00652748072571885, 0.00820906109770577, 0.00961813305324175,
                0.0107469203596044, 0.0116807372490792, 0.0124822954530104, 0.0131860191326933, 0.0137800580903817,
                0.0150255821288824, 0.0160629656156907, 0.0163259697787702, 0.000199999944454987, 0.000247999402094017,
                0.000253973827461869, 0.000261417484391979, 0.000263593201030217, 0.000309603332498718, 0.000318054959593167,
                0.00032995743123133, 0.000342155904429004, 0.000358541052950071, 0.000371402197953929, 0.000379037316291144,
                0.000387205256902421, 0.000435551859318853, 0.000443391895247618, 0.000428842049914623, 0.000524619289484804,
                0.000666865851465922, 0.000860128196610481, 0.00111350106037003, 0.00262895780164227, 0.00463910894529559,
                0.00652748072571885, 0.00820906109770577, 0.00961813305324175, 0.0107469203596044, 0.0116807372490792,
                0.0124822954530104, 0.0131860191326933, 0.0137800580903817, 0.0150255821288824, 0.0160629656156907,
                0.0163259697787702
        };
        int[] FLCurveDate = new int[] {
                20170504, 20170505, 20170506, 20170507, 20170508, 20170509, 20170510, 20170511, 20170512, 20170513,
                20170514, 20170515, 20170516, 20170517, 20170518, 20170519, 20170520, 20170521, 20170522, 20170523,
                20170524, 20170525, 20170526, 20170527, 20170528, 20170529, 20170530, 20170531, 20170601, 20170602,
                20170603, 20170604, 20170605, 20170606, 20170607, 20170608, 20170609, 20170610, 20170611, 20170612,
                20170613, 20170614, 20170615, 20170616, 20170617, 20170618, 20170619, 20170620, 20170621, 20170622,
                20170623, 20170624, 20170625, 20170626, 20170627, 20170628, 20170629, 20170630, 20170701, 20170702,
                20170703, 20170704, 20170705, 20170706, 20170707, 20170708, 20170709, 20170710, 20170711, 20170712,
                20170713, 20170714, 20170715, 20170716, 20170717, 20170718, 20170719, 20170720, 20170721, 20170722,
                20170723, 20170724, 20170725, 20170726, 20170727, 20170728, 20170729, 20170730, 20170731, 20170801,
                20170802, 20170803, 20170804, 20170805, 20170806, 20170807
        };
        double[] HistoryRate = new double[] {
                0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012,
                0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012,
                0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012,
                0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012,
                0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012,
                0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012,
                0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012,
                0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012,
                0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012,
                0.012, 0.012, 0.012, 0.012
        };
        double FLRefRatePeriod = 0.25;
        int[] FLResetFixDateInfo = new int[] {
                20170210, 20170510, 20170810, 20171110, 20180210, 20180510, 20180810, 20181110, 20190210, 20190510,
                20190810, 20191110, 5, 5, 5, 5, 5, 5, 5, 5,
                5, 5, 5, 5, 20170210, 20170510, 20170810, 20171110, 20180210, 20180510,
                20180810, 20181110, 20190210, 20190510, 20190810, 20191110, 20170510, 20170810, 20171110, 20180210,
                20180510, 20180810, 20181110, 20190210, 20190510, 20190810, 20191110, 20200210, 5, 5,
                5, 5, 5, 5, 5, 5, 5, 5,
                5, 5
        };
        int[] FLCouponDateInfo = new int[] {
                20170211, 20170511, 20170811, 20171111, 20180211, 20180511, 20180811, 20181111, 20190211, 20190511,
                20190811, 20191111, 20170511, 20170811, 20171111, 20180211, 20180511, 20180811, 20181111, 20190211,
                20190511, 20190811, 20191111, 20200211, 20170514, 20170814, 20171114, 20180214, 20180514, 20180814,
                20181114, 20190214, 20190514, 20190814, 20191114, 20200214
        };
        double[] FLCouponInfo = new double[] {
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0.0275, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0
        };
        int[] CalcFlag = new int[] {0, 1, 0};

        // 결과 배열 초기화
//        double[] ELS_Prob_n_Cf = new double[(int)(2 * (NStrike + 2))];
//        double[] ELS_Coupon_Prob_n_Cf = new double[(int)(2 * NCoupon)];
//        double[] IR_Prob_n_Cf = new double[(int)(2 * FLLongInfo[9])];
//        double[] PV01 = new double[(int)(FLLongInfo[2] + FLLongInfo[3])];
//        double[] ResultPrice = new double[] {0.0, 0.0, 0.0};

        double[] ELS_Prob_n_Cf = new double[16];
        double[] ELS_Coupon_Prob_n_Cf = new double[12];
        double[] IR_Prob_n_Cf = new double[24];
        double[] PV01 = new double[(int)(FLLongInfo[2] + FLLongInfo[3])];
//        double[] ResultPrice = new double[3];
        double[] ResultPrice = new double[] {1.1, 2.2, 3.3};

        // 네이티브 함수 호출
        loader.callKISPCalcHiFiveSwapMC(
                "KISP_CalcHiFiveSwapMC",
                PriceDate,
                NotionalAMT,
                UnderlyingPrice,
                TCurveInfo,
                CurveInfo,
                NT,
                TVol,
                Parity,
                LocalVolSurface,
                NVol,
                NParity,
                UnderlyingCorr,
                TDiv,
                Div,
                NDiv,
                DivTypeFlag,
                FX_TVol,
                FX_Vol,
                FX_NVol,
                FX_Corr,
                QuantoFlag,
                NStock,
                NStrike,
                X,
                KO_Barrier,
                KI_Barrier,
                PayOffSlope,
                PayOffAmount,
                DatesInfo,
                RedemCoupon,
                KnockBarrierFlag,
                BestWorstFlag,
                BeKO_Hitted,
                BeKI_Hitted,
                NCoupon,
                CouponBarrier,
                Coupon,
                CouponDatesInfo,
                CouponBarrierFlag,
                BeCouponBarrierHitted,
                NTrials,
                PayOffLimit,
                SC,
                Coupon_SC,
                FLLongInfo,
                FLCurveTerm,
                FLCurveRate,
                FLCurveDate,
                HistoryRate,
                FLRefRatePeriod,
                FLResetFixDateInfo,
                FLCouponDateInfo,
                FLCouponInfo,
                CalcFlag,
                ELS_Prob_n_Cf,
                ELS_Coupon_Prob_n_Cf,
                IR_Prob_n_Cf,
                PV01,
                ResultPrice
        );

        System.out.printf("PRICE: %.10f%n", ResultPrice[0]);
    }
}
