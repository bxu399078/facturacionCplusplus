#pragma once

#include <vector>
#include <chrono>
#include "cliente.hpp"
#include "producto.hpp"

struct ItemFactura {
    Producto producto;
    int cantidad = 1;
};

class Factura {
public:
    Factura() = default;
    Factura(int numero, TipoFactura tipo, std::chrono::year_month_day fecha, Cliente cliente);

    void agregarItem(const Producto& producto, int cantidad);
    void calcular();
    void mostrar() const;
    void asignarCAE(std::string cae, std::string vencimiento);

    [[nodiscard]] int numero() const noexcept { return numero_; }
    [[nodiscard]] TipoFactura tipo() const noexcept { return tipo_; }
    [[nodiscard]] const Cliente& cliente() const noexcept { return cliente_; }
    [[nodiscard]] const std::vector<ItemFactura>& items() const noexcept { return items_; }
    [[nodiscard]] double subtotalNeto() const noexcept { return subtotalNeto_; }
    [[nodiscard]] double totalIVA() const noexcept { return totalIVA_; }
    [[nodiscard]] double totalFinal() const noexcept { return totalFinal_; }
    [[nodiscard]] const std::string& cae() const noexcept { return cae_; }
    [[nodiscard]] const std::string& caeVencimiento() const noexcept { return caeVencimiento_; }

private:
    int numero_ = 0;
    TipoFactura tipo_ = TipoFactura::A;
    std::chrono::year_month_day fecha_;
    Cliente cliente_;
    std::vector<ItemFactura> items_;
    double subtotalNeto_ = 0.0;
    double totalIVA_ = 0.0;
    double totalFinal_ = 0.0;
    std::string cae_;
    std::string caeVencimiento_;
};
