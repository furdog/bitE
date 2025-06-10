MISRA="../misra"
HEADER="bite.h"
SOURCE="bite.test.c"

cppcheck --dump --std=c89 ${HEADER} --check-level=exhaustive
python ${MISRA}/misra.py ${HEADER}.dump --rule-texts=${MISRA}/misra_c_2023__headlines_for_cppcheck.txt

gcc ${SOURCE} -std=c89 -pedantic -Wall -Wextra -g -fsanitize=undefined -fsanitize-undefined-trap-on-error -DBITE_DEBUG
gcc ${SOURCE} -std=c89 -pedantic -Wall -Wextra -O3 -S &>/dev/null -masm=intel #-fverbose-asm
