set -x -e

# Get misra
MISRA_REPO='https://github.com/furdog/MISRA.git'
if cd misra; then git pull; cd ..; else git clone "$MISRA_REPO"; fi
MISRA='./MISRA/misra.sh'
"$MISRA" setup

#HEADER="bite.h"
#SOURCE="bite.test.c"
HEADER="bite_mini.h"
SOURCE="bite_mini.c"

cppcheck --dump --std=c89 ${HEADER} --check-level=exhaustive

"$MISRA" check ${HEADER}

gcc ${SOURCE} -std=c89 -pedantic -Wall -Wextra -g \
	      -fsanitize=undefined -fsanitize-undefined-trap-on-error \
	      -DBITE_DEBUG -DBITE_COLOR -DBITE_DEBUG_BUFFER_OVERFLOW \
	      #-DBITE_PEDANTIC -DBITE_UNSAFE_OPTIMIZATIONS

gcc ${SOURCE} -std=c89 -pedantic -Wall -Wextra -O3 -S &>/dev/null \
	      -masm=intel -DBITE_UNSAFE_OPTIMIZATIONS #-fverbose-asm

rm *.ctu-info *.dump || true

./a
rm a
