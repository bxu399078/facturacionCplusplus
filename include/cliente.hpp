#pragma once

#include <string>
#include "tipos.hpp"

class Cliente {
public:
    Cliente() = default;
    Cliente(std::string nombre, std::string cuit, CondicionIVA condicion);

    [[nodiscard]] const std::string& nombre() const noexcept { return nombre_; }
    [[nodiscard]] const std::string& cuit() const noexcept   { return cuit_; }
    [[nodiscard]] CondicionIVA condicionIVA() const noexcept { return condicion_; }

    void mostrar() const;

private:
    std::string nombre_;
    std::string cuit_;
    CondicionIVA condicion_ = CondicionIVA::ConsumidorFinal;
};
