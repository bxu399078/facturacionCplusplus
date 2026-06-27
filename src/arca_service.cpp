#include "arca_service.hpp"
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>
#include <stdexcept>

// =========================================================================
//  Mock implementation (siempre disponible)
// =========================================================================

bool ArcaService::mockAutenticar() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(500ms);

    auto now = std::time(nullptr);
    token_ = "MOCK_TOKEN_" + std::to_string(now);
    sign_  = "MOCK_SIGN_"  + std::to_string(now);
    return true;
}

std::optional<std::string> ArcaService::mockSolicitarCAE(Factura& factura) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(800ms);

    cae_ = "12345678901234";

    auto venc = std::chrono::system_clock::now() + std::chrono::days(15);
    auto vencTt = std::chrono::system_clock::to_time_t(venc);
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&vencTt), "%Y-%m-%d");
    caeVencimiento_ = ss.str();

    return cae_;
}

// =========================================================================
//  Implementacion real (requiere -DARCA_REAL y libcurl + openssl)
// =========================================================================

#if defined(ARCA_REAL)

#include <curl/curl.h>
#include <openssl/cms.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/evp.h>

// -- Custom deleters RAII --------------------------------------------------

using BioPtr       = std::unique_ptr<BIO,           decltype(&BIO_free)>;
using BioChainPtr  = std::unique_ptr<BIO,           decltype(&BIO_free_all)>;
using X509Ptr      = std::unique_ptr<X509,          decltype(&X509_free)>;
using EvpPkeyPtr   = std::unique_ptr<EVP_PKEY,      decltype(&EVP_PKEY_free)>;
using CmsPtr       = std::unique_ptr<CMS_ContentInfo, decltype(&CMS_ContentInfo_free)>;
using OpenSslMem   = std::unique_ptr<unsigned char, decltype(&OPENSSL_free)>;
using CurlPtr      = std::unique_ptr<CURL,          decltype(&curl_easy_cleanup)>;
using CurlSlistPtr = std::unique_ptr<curl_slist,    decltype(&curl_slist_free_all)>;

// -- utilerias -----------------------------------------------------------

static std::string base64Encode(const unsigned char* data, int len) {
    BioPtr mem(BIO_new(BIO_s_mem()), &BIO_free);
    if (!mem) throw std::runtime_error("Error al crear BIO memory");
    BioPtr b64(BIO_new(BIO_f_base64()), &BIO_free);
    if (!b64) throw std::runtime_error("Error al crear BIO base64");

    // BIO_push transfiere propiedad: b64 es la cabecera de la cadena
    BioChainPtr chain(BIO_push(b64.get(), mem.get()), &BIO_free_all);
    b64.release();
    mem.release();

    BIO_set_flags(chain.get(), BIO_FLAGS_BASE64_NO_NL);
    if (BIO_write(chain.get(), data, len) <= 0)
        throw std::runtime_error("Error al escribir en BIO");
    if (BIO_flush(chain.get()) <= 0)
        throw std::runtime_error("Error al flushear BIO");

    const char* encoded = nullptr;
    long encodedLen = BIO_get_mem_data(chain.get(), &encoded);
    return {encoded, static_cast<size_t>(encodedLen)};
}

static std::string firmarCMS(
    const std::string& tra,
    const std::string& certPath,
    const std::string& keyPath
) {
    BioPtr certBio(BIO_new_file(certPath.c_str(), "r"), &BIO_free);
    if (!certBio) throw std::runtime_error("No se pudo abrir el certificado");
    X509Ptr cert(PEM_read_bio_X509(certBio.get(), nullptr, nullptr, nullptr), &X509_free);
    if (!cert) throw std::runtime_error("Error al leer el certificado");

    BioPtr keyBio(BIO_new_file(keyPath.c_str(), "r"), &BIO_free);
    if (!keyBio) throw std::runtime_error("No se pudo abrir la clave privada");
    EvpPkeyPtr key(PEM_read_bio_PrivateKey(keyBio.get(), nullptr, nullptr, nullptr), &EVP_PKEY_free);
    if (!key) throw std::runtime_error("Error al leer la clave privada");

    BioPtr dataBio(BIO_new_mem_buf(tra.data(), static_cast<int>(tra.size())), &BIO_free);
    if (!dataBio) throw std::runtime_error("Error al crear BIO de datos");

    CmsPtr cms(CMS_sign(cert.get(), key.get(), nullptr, dataBio.get(),
                         CMS_DETACHED | CMS_BINARY | CMS_NOCERTS),
               &CMS_ContentInfo_free);
    if (!cms) throw std::runtime_error("Error al firmar CMS");

    unsigned char* derRaw = nullptr;
    int derLen = i2d_CMS_ContentInfo(cms.get(), &derRaw);
    if (derLen < 0) throw std::runtime_error("Error al codificar CMS a DER");
    OpenSslMem der(derRaw, &OPENSSL_free);

    return base64Encode(der.get(), derLen);
}

static std::string generarTRA() {
    using namespace std::chrono;

    auto now = system_clock::now();
    auto genTime = system_clock::to_time_t(now);
    auto expTime = system_clock::to_time_t(now + hours(12));

    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
       << "<loginTicketRequest version=\"1.0\">\n"
       << "  <header>\n"
       << "    <uniqueId>"
       << duration_cast<milliseconds>(now.time_since_epoch()).count()
       << "</uniqueId>\n"
       << "    <generationTime>"
       << std::put_time(std::gmtime(&genTime), "%Y-%m-%dT%H:%M:%S")
       << "</generationTime>\n"
       << "    <expirationTime>"
       << std::put_time(std::gmtime(&expTime), "%Y-%m-%dT%H:%M:%S")
       << "</expirationTime>\n"
       << "  </header>\n"
       << "  <service>wsfe</service>\n"
       << "</loginTicketRequest>\n";
    return ss.str();
}

static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* user) {
    auto& s = *static_cast<std::string*>(user);
    s.append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

static std::string soapRequest(const std::string& url, const std::string& soapXml) {
    CurlPtr curl(curl_easy_init(), &curl_easy_cleanup);
    if (!curl) throw std::runtime_error("Error al inicializar libcurl");

    std::string response;

    curl_slist* headersRaw = nullptr;
    headersRaw = curl_slist_append(headersRaw, "Content-Type: text/xml;charset=UTF-8");
    headersRaw = curl_slist_append(headersRaw, "SOAPAction: \"\"");
    CurlSlistPtr headers(headersRaw, &curl_slist_free_all);

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, soapXml.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl.get());

    long httpCode = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &httpCode);

    if (res != CURLE_OK)
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    if (httpCode != 200)
        throw std::runtime_error("HTTP " + std::to_string(httpCode));

    return response;
}

static std::string extraerTag(const std::string& xml, const std::string& tag) {
    auto open  = xml.find('<'  + tag + '>');
    if (open == std::string::npos) return {};
    open += tag.size() + 2;
    auto close = xml.find("</" + tag + '>', open);
    if (close == std::string::npos) return {};
    return xml.substr(open, close - open);
}

// -- WSAA ------------------------------------------------------------------

bool ArcaService::realAutenticar(
    const std::string& certPath,
    const std::string& keyPath
) {
    std::string tra  = generarTRA();
    std::string cms  = firmarCMS(tra, certPath, keyPath);

    std::string soap =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<SOAP-ENV:Envelope "
        "xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "xmlns:nsd=\"http://wsaa.view.sua.dvadac.navegacionar.gov.ar/\">"
        "<SOAP-ENV:Body>"
        "<nsd:loginCms>"
        "<nsd:in0>" + cms + "</nsd:in0>"
        "</nsd:loginCms>"
        "</SOAP-ENV:Body>"
        "</SOAP-ENV:Envelope>";

    std::string url = "https://wsaahomo.afip.gov.ar/ws/services/LoginCms";
    std::string response = soapRequest(url, soap);

    token_ = extraerTag(response, "token");
    sign_  = extraerTag(response, "sign");

    return !token_.empty() && !sign_.empty();
}

// -- WSFE ------------------------------------------------------------------

static std::string tipoAFIP(TipoFactura t) {
    switch (t) {
        case TipoFactura::A: return "1";
        case TipoFactura::B: return "6";
        case TipoFactura::C: return "11";
    }
    return "1";
}

static std::string ivaId(double alicuota) {
    if (alicuota == 0.21)  return "5";
    if (alicuota == 0.105) return "4";
    if (alicuota == 0.0)   return "3";
    return "3";
}

std::optional<std::string> ArcaService::realSolicitarCAE(Factura& factura) {
    std::string cuitLimpio;
    for (char c : factura.cliente().cuit()) {
        if (std::isdigit(static_cast<unsigned char>(c))) cuitLimpio += c;
    }

    auto f = factura.fecha();
    std::ostringstream fechaFe;
    fechaFe << static_cast<int>(f.year())
            << std::setw(2) << std::setfill('0')
            << static_cast<unsigned>(f.month())
            << std::setw(2) << std::setfill('0')
            << static_cast<unsigned>(f.day());
    std::string fechaStr = fechaFe.str();

    std::map<double, double> ivaPorTasa;
    std::map<double, double> netoPorTasa;
    for (const auto& item : factura.items()) {
        double netoItem = item.producto.precioNeto() * item.cantidad;
        double ivaItem  = netoItem * item.producto.alicuotaIVA();
        double tasa     = item.producto.alicuotaIVA();
        netoPorTasa[tasa] += netoItem;
        ivaPorTasa[tasa]  += ivaItem;
    }

    std::string alicIvasXml;
    for (const auto& [tasa, neto] : netoPorTasa) {
        double iva = ivaPorTasa.at(tasa);
        alicIvasXml +=
            "<AlicIva>"
            "<Id>" + ivaId(tasa) + "</Id>"
            "<BaseImp>" + std::to_string(neto) + "</BaseImp>"
            "<Importe>" + std::to_string(iva) + "</Importe>"
            "</AlicIva>";
    }

    std::string soap =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<soap:Envelope "
        "xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
        "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
        "<soap:Body>"
        "<FECAESolicitar xmlns=\"http://ar.gov.afip.dif.FEV1/\">"
        "<Auth>"
        "<Token>" + token_ + "</Token>"
        "<Sign>"  + sign_  + "</Sign>"
        "<Cuit>"  + cuitLimpio + "</Cuit>"
        "</Auth>"
        "<FeCAEReq>"
        "<FeCabReq>"
        "<CantReg>1</CantReg>"
        "<PtoVta>1</PtoVta>"
        "<CbteTipo>" + tipoAFIP(factura.tipo()) + "</CbteTipo>"
        "</FeCabReq>"
        "<FeDetReq>"
        "<FECAEDetRequest>"
        "<Concepto>1</Concepto>"
        "<DocTipo>80</DocTipo>"
        "<DocNro>" + cuitLimpio + "</DocNro>"
        "<CbteDesde>" + std::to_string(factura.numero()) + "</CbteDesde>"
        "<CbteHasta>" + std::to_string(factura.numero()) + "</CbteHasta>"
        "<CbteFch>" + fechaStr + "</CbteFch>"
        "<ImpTotal>"  + std::to_string(factura.totalFinal()) + "</ImpTotal>"
        "<ImpTotConc>0</ImpTotConc>"
        "<ImpNeto>"   + std::to_string(factura.subtotalNeto()) + "</ImpNeto>"
        "<ImpOpEx>0</ImpOpEx>"
        "<ImpIVA>"    + std::to_string(factura.totalIVA()) + "</ImpIVA>"
        "<ImpTrib>0</ImpTrib>"
        "<MonId>PES</MonId>"
        "<MonCotiz>1</MonCotiz>"
        "<Iva>" + alicIvasXml + "</Iva>"
        "</FECAEDetRequest>"
        "</FeDetReq>"
        "</FeCAEReq>"
        "</FECAESolicitar>"
        "</soap:Body>"
        "</soap:Envelope>";

    std::string url = "https://wswhomo.afip.gov.ar/wsfev1/service.asmx";
    std::string response = soapRequest(url, soap);

    cae_           = extraerTag(response, "CAE");
    caeVencimiento_ = extraerTag(response, "CAEFchVto");

    if (cae_.empty()) return std::nullopt;
    return cae_;
}

#else // !ARCA_REAL

bool ArcaService::realAutenticar(
    [[maybe_unused]] const std::string& certPath,
    [[maybe_unused]] const std::string& keyPath
) {
    throw std::runtime_error(
        "Modo real no disponible. Recompilar con -DARCA_REAL "
        "y enlazar libcurl + openssl."
    );
}

std::optional<std::string> ArcaService::realSolicitarCAE(
    [[maybe_unused]] Factura& factura
) {
    throw std::runtime_error(
        "Modo real no disponible. Recompilar con -DARCA_REAL "
        "y enlazar libcurl + openssl."
    );
}

#endif

// =========================================================================
//  Interfaz publica
// =========================================================================

ArcaService::ArcaService(bool mock) : mock_(mock) {}

bool ArcaService::autenticar(const std::string& certPath, const std::string& keyPath) {
    if (mock_) return mockAutenticar();
    return realAutenticar(certPath, keyPath);
}

std::optional<std::string> ArcaService::solicitarCAE(Factura& factura) {
    if (mock_) return mockSolicitarCAE(factura);
    return realSolicitarCAE(factura);
}
