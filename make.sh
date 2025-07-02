MISRA="../misra"
HEADER="bite.h"
SOURCE="bite.test.c"

cppcheck --dump --std=c89 ${HEADER} --check-level=exhaustive

python ${MISRA}/misra.py ${HEADER}.dump \
	--rule-texts=${MISRA}/misra_c_2023__headlines_for_cppcheck.txt

gcc ${SOURCE} -std=c89 -pedantic -Wall -Wextra -g \
	      -fsanitize=undefined -fsanitize-undefined-trap-on-error \
	      -DBITE_DEBUG -DBITE_COLOR -DBITE_DEBUG_BUFFER_OVERFLOW \
	      #-DBITE_PEDANTIC -DBITE_UNSAFE_OPTIMIZATIONS

gcc ${SOURCE} -std=c89 -pedantic -Wall -Wextra -O3 -S &>/dev/null \
	      -masm=intel -DBITE_UNSAFE_OPTIMIZATIONS #-fverbose-asm

rm *.h.ctu-info *.h.dump
