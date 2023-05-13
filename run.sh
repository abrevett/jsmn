#!/bin/bash
run_2m () {
    cat test-large-2M.json | ./jsondump > /dev/null
}

run_8m() {
    cat test-large-8M.json | ./jsondump > /dev/null
}

run_8m_par() {
    ./main2 > /dev/null
}

echo '2MB run:'
time run_2m

echo '8MB run:'
time run_8m

echo '8MB run w/ pthreads:'
time run_8m_par
