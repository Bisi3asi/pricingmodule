package org.jna;

import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

// 모든 필드는 public이어야 한다.
// 모든 배열은 네이티브 메모리에 매핑되어야 한다.
public class MyStruct extends Structure {
    public String positionNm;
    public Pointer bucket;   // 배열은 Pointer를 사용해 메모리를 직접 할당해주고 매핑함
    public int bucketSize;
    public int tenor;
    public double value;
    public String bsdt;

    public MyStruct(String positionNm, String[] bucket, int bucketSize, String bsdt, int tenor, double value) {
        this.positionNm = positionNm;
        this.bucketSize = bucketSize;
        this.bsdt = bsdt;
        this.tenor = tenor;
        this.value = value;

        // String[]을 네이티브 메모리로 변환 (char**)
        Pointer[] bucketPointers = new Pointer[bucketSize];
        for (int i = 0; i < bucketSize; i++) {
            bucketPointers[i] = new Memory(bucket[i].length() + 1);  // 문자열 메모리 할당
            bucketPointers[i].setString(0, bucket[i]); // 문자열 복사
        }

        // 포인터 배열을 메모리에 할당
        this.bucket = new Memory(Native.POINTER_SIZE * bucketSize);
        for (int i = 0; i < bucketSize; i++) {
            this.bucket.setPointer(i * Native.POINTER_SIZE, bucketPointers[i]);
        }

        write(); // 네이티브 메모리에 동기화
    }

    // 구조체의 크기를 알 수 있도록 반드시 오버라이딩
    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("positionNm", "bucket", "bucketSize", "bsdt", "tenor", "value");
    }

}
