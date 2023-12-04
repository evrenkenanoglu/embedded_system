/**
 * @file stringConversion.hpp
 * @brief Header file for stringConversion
 *
 * This file contains declarations for the stringConversion class and related data types and functions.
 */
#ifndef HELPERCONVERSIONS_HPP
#define HELPERCONVERSIONS_HPP

#include <string>

namespace mac
{
std::string convertToMac(uint8_t mac[6]);
}

#endif /* HELPERCONVERSIONS_HPP */
