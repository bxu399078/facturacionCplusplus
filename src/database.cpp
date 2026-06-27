#include "database.hpp"
#include <libpq-fe.h>
#include <iostream>

static PGconn* pg(void* p) { return static_cast<PGconn*>(p); }

static CondicionIVA toCondicion(const std::string& s) {
    if (s == "ResponsableInscripto") return CondicionIVA::ResponsableInscripto;
    if (s == "ConsumidorFinal")      return CondicionIVA::ConsumidorFinal;
    if (s == "Monotributista")       return CondicionIVA::Monotributista;
    return CondicionIVA::Exento;
}

DatabaseManager::DatabaseManager(std::string connString)
    : connString_(std::move(connString)) {}

DatabaseManager::~DatabaseManager() {
    if (conn_) PQfinish(pg(conn_));
}

bool DatabaseManager::conectar() {
    conn_ = PQconnectdb(connString_.c_str());
    if (PQstatus(pg(conn_)) != CONNECTION_OK) {
        std::cerr << "  Error BD: " << PQerrorMessage(pg(conn_));
        PQfinish(pg(conn_));
        conn_ = nullptr;
        return false;
    }
    return true;
}

std::optional<std::vector<Cliente>> DatabaseManager::obtenerClientes() {
    if (!conn_) return std::nullopt;

    PGresult* res = PQexec(pg(conn_),
        "SELECT id, nombre, cuit, condicion_iva FROM cliente ORDER BY id");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "  Error query: " << PQerrorMessage(pg(conn_));
        PQclear(res);
        return std::nullopt;
    }

    int n = PQntuples(res);
    std::vector<Cliente> clientes;
    clientes.reserve(static_cast<size_t>(n));

    for (int i = 0; i < n; ++i) {
        clientes.emplace_back(
            PQgetvalue(res, i, 1),
            PQgetvalue(res, i, 2),
            toCondicion(PQgetvalue(res, i, 3))
        );
    }

    PQclear(res);
    return clientes;
}

std::optional<std::vector<ProductoDB>> DatabaseManager::obtenerProductos() {
    if (!conn_) return std::nullopt;

    PGresult* res = PQexec(pg(conn_),
        "SELECT id, nombre, cantidad, precio FROM productos ORDER BY id");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "  Error query: " << PQerrorMessage(pg(conn_));
        PQclear(res);
        return std::nullopt;
    }

    int n = PQntuples(res);
    std::vector<ProductoDB> productos;
    productos.reserve(static_cast<size_t>(n));

    for (int i = 0; i < n; ++i) {
        productos.push_back({
            std::stoi(PQgetvalue(res, i, 0)),
            PQgetvalue(res, i, 1),
            std::stoi(PQgetvalue(res, i, 2)),
            std::stod(PQgetvalue(res, i, 3))
        });
    }

    PQclear(res);
    return productos;
}
