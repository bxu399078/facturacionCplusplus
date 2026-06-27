#include "reporte_pdf.hpp"
#include "terminal.hpp"
#include "database.hpp"
#include <hpdf.h>
#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

using PdfPtr = std::unique_ptr<std::remove_pointer_t<HPDF_Doc>, decltype(&HPDF_Free)>;

static void errorHandler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void*) {
    std::cerr << term::RED << "\n  Error HPDF: codigo=" << static_cast<int>(error_no)
              << " detalle=" << static_cast<int>(detail_no) << term::RESET << "\n";
}

void generarReporteClientesPDF() {
    term::titulo("INFORMES - REPORTE DE CLIENTES");

    DatabaseManager db("host=localhost dbname=facturacioniva user=facturas password=a");
    if (!db.conectar()) {
        term::error("No se pudo conectar a la base de datos.");
        return;
    }

    term::ok("Conectado a PostgreSQL.");

    auto clientesOpt = db.obtenerClientes();
    if (!clientesOpt || clientesOpt->empty()) {
        term::error("No hay clientes en la base de datos.");
        return;
    }

    term::ok(std::to_string(clientesOpt->size()) + " clientes leidos.");
    std::cout << "  Generando PDF...\n";

    HPDF_Doc raw = HPDF_New(errorHandler, nullptr);
    if (!raw) {
        term::error("Error al crear documento PDF.");
        return;
    }
    PdfPtr pdf(raw, &HPDF_Free);

    HPDF_Page page = HPDF_AddPage(pdf.get());
    if (!page) {
        term::error("Error al crear pagina PDF.");
        return;
    }

    HPDF_Page_SetWidth(page, 595);
    HPDF_Page_SetHeight(page, 842);

    HPDF_Font font = HPDF_GetFont(pdf.get(), "Helvetica", nullptr);

    HPDF_Page_SetFontAndSize(page, font, 20);
    HPDF_Page_BeginText(page);
    HPDF_Page_TextOut(page, 40, 780, "Reporte de Clientes");
    HPDF_Page_EndText(page);

    HPDF_Page_SetFontAndSize(page, font, 11);
    HPDF_Page_BeginText(page);

    float y = 740;
    const float x_id = 40;
    const float x_nombre = 100;
    const float x_cuit = 380;

    //HPDF_Page_TextOut(page, x_id, y, "ID");
    HPDF_Page_TextOut(page, x_nombre, y, "Nombre");
    HPDF_Page_TextOut(page, x_cuit, y, "CUIT");
    HPDF_Page_EndText(page);

    y -= 6;
    HPDF_Page_SetLineWidth(page, 0.5);
    HPDF_Page_MoveTo(page, x_id, y);
    HPDF_Page_LineTo(page, 500, y);
    HPDF_Page_Stroke(page);
    y -= 14;

    HPDF_Page_SetFontAndSize(page, font, 10);

    int count = 0;
    for (const auto& c : *clientesOpt) {
        if (y < 60) {
            page = HPDF_AddPage(pdf.get());
            if (!page) break;
            HPDF_Page_SetWidth(page, 595);
            HPDF_Page_SetHeight(page, 842);
            HPDF_Page_SetFontAndSize(page, font, 10);
            y = 780;
        }

        HPDF_Page_BeginText(page);
 //       HPDF_Page_TextOut(page, x_id, y, std::to_string(++count).c_str());
        HPDF_Page_TextOut(page, x_nombre, y, c.nombre().c_str());
        HPDF_Page_TextOut(page, x_cuit, y, c.cuit().c_str());
        HPDF_Page_EndText(page);
        y -= 18;
    }

    const char* filename = "listado_clientes.pdf";
    if (HPDF_SaveToFile(pdf.get(), filename) != HPDF_OK) {
        term::error("Error al guardar el PDF.");
        return;
    }

    term::ok(std::string("PDF generado: ") + filename);
}

void generarReporteProductosPDF() {
    term::titulo("INFORMES - LISTADO DE PRODUCTOS");

    DatabaseManager db("host=localhost dbname=facturacioniva user=facturas password=a");
    if (!db.conectar()) {
        term::error("No se pudo conectar a la base de datos.");
        return;
    }

    term::ok("Conectado a PostgreSQL.");

    auto prodOpt = db.obtenerProductos();
    if (!prodOpt || prodOpt->empty()) {
        term::error("No hay productos en la base de datos.");
        return;
    }

    term::ok(std::to_string(prodOpt->size()) + " productos leidos.");
    std::cout << "  Generando PDF...\n";

    HPDF_Doc raw = HPDF_New(errorHandler, nullptr);
    if (!raw) {
        term::error("Error al crear documento PDF.");
        return;
    }
    PdfPtr pdf(raw, &HPDF_Free);

    HPDF_Page page = HPDF_AddPage(pdf.get());
    if (!page) {
        term::error("Error al crear pagina PDF.");
        return;
    }

    HPDF_Page_SetWidth(page, 595);
    HPDF_Page_SetHeight(page, 842);

    HPDF_Font font = HPDF_GetFont(pdf.get(), "Helvetica", nullptr);

    HPDF_Page_SetFontAndSize(page, font, 20);
    HPDF_Page_BeginText(page);
    HPDF_Page_TextOut(page, 40, 780, "Listado de Productos");
    HPDF_Page_EndText(page);

    HPDF_Page_SetFontAndSize(page, font, 11);
    HPDF_Page_BeginText(page);

    float y = 740;
    const float x_id = 40;
    const float x_nombre = 90;
    const float x_cant = 370;
    const float x_precio = 440;

    HPDF_Page_TextOut(page, x_id, y, "ID");
    HPDF_Page_TextOut(page, x_nombre, y, "Nombre");
    HPDF_Page_TextOut(page, x_cant, y, "Cant.");
    HPDF_Page_TextOut(page, x_precio, y, "Precio");
    HPDF_Page_EndText(page);

    y -= 6;
    HPDF_Page_SetLineWidth(page, 0.5);
    HPDF_Page_MoveTo(page, x_id, y);
    HPDF_Page_LineTo(page, 530, y);
    HPDF_Page_Stroke(page);
    y -= 14;

    HPDF_Page_SetFontAndSize(page, font, 10);

    for (const auto& p : *prodOpt) {
        if (y < 60) {
            page = HPDF_AddPage(pdf.get());
            if (!page) break;
            HPDF_Page_SetWidth(page, 595);
            HPDF_Page_SetHeight(page, 842);
            HPDF_Page_SetFontAndSize(page, font, 10);
            y = 780;
        }

        HPDF_Page_BeginText(page);
        HPDF_Page_TextOut(page, x_id, y, std::to_string(p.id).c_str());
        HPDF_Page_TextOut(page, x_nombre, y, p.nombre.c_str());
        HPDF_Page_TextOut(page, x_cant, y, std::to_string(p.cantidad).c_str());

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << "$" << p.precio;
        HPDF_Page_TextOut(page, x_precio, y, ss.str().c_str());
        HPDF_Page_EndText(page);
        y -= 18;
    }

    const char* filename = "listado_productos.pdf";
    if (HPDF_SaveToFile(pdf.get(), filename) != HPDF_OK) {
        term::error("Error al guardar el PDF.");
        return;
    }

    term::ok(std::string("PDF generado: ") + filename);
}
