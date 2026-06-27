#pragma once

#include <string_view>

enum class CondicionIVA {
    ResponsableInscripto,
    ConsumidorFinal,
    Monotributista,
    Exento
};

enum class TipoFactura {
    A,
    B,
    C
};

[[nodiscard]] constexpr std::string_view toString(CondicionIVA c) noexcept {
    using enum CondicionIVA;
    switch (c) {
        case ResponsableInscripto: return "Responsable Inscripto";
        case ConsumidorFinal:     return "Consumidor Final";
        case Monotributista:      return "Monotributista";
        case Exento:              return "Exento";
    }
    return "Desconocido";
}

[[nodiscard]] constexpr std::string_view toString(TipoFactura t) noexcept {
    using enum TipoFactura;
    switch (t) {
        case A: return "A";
        case B: return "B";
        case C: return "C";
    }
    return "?";
}

[[nodiscard]] constexpr TipoFactura tipoFacturaPara(CondicionIVA c) noexcept {
    using enum CondicionIVA;
    using enum TipoFactura;
    switch (c) {
        case ResponsableInscripto: return A;
        case ConsumidorFinal:      return B;
        case Monotributista:       return C;
        case Exento:               return C;
    }
    return A;
}
