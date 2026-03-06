#if 0
#!/bin/sh
set -e

echo "=== TCC (-run) ==="
/usr/bin/time -p tcc -run "$0" bootstrapped

echo "=== TCC (compile to binary) ==="
/usr/bin/time -p tcc -o "${0%.c}" "$0"
/usr/bin/time -p "${0%.c}" bootstrapped

echo "=== Clang (-O0) ==="
/usr/bin/time -p clang -O0 -o "${0%.c}" "$0"
/usr/bin/time -p "${0%.c}" bootstrapped

echo "=== Clang (-O3) ==="
/usr/bin/time -p clang -O3 -o "${0%.c}" "$0"
/usr/bin/time -p "${0%.c}" bootstrapped

rm -f "${0%.c}"
exit 0
#endif

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc > 1) {
        printf("Hello from %s!\n", argv[1]);
        return 0;
    }
    printf("Hello from direct execution!\n");
    return 0;
}
