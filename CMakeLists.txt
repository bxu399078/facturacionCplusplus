cmake_minimum_required(VERSION 3.16)
project(Facturacion VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PostgreSQL REQUIRED)
find_package(CURL REQUIRED)

find_library(HPDF_LIBRARY NAMES hpdf hpdf-2.3.0)
find_path(HPDF_INCLUDE_DIR hpdf.h)

add_executable(facturacion
    src/main.cpp
    src/cuit_validator.cpp
    src/cliente.cpp
    src/factura.cpp
    src/arca_service.cpp
    src/database.cpp
    src/email_service.cpp
    src/reporte_pdf.cpp
)

target_include_directories(facturacion PRIVATE include ${PostgreSQL_INCLUDE_DIRS} ${HPDF_INCLUDE_DIR})
target_link_libraries(facturacion PRIVATE ${PostgreSQL_LIBRARIES} CURL::libcurl ${HPDF_LIBRARY})

# Descomentar para habilitar el modo real con ARCA (requiere openssl):
# target_compile_definitions(facturacion PRIVATE ARCA_REAL)
# find_package(OpenSSL REQUIRED)
# target_link_libraries(facturacion PRIVATE OpenSSL::SSL OpenSSL::Crypto)
