#pragma once

#include <string>

class Producto {
public:
    Producto() = default;
    Producto(std::string codigo, std::string descripcion, double precioNeto, double alicuotaIVA)
        : codigo_(std::move(codigo))
        , descripcion_(std::move(descripcion))
        , precioNeto_(precioNeto)
        , alicuotaIVA_(alicuotaIVA) {}

    [[nodiscard]] const std::string& codigo() const noexcept { return codigo_; }
    [[nodiscard]] const std::string& descripcion() const noexcept { return descripcion_; }
    [[nodiscard]] double precioNeto() const noexcept { return precioNeto_; }
    [[nodiscard]] double alicuotaIVA() const noexcept { return alicuotaIVA_; }

private:
    std::string codigo_;
    std::string descripcion_;
    double precioNeto_ = 0.0;
    double alicuotaIVA_ = 0.21;
};
