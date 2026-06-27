#!/usr/bin/env bash
g++ -std=c++20 -Iinclude -I/usr/include/postgresql \
    src/main.cpp \
    src/cuit_validator.cpp \
    src/cliente.cpp \
    src/factura.cpp \
    src/arca_service.cpp \
    src/database.cpp \
    src/email_service.cpp \
    src/reporte_pdf.cpp \
    -o facturacion \
    -lpq -lcurl -lhpdf \
    && ./facturacion
