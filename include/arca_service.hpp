#pragma once

#include <string>
#include <optional>
#include "factura.hpp"

class ArcaService {
public:
    explicit ArcaService(bool mock = true);

    bool autenticar(const std::string& certPath, const std::string& keyPath);
    std::optional<std::string> solicitarCAE(Factura& factura);

    [[nodiscard]] const std::string& token() const noexcept { return token_; }
    [[nodiscard]] const std::string& sign() const noexcept { return sign_; }
    [[nodiscard]] const std::string& cae() const noexcept { return cae_; }
    [[nodiscard]] const std::string& caeVencimiento() const noexcept { return caeVencimiento_; }

private:
    bool mock_;
    std::string token_;
    std::string sign_;
    std::string cae_;
    std::string caeVencimiento_;

    bool mockAutenticar();
    std::optional<std::string> mockSolicitarCAE(Factura& factura);

    bool realAutenticar(const std::string& certPath, const std::string& keyPath);
    std::optional<std::string> realSolicitarCAE(Factura& factura);
};
