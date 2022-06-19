# Script to do valgrind memcheck
for f in *.lil; do
    valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes  --log-file=${f}.log  ./lil ${f}
done

