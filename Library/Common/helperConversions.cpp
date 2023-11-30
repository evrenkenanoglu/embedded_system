/**
 * @file stringConversion.cpp
 * @brief Source file for stringConversion
 *
 * This file contains definitions for the stringConversion class and related data types and functions.
 */

#include "helperConversions.h"
#include <stdio.h>
#include <string.h>

std::string mac::convertToMac(uint8_t mac[6])
{
    char macAddress[18] = ""; // Initialize the macAddress string to an empty string
    int  i;

    for (i = 0; i < 6; i++)
    {
        uint16_t value   = mac[i];
        char     temp[3] = ""; // Create a temporary string to hold the converted value

        if (i > 0)
        {
            strcat(macAddress, ":"); // Add the ":" separator between the address bytes
        }

        // Convert the value to a string and pad with zeros
        sprintf(temp, "%02x", value);

        // Append the converted value to the macAddress string
        strcat(macAddress, temp);
    }

    // Return the macAddress string
    return std::string(macAddress);
}
