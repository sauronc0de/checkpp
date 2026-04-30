if(NOT DEFINED CHECKPP_BIN)
  message(FATAL_ERROR "CHECKPP_BIN is required")
endif()

if(NOT DEFINED PROJECT_ROOT)
  message(FATAL_ERROR "PROJECT_ROOT is required")
endif()

if(NOT DEFINED COMPILE_DB_DIR)
  message(FATAL_ERROR "COMPILE_DB_DIR is required")
endif()

if(NOT DEFINED RULES_FILE)
  message(FATAL_ERROR "RULES_FILE is required")
endif()

if(NOT DEFINED EXPECT_EXIT_CODE)
  message(FATAL_ERROR "EXPECT_EXIT_CODE is required")
endif()

if(NOT DEFINED EXPECT_OUTPUT_REGEX)
  message(FATAL_ERROR "EXPECT_OUTPUT_REGEX is required")
endif()

set(_cmd
    "${CHECKPP_BIN}"
    "--plain-text"
    "--ignore-paths" "${IGNORE_PATHS_FILE}"
    "${PROJECT_ROOT}"
    "${COMPILE_DB_DIR}"
    "${RULES_FILE}")

execute_process(
  COMMAND ${_cmd}
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)

set(_output "${_stdout}${_stderr}")

if(NOT _result EQUAL EXPECT_EXIT_CODE)
  message(FATAL_ERROR "Unexpected exit code. Expected ${EXPECT_EXIT_CODE}, got ${_result}.\n${_output}")
endif()

string(REGEX MATCH "${EXPECT_OUTPUT_REGEX}" _match "${_output}")
if("${_match}" STREQUAL "")
  message(FATAL_ERROR "Expected output regex not found: ${EXPECT_OUTPUT_REGEX}\n${_output}")
endif()
