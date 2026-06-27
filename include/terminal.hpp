#pragma once

#include <iostream>
#include <string>
#include <string_view>

namespace term {

inline constexpr auto RESET   = "\033[0m";
inline constexpr auto BOLD    = "\033[1m";
inline constexpr auto DIM     = "\033[2m";
inline constexpr auto GREEN   = "\033[32m";
inline constexpr auto YELLOW  = "\033[33m";
inline constexpr auto BLUE    = "\033[34m";
inline constexpr auto MAGENTA = "\033[35m";
inline constexpr auto CYAN    = "\033[36m";
inline constexpr auto RED     = "\033[31m";
inline constexpr auto WHITE   = "\033[37m";
inline constexpr auto ColorFelicidad   = "\033[40m";

inline void titulo(std::string_view t) {
    std::cout << BOLD << BLUE << "=== " << RESET
              << BOLD << WHITE << t << RESET
              << BOLD << BLUE << " ===" << RESET << "\n";
}

inline void subtitulo(std::string_view t) {
    std::cout << BOLD << CYAN << "--- " << t << " ---" << RESET << "\n";
}

inline void ok(std::string_view msg) {
    std::cout << GREEN << "  OK: " << msg << RESET << "\n";
}

 
inline void error(std::string_view msg) {
    std::cout << RED << "  ERROR: " << msg << RESET << "\n";
}

inline void resaltar(std::string_view label, std::string_view valor) {
    std::cout << "  " << BOLD << label << RESET << ": " << YELLOW << valor << RESET << "\n";
}

inline void ProbarColor(std::string_view msg){
    std::cout<< BOLD<<ColorFelicidad<<"----"<<msg<<"---"<<RESET<<"\n";
}

} // namespace term
