idf_component_register(
  SRCS "src/SystemChrono.cpp"
  INCLUDE_DIRS "include"
  REQUIRES esp_timer
)

target_compile_definitions(${COMPONENT_LIB}
  PUBLIC SYSTEMCHRONO_PLATFORM_IDF=1
)
