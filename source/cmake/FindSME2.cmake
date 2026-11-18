if(APPLE)
    execute_process(COMMAND sysctl -n hw.optional.arm.FEAT_SME2
                    OUTPUT_VARIABLE sme2_features
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    execute_process(COMMAND cat /proc/cpuinfo
                    COMMAND grep Features
                    COMMAND grep -w sme2
                    OUTPUT_VARIABLE sme2_features
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(sme2_features)
    set(CPU_HAS_SME2 1)
endif()
