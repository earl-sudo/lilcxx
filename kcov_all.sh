# Script to produce coverage information using kcov.
for f in *.lil; do
    kcov /home/heron/lil/lil/kcov ./lil ${f}
done

