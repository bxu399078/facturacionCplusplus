#pragma once

#include <string>
#include <vector>
#include <optional>
#include "cliente.hpp"

struct ProductoDB {
    int id;
    std::string nombre;
    int cantidad;
    double precio;
};

class DatabaseManager {
public:
    explicit DatabaseManager(std::string connString);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    DatabaseManager(DatabaseManager&&) = delete;
    DatabaseManager& operator=(DatabaseManager&&) = delete;

    bool conectar();
    std::optional<std::vector<Cliente>> obtenerClientes();
    std::optional<std::vector<ProductoDB>> obtenerProductos();

    [[nodiscard]] void* conn() const noexcept { return conn_; }

private:
    std::string connString_;
    void* conn_ = nullptr;
};
