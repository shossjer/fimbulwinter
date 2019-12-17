
include (src/CMakeLists.txt)

add_library(utility "")
target_sources(utility PRIVATE ${SOURCES_UTILITY} ${HEADERS_UTILITY})
target_include_directories(utility PUBLIC "src" PRIVATE "${GENERAL_INCLUDES}")
target_link_libraries(utility PRIVATE generated)

add_library(core "")
target_sources(core PRIVATE ${SOURCES_CORE} ${HEADERS_CORE})
target_include_directories(core PUBLIC "src" PRIVATE "${GENERAL_INCLUDES}")
target_link_libraries(core PRIVATE generated utility)

add_library(engine "")
target_sources(engine PRIVATE ${SOURCES_ENGINE} ${HEADERS_ENGINE})
target_include_directories(engine PUBLIC "src" PRIVATE "${GENERAL_INCLUDES}")
target_link_libraries(engine PRIVATE generated utility core)

add_library(gameplay "")
target_sources(gameplay PRIVATE ${SOURCES_GAMEPLAY} ${HEADERS_GAMEPLAY})
target_include_directories(gameplay PUBLIC "src" PRIVATE "${GENERAL_INCLUDES}")
target_link_libraries(gameplay PRIVATE generated utility core engine)
