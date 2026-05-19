idf_component_register(
  SRCS "main.cpp"
  INCLUDE_DIRS "." "../../common" "../../.."
  REQUIRES SystemChrono esp_timer esp_rom freertos vfs
)
