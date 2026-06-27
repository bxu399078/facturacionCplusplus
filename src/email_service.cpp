#include "email_service.hpp"
#include <curl/curl.h>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <iostream>

using CurlPtr = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;
using CurlSlistPtr = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>;

struct EmailPayload {
    std::string data;
    size_t offset = 0;
};

static size_t readPayload(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* payload = static_cast<EmailPayload*>(userdata);
    if (!payload || payload->offset >= payload->data.size()) return 0;

    size_t chunk = std::min(size * nmemb, payload->data.size() - payload->offset);
    std::memcpy(ptr, payload->data.data() + payload->offset, chunk);
    payload->offset += chunk;
    return chunk;
}

void enviarEmailCompra(const std::string& detallesCompra) {
    CurlPtr curl(curl_easy_init(), &curl_easy_cleanup);
    if (!curl)
        throw std::runtime_error("Error al inicializar CURL para email");

    const std::string fromto = "bux399078@gmail.com";
    const std::string password = "28834316&mg";

    std::string payload = "To: " + fromto + "\r\n"
                          "From: " + fromto + "\r\n"
                          "Subject: Detalles de Facturacion\r\n"
                          "\r\n"
                          + detallesCompra;

    EmailPayload emailPayload{std::move(payload), 0};

    curl_easy_setopt(curl.get(), CURLOPT_URL, "smtps://smtp.gmail.com:465");
    curl_easy_setopt(curl.get(), CURLOPT_USERNAME, fromto.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_MAIL_FROM, fromto.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_READFUNCTION, readPayload);
    curl_easy_setopt(curl.get(), CURLOPT_READDATA, &emailPayload);
    curl_easy_setopt(curl.get(), CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30L);

    CurlSlistPtr recipients(nullptr, &curl_slist_free_all);

    curl_slist* temp = curl_slist_append(recipients.get(), fromto.c_str());
    if (!temp)
        throw std::runtime_error("Error de memoria al crear lista de destinatarios");
    recipients.release();
    recipients.reset(temp);

    curl_easy_setopt(curl.get(), CURLOPT_MAIL_RCPT, recipients.get());

    CURLcode res = curl_easy_perform(curl.get());
    if (res != CURLE_OK)
        throw std::runtime_error(std::string("Error al enviar email: ") + curl_easy_strerror(res));

    std::cout << "  Correo enviado exitosamente a " << fromto << "\n";
}
