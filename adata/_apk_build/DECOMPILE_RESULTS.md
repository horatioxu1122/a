# APK Decompilation: NDK vs Non-NDK

Tested 2026-03-23. Built same Kotlin app two ways, decompiled both with apktool and jadx.

## Builds

| APK | Size | Contents |
|-----|------|----------|
| non-ndk.apk | 779K | Kotlin only (WebView + Termux launcher) |
| ndk.apk | 784K | Same Kotlin + native C lib (libnative.so, arm64-v8a) |

## Tools

- **apktool 2.10.0**: Decodes APK to smali (Dalvik bytecode) + resources + native libs
- **jadx 1.5.1**: Decompiles DEX back to readable Java source

## Results

Both APKs decompile fully. No difference in decompilability.

### Java/Kotlin layer (jadx)

Jadx recovers near-identical readable Java from both. All string constants, URLs, HTML templates, method logic, class structure — fully visible. The decompiled `M.java` is a clean 1:1 mapping of the original `M.kt` with Kotlin compiler artifacts (`Intrinsics.checkNotNull`, `@Metadata` annotations).

### Smali layer (apktool)

Both produce identical smali output for app classes. NDK build additionally extracts `lib/arm64-v8a/libnative.so`.

### Native code (NDK only)

The `.so` is extracted as-is. It's a stripped ARM64 ELF. `nm -D` shows exported JNI symbol:

```
0000000000000624 T Java_com_aios_a_M_nativeHello
```

JNI function naming convention leaks the class and method name. C source is not recoverable (only ARM64 disassembly), but the API surface is exposed.

## Conclusion

NDK does not protect code from decompilation:
- Kotlin/Java side decompiles identically regardless of NDK presence
- Native .so hides C implementation but exposes all JNI function names
- For obfuscation: R8/ProGuard on Java side, `-fvisibility=hidden` + symbol stripping on native side
- Neither provides real protection against a determined reverse engineer

## Reproduction

```bash
JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64

# Non-NDK build (remove externalNativeBuild from build.gradle.kts)
./gradlew assembleDebug

# NDK build (add cmake path + ndk abiFilters to build.gradle.kts)
./gradlew clean assembleDebug

# Decompile
java -jar tools/apktool.jar d -f -o out/apktool-X out/X.apk
tools/jadx/bin/jadx -d out/jadx-X --no-res out/X.apk
```
