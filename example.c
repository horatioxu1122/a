#if 0
B="$HOME/.cache/a";mkdir -p "$B";N="$B/$(basename "$0" .c)";T=$(mktemp)
[ "$N" -nt "$0" ]&&exec "$N" "$@"
clang -O3 -march=native -flto -w -o "$N" "$0" &>/dev/null &
tcc -run "$0" "$@" 2>"$T"||{ cat "$T";echo "try: a c \"$(tail -1 "$T")\"";};rm -f "$T";exit
#endif
#include <stdio.h>
int main(int ac, char **av) {
    for (int i = 0; i < ac; i++) printf("%s%s", i?" ":"", av[i]);
    puts("");
}
