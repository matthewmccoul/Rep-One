# Runs: <EXE> <SUB> <PUZ> <ARG>; requires exit code 1 and an output that starts with WANT (a "FAIL <CODE>" line).
execute_process(COMMAND ${EXE} ${SUB} ${PUZ} ${ARG} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 1)
  message(FATAL_ERROR "expected exit 1, got '${rc}'. output: ${out} ${err}")
endif()
string(FIND "${out}" "${WANT}" pos)
if(NOT pos EQUAL 0)
  message(FATAL_ERROR "output does not start with '${WANT}': ${out}")
endif()
