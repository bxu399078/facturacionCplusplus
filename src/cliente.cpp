#include "cliente.hpp"
#include <iostream>

Cliente::Cliente(std::string nombre, std::string cuit, CondicionIVA condicion)
    : nombre_(std::move(nombre))
    , cuit_(std::move(cuit))
    , condicion_(condicion) {}

void Cliente::mostrar() const {
    std::cout << "  Cliente: " << nombre_ << "\n"
              << "  CUIT:    " << cuit_ << "\n"
              << "  IVA:     " << toString(condicion_) << "\n";
}
