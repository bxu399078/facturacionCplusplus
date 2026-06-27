#include "factura.hpp"
#include "terminal.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <cmath>

static std::string fechaToString(const std::chrono::year_month_day& fecha) {
    std::ostringstream oss;
    oss << static_cast<int>(fecha.year()) << '-'
        << std::setw(2) << std::setfill('0')
        << static_cast<unsigned>(fecha.month()) << '-'
        << std::setw(2) << std::setfill('0')
        << static_cast<unsigned>(fecha.day());
    return oss.str();
}

Factura::Factura(int numero, TipoFactura tipo, std::chrono::year_month_day fecha, Cliente cliente)
    : numero_(numero)
    , tipo_(tipo)
    , fecha_(fecha)
    , cliente_(std::move(cliente)) {}

void Factura::agregarItem(const Producto& producto, int cantidad) {
    items_.push_back({producto, cantidad});
}

void Factura::asignarCAE(std::string cae, std::string vencimiento) {
    cae_ = std::move(cae);
    caeVencimiento_ = std::move(vencimiento);
}

void Factura::calcular() {
    subtotalNeto_ = 0.0;
    totalIVA_ = 0.0;
    totalFinal_ = 0.0;

    for (const auto& item : items_) {
        double netoItem = item.producto.precioNeto() * item.cantidad;
        double ivaItem  = netoItem * item.producto.alicuotaIVA();
        subtotalNeto_ += netoItem;
        totalIVA_     += ivaItem;
    }

    switch (tipo_) {
        case TipoFactura::A:
            totalFinal_ = subtotalNeto_ + totalIVA_;
            break;
        case TipoFactura::B:
            totalFinal_ = 0.0;
            for (const auto& item : items_) {
                totalFinal_ += item.producto.precioNeto()
                             * (1.0 + item.producto.alicuotaIVA())
                             * item.cantidad;
            }
            break;
        case TipoFactura::C:
            totalFinal_ = subtotalNeto_;
            totalIVA_ = 0.0;
            break;
    }
}

void Factura::mostrar() const {
    constexpr int W = 72;
    auto sep = [&](char c = '=') {
        std::cout << term::DIM << std::string(W, c) << term::RESET << "\n";
    };

    sep();
    std::cout << term::BOLD << term::BLUE
              << "          FACTURA TIPO " << toString(tipo_)
              << " - Nro " << std::setw(4) << std::setfill('0') << numero_
              << term::RESET << "\n" << std::setfill(' ');
    if (!cae_.empty()) {
        std::cout << "          " << term::YELLOW << "CAE: " << cae_
                  << " - Vto: " << caeVencimiento_ << term::RESET << "\n";
    }
    sep('-');
    std::cout << "  " << term::BOLD << "Cliente:" << term::RESET << " " << cliente_.nombre() << "\n"
              << "  " << term::BOLD << "CUIT:" << term::RESET << "    " << cliente_.cuit() << "\n"
              << "  " << term::BOLD << "IVA:" << term::RESET << "     " << toString(cliente_.condicionIVA()) << "\n"
              << "  " << term::BOLD << "Fecha:" << term::RESET << "   " << fechaToString(fecha_) << "\n";
    sep('-');

    auto header = [&] {
        std::cout << term::BOLD << term::CYAN
                  << std::left
                  << std::setw(9) << "Codigo"
                  << std::setw(22) << "Descripcion"
                  << std::right
                  << std::setw(5) << "Cant"
                  << std::setw(12) << "Precio"
                  << std::setw(12) << "Subtotal";
        if (tipo_ == TipoFactura::A) {
            std::cout << std::setw(10) << "IVA";
        }
        std::cout << term::RESET << "\n";
    };
    header();

    for (const auto& item : items_) {
        double netoItem = item.producto.precioNeto() * item.cantidad;
        std::cout << std::left
                  << std::setw(9) << item.producto.codigo()
                  << std::setw(22) << item.producto.descripcion()
                  << std::right
                  << std::setw(5) << item.cantidad
                  << std::fixed << std::setprecision(2);

        if (tipo_ == TipoFactura::A) {
            double ivaItem = netoItem * item.producto.alicuotaIVA();
            std::cout << std::setw(12) << item.producto.precioNeto()
                      << std::setw(12) << netoItem
                      << std::setw(10) << ivaItem;
        } else if (tipo_ == TipoFactura::B) {
            double precioFinal = item.producto.precioNeto()
                               * (1.0 + item.producto.alicuotaIVA());
            std::cout << std::setw(12) << precioFinal
                      << std::setw(12) << (precioFinal * item.cantidad);
        } else {
            std::cout << std::setw(12) << item.producto.precioNeto()
                      << std::setw(12) << netoItem;
        }
        std::cout << "\n";
    }
    sep('-');

    auto printLine = [](const std::string& label, double value, const char* color = "") {
        std::cout << color << std::right << std::setw(48) << label
                  << std::fixed << std::setprecision(2)
                  << std::setw(12) << value << term::RESET << "\n";
    };

    if (tipo_ == TipoFactura::A) {
        printLine("Subtotal Neto:", subtotalNeto_);

        std::map<double, double> ivaPorTasa;
        for (const auto& item : items_) {
            double netoItem = item.producto.precioNeto() * item.cantidad;
            ivaPorTasa[item.producto.alicuotaIVA()]
                += netoItem * item.producto.alicuotaIVA();
        }
        for (const auto& [tasa, monto] : ivaPorTasa) {
            std::ostringstream lbl;
            lbl << "IVA " << static_cast<int>(std::round(tasa * 100)) << "%:";
            printLine(lbl.str(), monto, term::YELLOW);
        }
        sep('-');
        printLine("TOTAL:", totalFinal_, term::GREEN);
    } else if (tipo_ == TipoFactura::B) {
        printLine("TOTAL (IVA incluido):", totalFinal_, term::GREEN);
    } else {
        printLine("TOTAL:", totalFinal_, term::GREEN);
    }
    sep();
}
