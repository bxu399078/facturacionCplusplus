#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <string>
#include <limits>
#include <csignal>
#include "tipos.hpp"
#include "cliente.hpp"
#include "producto.hpp"
#include "factura.hpp"
#include "cuit_validator.hpp"
#include "arca_service.hpp"
#include "database.hpp"
#include "email_service.hpp"
#include "reporte_pdf.hpp"
#include "terminal.hpp"

using namespace std::chrono;

static volatile sig_atomic_t signal_status = 0;

extern "C" void signalHandler(int signal) {
    signal_status = signal;
}

static void limpiarPantalla() {
    std::cout << "\033[2J\033[1;1H";
}

static int leerEntero(const std::string& mensaje, int min, int max) {
    int valor;
    while (true) {
        std::cout << term::BOLD << mensaje << term::RESET;
        std::cin >> valor;
        if (std::cin.fail() || valor < min || valor > max) {
            if (signal_status == SIGINT) {
                std::signal(SIGINT, signalHandler);
                signal_status = 0;
                return min - 1;
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << term::RED << "  Valor invalido. Intente de nuevo.\n" << term::RESET;
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return valor;
        }
    }
}

static double leerDouble(const std::string& mensaje, double min) {
    double valor;
    while (true) {
        std::cout << term::BOLD << mensaje << term::RESET;
        std::cin >> valor;
        if (std::cin.fail() || valor < min) {
            if (signal_status == SIGINT) {
                std::signal(SIGINT, signalHandler);
                signal_status = 0;
                return min - 1.0;
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << term::RED << "  Valor invalido. Intente de nuevo.\n" << term::RESET;
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return valor;
        }
    }
}

static std::string leerString(const std::string& mensaje) {
    std::string linea;
    std::cout << term::BOLD << mensaje << term::RESET;
    std::getline(std::cin, linea);
    if (signal_status == SIGINT) {
        std::signal(SIGINT, signalHandler);
        signal_status = 0;
        std::cin.clear();
    }
    return linea;
}

static double seleccionarAlicuota() {
    std::cout << "\n"
              << "  " << term::BOLD << term::CYAN << "Alicuota de IVA:" << term::RESET << "\n"
              << "    " << term::YELLOW << "1)" << term::RESET << " 21%\n"
              << "    " << term::YELLOW << "2)" << term::RESET << " 10.5%\n"
              << "    " << term::YELLOW << "3)" << term::RESET << " 0%\n";
    int opcion = leerEntero("  Elija (1-3): ", 1, 3);
    switch (opcion) {
        case 1: return 0.21;
        case 2: return 0.105;
        case 3: return 0.0;
        default: return 0.21;
    }
}

static void pausa() {
    std::cout << "\n  " << term::DIM << "Presione Enter para continuar..." << term::RESET;
    std::cin.get();
    if (signal_status == SIGINT) {
        std::signal(SIGINT, signalHandler);
        signal_status = 0;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

static void mostrarMenu() {
    limpiarPantalla();
    std::cout << term::BOLD << term::BLUE
              << "  ╔══════════════════════════════════════╗\n"
              << "  ║   SISTEMA DE FACTURACION - ARGENTINA ║\n"
              << "  ╚══════════════════════════════════════╝\n"
              << term::RESET << "\n"
              << "    " << term::YELLOW << "1)" << term::RESET << " Informes\n"
              << "    " << term::YELLOW << "2)" << term::RESET << " Nueva Facturacion\n"
              << "    " << term::YELLOW << "3)" << term::RESET << " Prueba\n"
              << "    " << term::RED    << "0)" << term::RESET << " Salir\n\n";
}

static void moduloInformes() {
    limpiarPantalla();
    term::titulo("INFORMES");
    std::cout << "\n"
              << "    " << term::YELLOW << "1)" << term::RESET << " Reporte de Clientes\n"
              << "    " << term::YELLOW << "2)" << term::RESET << " Listado de Productos\n"
              << "    " << term::RED    << "0)" << term::RESET << " Volver\n\n";
    int sub = leerEntero("  Elija una opcion: ", 0, 2);
    switch (sub) {
        case 1: generarReporteClientesPDF(); break;
        case 2: generarReporteProductosPDF(); break;
        case 0: break;
    }
    if (sub != 0) pausa();
}

static void prueba(){
    limpiarPantalla();
    term::titulo("PRUEBA");
    std::cout << "\n  " << term::ColorFelicidad << "Prueba Cazadoraaaa PaPaaaaaaa;;;;;;;;;" << term::RESET << "\n";
    pausa();
}

static void nuevaFacturacion() {
    limpiarPantalla();
    term::titulo("NUEVA FACTURACION");

    DatabaseManager db("host=localhost dbname=facturacioniva user=facturas password=a");
    if (!db.conectar()) {
        term::error("No se pudo conectar a la base de datos.");
        pausa();
        return;
    }

    auto clientesOpt = db.obtenerClientes();
    if (!clientesOpt || clientesOpt->empty()) {
        term::error("No hay clientes en la base de datos.");
        pausa();
        return;
    }

    std::cout << "\n";
    term::subtitulo("SELECCIONE CLIENTE");
    for (size_t i = 0; i < clientesOpt->size(); ++i) {
        const auto& c = (*clientesOpt)[i];
        std::cout << "  " << term::YELLOW << (i + 1) << ")" << term::RESET << " "
                  << c.nombre() << " - " << c.cuit()
                  << " (" << term::CYAN << toString(c.condicionIVA()) << term::RESET << ")\n";
    }
    int opcion = leerEntero("\n  Elija (1-" + std::to_string(clientesOpt->size()) + "): ",
                            1, static_cast<int>(clientesOpt->size()));
    Cliente cliente = (*clientesOpt)[opcion - 1];
    std::cout << "  Cliente: " << term::BOLD << cliente.nombre() << term::RESET << "\n\n";

    int cantProductos = leerEntero("  Cantidad de productos a facturar: ", 1, 100);

    std::vector<Producto> productos;
    std::vector<int> cantidades;

    for (int i = 0; i < cantProductos; ++i) {
        std::cout << "\n  " << term::BOLD << term::CYAN << "--- Producto " << (i + 1) << " ---" << term::RESET << "\n";
        std::string codigo = leerString("  Codigo: ");
        std::string desc   = leerString("  Descripcion: ");
        double precio = leerDouble("  Precio neto: $", 0.0);
        double alicuota = seleccionarAlicuota();
        int cantidad = leerEntero("  Cantidad: ", 1, 99999);

        productos.emplace_back(codigo, desc, precio, alicuota);
        cantidades.push_back(cantidad);
    }

    TipoFactura tipo = tipoFacturaPara(cliente.condicionIVA());

    auto hoy = year_month_day{
        year(2026), month(6), day(7)
    };

    Factura factura(1, tipo, hoy, cliente);
    for (int i = 0; i < cantProductos; ++i) {
        factura.agregarItem(productos[i], cantidades[i]);
    }
    factura.calcular();

    std::cout << "\n\n";
    term::subtitulo("SOLICITANDO CAE A ARCA (MOCK)");

    ArcaService arca(true);

    std::cout << "  Autenticando contra WSAA...\n";
    if (!arca.autenticar("certificado.crt", "clave.key")) {
        term::error("Error de autenticacion.");
        pausa();
        return;
    }
    term::resaltar("Token", arca.token());

    std::cout << "  Solicitando CAE a WSFE...\n";
    auto cae = arca.solicitarCAE(factura);
    if (!cae) {
        term::error("Error al obtener CAE.");
        pausa();
        return;
    }
    factura.asignarCAE(*cae, arca.caeVencimiento());
    term::ok("CAE asignado: " + *cae);
    std::cout << "\n";

    factura.mostrar();

    std::cout << "\n  " << term::BOLD << "Desea enviar respaldo por email? (s/N): " << term::RESET;
    std::string respuesta;
    std::getline(std::cin, respuesta);
    if (respuesta == "s" || respuesta == "S") {
        try {
            std::string detalles =
                "=== DETALLES DE FACTURACION ===\n"
                "Cliente: " + cliente.nombre() + "\n"
                "CUIT: " + cliente.cuit() + "\n"
                "Tipo Factura: " + std::string(toString(factura.tipo())) + "\n"
                "CAE: " + factura.cae() + "\n"
                "Vencimiento CAE: " + factura.caeVencimiento() + "\n"
                "Total: $" + std::to_string(factura.totalFinal()) + "\n";

            enviarEmailCompra(detalles);
        } catch (const std::exception& e) {
            term::error(std::string("Error al enviar email: ") + e.what());
        }
    }

    pausa();
}

int main() {
    std::signal(SIGINT, signalHandler);

    int opcion;
    do {
        mostrarMenu();
        opcion = leerEntero("  Elija una opcion: ", 0, 3);

        if (opcion < 0) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\n  " << term::YELLOW
                      << "Se detecto Ctrl+C. Esta seguro que desea salir? (s/N): "
                      << term::RESET;
            std::string r;
            std::getline(std::cin, r);
            if (r == "s" || r == "S") {
                opcion = 0;
            } else {
                continue;
            }
        }

        switch (opcion) {
            case 1: moduloInformes(); break;
            case 2: nuevaFacturacion(); break;
            case 3: prueba();break;
            case 0:
                std::cout << "\n  " << term::RED << "Saliendo del sistema..." << term::RESET << "\n";
                break;
        }
    } while (opcion != 0);

    return 0;
}
