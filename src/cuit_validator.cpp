#include "cuit_validator.hpp"
#include <cctype>
#include <string>

bool esCUITValido(std::string_view cuit) noexcept {
    std::string limpio;
    limpio.reserve(11);
    for (char c : cuit) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            limpio += c;
        }
    }
    if (limpio.size() != 11) return false;

    constexpr int pesos[] = {5, 4, 3, 2, 7, 6, 5, 4, 3, 2};
    int suma = 0;
    for (int i = 0; i < 10; ++i) {
        suma += (limpio[i] - '0') * pesos[i];
    }

    int resto = suma % 11;
    int digitoVerificador = 11 - resto;
    if (digitoVerificador == 11) digitoVerificador = 0;
    if (digitoVerificador == 10) return false;

    return digitoVerificador == (limpio[10] - '0');
}
