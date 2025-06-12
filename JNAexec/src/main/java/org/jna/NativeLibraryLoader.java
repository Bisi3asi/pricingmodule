package org.jna;

import com.sun.jna.Memory;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Platform;
import com.sun.jna.ptr.DoubleByReference;
import lombok.Getter;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

@Getter
public class NativeLibraryLoader {
    private final String libraryPath;
    private final List<String> functions;
    private final NativeLibrary library;


    public NativeLibraryLoader(String libraryPath) {
        this.libraryPath = libraryPath;
        this.functions = new ArrayList<>();
        this.library = NativeLibrary.getInstance(libraryPath);
        loadFunctionList();
    }

    private void loadFunctionList() {
        try {
            ProcessBuilder processBuilder;

            if (Platform.isWindows()) {
                processBuilder = new ProcessBuilder("cmd.exe", "/c", "dumpbin /EXPORTS \"" + libraryPath + "\"");
                processBuilder.redirectErrorStream(true);
            } else if (Platform.isLinux()) {
                // Linux : 함수 목록 파싱 (nm -D)
                processBuilder = new ProcessBuilder("nm", "-D", libraryPath);
            } else {
                return;
            }

            Process process = processBuilder.start();
            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));

            String line = "";
            while ((line = reader.readLine()) != null) {
                // Windows : 함수 목록 파싱 (dumpbin /EXPORTS)
                // 정규식 1: @ILT+xxx(functionName)
                String iltPattern = ".*\\((\\w+)\\).*";
                // 정규식 2: ordinal hint RVA functionName (e.g. 1 0 00028C30 pricing)
                String plainExportPattern = "^\\s*\\d+\\s+\\d+\\s+[0-9A-Fa-f]+\\s+(\\w+)$";

                Pattern pattern1 = Pattern.compile(iltPattern);
                Pattern pattern2 = Pattern.compile(plainExportPattern);

                Matcher matcher1 = pattern1.matcher(line);
                Matcher matcher2 = pattern2.matcher(line);

                if (matcher1.matches()) {
                    String name = matcher1.group(1);
                    if (!name.equals("R") && !name.equals("C")) {
                        functions.add(name);
                    }
                } else if (matcher2.matches()) {
                    String name = matcher2.group(1);
                    if (!name.equals("R") && !name.equals("C")) {
                        functions.add(name);
                    }
                } else {
                    // Linux : 함수 목록 파싱 (nm -ㅇ)
                    if (line.contains(" T ")) {
                        String[] parts = line.trim().split("\\s+");
                        // "__init", "__fini" 등 네이티브 함수 제외
                        if (!parts[parts.length - 1].startsWith("_")) {
                            functions.add(parts[parts.length - 1]); // 마지막 컬럼이 함수명
                        }
                    }
                }
            }
            reader.close();
            process.waitFor();
        } catch (Exception e) {
            throw new RuntimeException("Failed to load functions from native library: " + e.getMessage(), e);
        }
    }

    // function (Div, Mul, Sum, Min)
    // java에서 던진 double 형을 동적 라이브러리에서 받아 연산한 값을 리턴한다.
    public String callMathFunction(String functionName,
                                   double param1, double param2) {
        try {
            return library.getFunction(functionName).invoke(double.class, new Object[]{param1, param2}).toString();
        } catch (Exception e) {
            throw new RuntimeException("Failed to call function: " + functionName, e);
        }
    }

    // function (printType)
    // 1. java에서 던진 array 형을 동적 라이브러리에서 받아 출력한다. (param 1, param 2, param 3, param 4)
    // 2. java에서 double 형 포인터 참조값을, 동적 라이브러리에서 받아 1 증가시킨 후, 각각 라이브러리와 java에서 출력한다. (param 5)
    public void callPrintType(String functionName,
                              String[] strArr, int[] intArr, double[] doubleArr, long[] longArr, double value,
                              int strArrSize, int intArrSize, int doubleArrSize, int longArrsize
    ) {
        DoubleByReference doublePtr = new DoubleByReference(value);
        try {
            library.getFunction(functionName).invoke(void.class, new Object[]{
                    strArr, intArr, doubleArr, longArr, doublePtr,
                    strArrSize, intArrSize, doubleArrSize, longArrsize
            });
            System.out.println("java catched updated pointer value after function call: " + doublePtr.getValue());
        } catch (Exception e) {
            throw new RuntimeException("Failed to call function: " + functionName, e);
        }
    }

    // function (printStruct)
    // java에서 던진 각 값을 구조체로 만들어 던진 후 동적 라이브러리에서 받아 라이브러리에서 출력한다.
    public void callPrintStruct(String functionName,
                                String positionNm, String[] bucket, int bucketSize, String bsdt, int tenor, double value) {
        MyStruct struct = new MyStruct(positionNm, bucket, bucketSize, bsdt, tenor, value);
        try {
            library.getFunction(functionName).invoke(void.class, new Object[]{struct});
        } catch (Exception e) {
            throw new RuntimeException("Failed to call function: " + functionName, e);
        }
    }


    // function (updateDoubleArr)
    // java에서 배열도 포인터 개념으로 실행되는지
    public void callUpdateDoubleArr(String functionName,
                                    double[] doubleArr) {
        ;
        try {
            library.getFunction(functionName).invoke(void.class, new Object[]{doubleArr, doubleArr.length});
            System.out.println("java catched updated double array values after function call:");
            for (int i = 0; i < doubleArr.length; i++) {
                System.out.println("Index " + i + ": " + doubleArr[i]);
            }
        } catch (Exception e) {
            throw new RuntimeException("Failed to call function: " + functionName, e);
        }
    }

    // function (OtStock)
    // StockPricing
    public void callStockPricing(String functionName,
                                 double amt, double price, double basePrice, double beta, double fx, double[] p, double[] resultPrice) {
        try {
//            DoubleByReference resultPricePtr = new DoubleByReference(resultPrice);

            long result = (Long) library.getFunction(functionName).invoke(long.class,
                    new Object[]{amt, price, basePrice, beta, fx, p, resultPrice});
            System.out.println("java catched updated result price after function call: " + resultPrice[0]);
        } catch (Exception e) {
            throw new RuntimeException("Failed to call function: OtStock", e);
        }
    }

    // function (fxForward)
    // pricing
    public void callFxFowardPricing(String functionName,
                                    long maturityDate,
                                    long revaluationDate,
                                    double exchangeRate,
                                    String buySideCurrency,
                                    double notionalForeign,
                                    int buySideDCB,
                                    String buySideDcCurve,
                                    int buyCurveSize,
                                    double[] buyCurveYearFrac,
                                    double[] buyMarketData,
                                    String sellSideCurrency,
                                    double notionalDomestic,
                                    int sellSideDCB,
                                    String sellSideDcCurve,
                                    int sellCurveSize,
                                    double[] sellCurveYearFrac,
                                    double[] sellMarketData,
                                    int calType,
                                    int logYn,
                                    double[] resultNetPvFxSensitivity,
                                    double[] resultGirrDelta) {
        try {
            library.getFunction(functionName).invoke(void.class, new Object[]{
                    maturityDate,
                    revaluationDate,
                    exchangeRate,
                    buySideCurrency,
                    notionalForeign,
                    buySideDCB,
                    buySideDcCurve,
                    buyCurveSize,
                    buyCurveYearFrac,
                    buyMarketData,
                    sellSideCurrency,
                    notionalDomestic,
                    sellSideDCB,
                    sellSideDcCurve,
                    sellCurveSize,
                    sellCurveYearFrac,
                    sellMarketData,
                    calType,
                    logYn,
                    resultNetPvFxSensitivity,
                    resultGirrDelta
            });

            // 결과 확인
            System.out.printf("Net PV: %.2f%n", resultNetPvFxSensitivity[0]);
            System.out.printf("FX Sensitivity: %.2f%n", resultNetPvFxSensitivity[1]);

            int size = (int) resultGirrDelta[0];
            System.out.println("GIRR Delta Size: " + size);
            for (int i = 1; i <= size; i++) {
                System.out.printf("%d. Tenor: %.2f%n", i, resultGirrDelta[i]);
            }
            for (int i = size + 1; i <= size * 2; i++) {
                System.out.printf("%d. Sensitivity: %.2f%n", i - size, resultGirrDelta[i]);
            }

        } catch (Exception e) {
            throw new RuntimeException("Failed to call function: fxForward pricing", e);
        }
    }

    public double callFixedRateBondPricing(String functionName,
                                           int evaluationDate,
                                           int settlementDays,
                                           int issueDate,
                                           int maturityDate,
                                           double notional,
                                           double couponRate,
                                           int couponDayCounter,
                                           int numberOfCoupons,
                                           int[] paymentDates,
                                           int[] realStartDates,
                                           int[] realEndDates,
                                           int numberOfGirrTenors,
                                           int[] girrTenorDays,
                                           double[] girrRates,
                                           int girrDayCounter,
                                           int girrInterpolator,
                                           int girrCompounding,
                                           int girrFrequency,
                                           double spreadOverYield,
                                           int spreadOverYieldCompounding,
                                           int spreadOverYieldDayCounter,
                                           int numberOfCsrTenors,
                                           int[] csrTenorDays,
                                           double[] csrRates,
                                           int calType,
                                           int logYn,
                                           double[] resultGirrDelta,
                                           double[] resultCsrDelta
    ) {
        try {
            return (double) library.getFunction(functionName).invoke(double.class, new Object[]{
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
            });
        } catch (Exception e) {
            throw new RuntimeException("Failed to call pricing function", e);
        }
    }

    public void callKISPCalcHiFiveSwapMC(String functionName,
                                         int priceDate,
                                         double notionalAMT,
                                         double[] underlyingPrice,
                                         double[] tCurveInfo,
                                         double[] curveInfo,
                                         int[] nt,
                                         double[] tVol,
                                         double[] parity,
                                         double[] localVolSurface,
                                         int[] nVol,
                                         int[] nParity,
                                         double[] underlyingCorr,
                                         int[] tDiv,
                                         double[] div,
                                         int[] nDiv,
                                         int[] divTypeFlag,
                                         double[] fx_tVol,
                                         double[] fx_vol,
                                         int[] fx_nVol,
                                         double[] fx_corr,
                                         int[] quantoFlag,
                                         int nStock,
                                         int nStrike,
                                         double[] x,
                                         double[] ko_barrier,
                                         double[] ki_barrier,
                                         double[] payOffSlope,
                                         double[] payOffAmount,
                                         int[] datesInfo,
                                         double[] redemCoupon,
                                         int[] knockBarrierFlag,
                                         int bestWorstFlag,
                                         int[] beKO_hitted,
                                         int beKI_hitted,
                                         int nCoupon,
                                         double[] couponBarrier,
                                         double[] coupon,
                                         int[] couponDatesInfo,
                                         int couponBarrierFlag,
                                         int beCouponBarrierHitted,
                                         double nTrials,
                                         double[] payOffLimit,
                                         double[] sc,
                                         double[] coupon_sc,
                                         int[] flLongInfo,
                                         double[] flCurveTerm,
                                         double[] flCurveRate,
                                         int[] flCurveDate,
                                         double[] historyRate,
                                         double flRefRatePeriod,
                                         int[] flResetFixDateInfo,
                                         int[] flCouponDateInfo,
                                         double[] flCouponInfo,
                                         int[] calcFlag,
                                         double[] ELS_Prob_n_Cf,
                                         double[] ELS_Coupon_Prob_n_Cf,
                                         double[] IR_Prob_n_Cf,
                                         double[] PV01,
                                         double[] ResultPrice
    ) {
        try {
            library.getFunction(functionName).invoke(long.class, new Object[]{
                    priceDate,
                    notionalAMT,
                    underlyingPrice,
                    tCurveInfo,
                    curveInfo,
                    nt,
                    tVol,
                    parity,
                    localVolSurface,
                    nVol,
                    nParity,
                    underlyingCorr,
                    tDiv,
                    div,
                    nDiv,
                    divTypeFlag,
                    fx_tVol,
                    fx_vol,
                    fx_nVol,
                    fx_corr,
                    quantoFlag,
                    nStock,
                    nStrike,
                    x,
                    ko_barrier,
                    ki_barrier,
                    payOffSlope,
                    payOffAmount,
                    datesInfo,
                    redemCoupon,
                    knockBarrierFlag,
                    bestWorstFlag,
                    beKO_hitted,
                    beKI_hitted,
                    nCoupon,
                    couponBarrier,
                    coupon,
                    couponDatesInfo,
                    couponBarrierFlag,
                    beCouponBarrierHitted,
                    nTrials,
                    payOffLimit,
                    sc,
                    coupon_sc,
                    flLongInfo,
                    flCurveTerm,
                    flCurveRate,
                    flCurveDate,
                    historyRate,
                    flRefRatePeriod,
                    flResetFixDateInfo,
                    flCouponDateInfo,
                    flCouponInfo,
                    calcFlag,
                    ELS_Prob_n_Cf,
                    ELS_Coupon_Prob_n_Cf,
                    IR_Prob_n_Cf,
                    PV01,
                    ResultPrice
//                    resultMem
            });

//            ResultPrice[0] = resultMem.getDouble(0);
//            ResultPrice[1] = resultMem.getDouble(8);
//            ResultPrice[2] = resultMem.getDouble(16);

        } catch (Exception e) {
            throw new RuntimeException("Failed to KISPCALC pricing function", e);
        }
    }
}
