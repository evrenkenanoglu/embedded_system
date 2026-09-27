/** @file       Crc.hpp
 *  @brief      Generic, parameterized Rocksoft-model CRC engine for embedded systems.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/09/2026
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

/**
 * @class Crc
 * @brief Generic, parameter-driven Cyclic Redundancy Check (CRC) calculation engine.
 *
 * Features:
 * - Full parametric Rocksoft model (polynomial, initial value, final XOR, reflection).
 * - Automatic compile-time constexpr lookup tables for standard profiles (0 RAM, Flash-resident).
 * - Fallback bitwise execution for arbitrary runtime custom polynomials (0 stack/RAM overhead).
 * - Streaming stateful instance interface and stateless static batch methods.
 *
 * @tparam UIntType The unsigned integer word type (uint8_t, uint16_t, uint32_t, uint64_t).
 */
template <typename UIntType>
class Crc
{
    static_assert(std::is_unsigned<UIntType>::value, "Crc requires an unsigned integral type.");

public:
    /**
     * @struct Parameters
     * @brief Parametric specification defining any standard or custom CRC algorithm.
     */
    struct Parameters
    {
        UIntType polynomial;    ///< Generator polynomial
        UIntType initialValue;  ///< Initial remainder value
        UIntType finalXor;      ///< Value XORed with final remainder
        bool     reflectInput;  ///< Reflect input bytes (LSB first)
        bool     reflectOutput; ///< Reflect final remainder before XOR mask
    };

    /**
     * @brief Standardized pre-configured parameter profiles.
     */
    struct Profile
    {
        // ---------------------------------------------------------------------
        // 32-bit Profiles (available when UIntType is uint32_t)
        // ---------------------------------------------------------------------

        /** @brief Standard IEEE 802.3 (Ethernet, ZIP, PNG, ISO-HDLC) */
        static constexpr Parameters Ieee802_3()
        {
            return {0x04C11DB7U, 0xFFFFFFFFU, 0xFFFFFFFFU, true, true};
        }

        /** @brief Castagnoli CRC-32C (iSCSI, SCTP, Btrfs, ext4) */
        static constexpr Parameters Castagnoli()
        {
            return {0x1EDC6F41U, 0xFFFFFFFFU, 0xFFFFFFFFU, true, true};
        }

        /** @brief POSIX (cksum) */
        static constexpr Parameters Posix()
        {
            return {0x04C11DB7U, 0x00000000U, 0xFFFFFFFFU, false, false};
        }

        /** @brief MPEG-2 / DVB */
        static constexpr Parameters Mpeg2()
        {
            return {0x04C11DB7U, 0xFFFFFFFFU, 0x00000000U, false, false};
        }

        /** @brief BZIP2 */
        static constexpr Parameters Bzip2()
        {
            return {0x04C11DB7U, 0xFFFFFFFFU, 0xFFFFFFFFU, false, false};
        }

        // ---------------------------------------------------------------------
        // 16-bit Profiles (available when UIntType is uint16_t)
        // ---------------------------------------------------------------------

        /** @brief Modbus / USB (CRC-16/IBM) */
        static constexpr Parameters Modbus()
        {
            return {0x8005U, 0xFFFFU, 0x0000U, true, true};
        }

        /** @brief CCITT-FALSE */
        static constexpr Parameters CcittFalse()
        {
            return {0x1021U, 0xFFFFU, 0x0000U, false, false};
        }
    };

private:
    /**
     * @brief Compile-time lookup table generator stored in Flash (.rodata). Consumes 0 bytes of RAM.
     */
    template <UIntType Polynomial>
    struct StaticTable
    {
        static constexpr size_t   width  = sizeof(UIntType) * 8;
        static constexpr UIntType topBit = static_cast<UIntType>(static_cast<UIntType>(1) << (width - 1));

        UIntType values[256];

        constexpr StaticTable()
            : values{}
        {
            for (size_t i = 0; i < 256; ++i)
            {
                UIntType remainder = static_cast<UIntType>(static_cast<UIntType>(i) << (width - 8));
                for (uint8_t bit = 0; bit < 8; ++bit)
                {
                    if ((remainder & topBit) != 0)
                    {
                        remainder = static_cast<UIntType>((remainder << 1) ^ Polynomial);
                    }
                    else
                    {
                        remainder = static_cast<UIntType>(remainder << 1);
                    }
                }
                values[i] = remainder;
            }
        }
    };

public:
    /*========================================================================*/
    /* STATIC HELPER ROUTINES                                                 */
    /*========================================================================*/

    /**
     * @brief Reflects the bit order of an 8-bit byte.
     */
    static constexpr uint8_t reflectByte(uint8_t byte)
    {
        byte = static_cast<uint8_t>(((byte & 0xF0) >> 4) | ((byte & 0x0F) << 4));
        byte = static_cast<uint8_t>(((byte & 0xCC) >> 2) | ((byte & 0x33) << 2));
        byte = static_cast<uint8_t>(((byte & 0xAA) >> 1) | ((byte & 0x55) << 1));
        return byte;
    }

    /**
     * @brief Reflects the bit order of an arbitrary unsigned word.
     */
    static constexpr UIntType reflectWord(UIntType value)
    {
        constexpr size_t totalBits  = sizeof(UIntType) * 8;
        UIntType         reflection = 0;
        for (size_t i = 0; i < totalBits; ++i)
        {
            if ((value & (static_cast<UIntType>(1) << i)) != 0)
            {
                reflection |= (static_cast<UIntType>(1) << (totalBits - 1 - i));
            }
        }
        return reflection;
    }

    /*========================================================================*/
    /* HIGH-SPEED TABLE EXECUTION (0 RAM / FLASH .rodata)                     */
    /*========================================================================*/

    /**
     * @brief Executes table-driven byte-by-byte CRC calculation using a compile-time Flash table.
     */
    template <UIntType Polynomial>
    static UIntType updateFast(UIntType currentRemainder, const uint8_t* data, size_t length, const Parameters& params)
    {
        if (data == nullptr || length == 0)
        {
            return currentRemainder;
        }

        constexpr size_t                         width = sizeof(UIntType) * 8;
        static constexpr StaticTable<Polynomial> table{};

        UIntType remainder = currentRemainder;

        for (size_t i = 0; i < length; ++i)
        {
            const uint8_t curByte = params.reflectInput ? reflectByte(data[i]) : data[i];
            const uint8_t index   = static_cast<uint8_t>((remainder >> (width - 8)) ^ curByte);

            if constexpr (width == 8)
            {
                remainder = table.values[index];
            }
            else
            {
                remainder = static_cast<UIntType>((remainder << 8) ^ table.values[index]);
            }
        }

        return remainder;
    }

    /*========================================================================*/
    /* BITWISE EXECUTION (ZERO STACK / ZERO RAM FALLBACK)                     */
    /*========================================================================*/

    /**
     * @brief Bitwise calculation fallback for arbitrary runtime polynomials.
     */
    static UIntType updateBitwise(UIntType currentRemainder, const uint8_t* data, size_t length, const Parameters& params)
    {
        if (data == nullptr || length == 0)
        {
            return currentRemainder;
        }

        constexpr size_t   width  = sizeof(UIntType) * 8;
        constexpr UIntType topBit = static_cast<UIntType>(static_cast<UIntType>(1) << (width - 1));

        UIntType remainder = currentRemainder;

        for (size_t i = 0; i < length; ++i)
        {
            const uint8_t curByte = params.reflectInput ? reflectByte(data[i]) : data[i];
            remainder ^= static_cast<UIntType>(static_cast<UIntType>(curByte) << (width - 8));

            for (uint8_t bit = 0; bit < 8; ++bit)
            {
                if ((remainder & topBit) != 0)
                {
                    remainder = static_cast<UIntType>((remainder << 1) ^ params.polynomial);
                }
                else
                {
                    remainder = static_cast<UIntType>(remainder << 1);
                }
            }
        }

        return remainder;
    }

    /*========================================================================*/
    /* GENERAL / DISPATCHING EXECUTION APIS                                   */
    /*========================================================================*/

    /**
     * @brief Updates an ongoing CRC calculation.
     *        Automatically selects compile-time Flash tables for standard profiles,
     *        or bitwise execution for arbitrary custom polynomials.
     *
     * @param[in] currentRemainder Current intermediate remainder.
     * @param[in] data             Buffer containing data bytes.
     * @param[in] length           Number of bytes to process.
     * @param[in] params           CRC algorithm configuration.
     * @return UIntType            Updated intermediate remainder.
     */
    static UIntType update(UIntType currentRemainder, const uint8_t* data, size_t length, const Parameters& params)
    {
        if constexpr (sizeof(UIntType) == sizeof(uint32_t))
        {
            if (params.polynomial == Profile::Ieee802_3().polynomial)
            {
                return updateFast<Profile::Ieee802_3().polynomial>(currentRemainder, data, length, params);
            }
            if (params.polynomial == Profile::Castagnoli().polynomial)
            {
                return updateFast<Profile::Castagnoli().polynomial>(currentRemainder, data, length, params);
            }
        }
        else if constexpr (sizeof(UIntType) == sizeof(uint16_t))
        {
            if (params.polynomial == Profile::Modbus().polynomial)
            {
                return updateFast<Profile::Modbus().polynomial>(currentRemainder, data, length, params);
            }
            if (params.polynomial == Profile::CcittFalse().polynomial)
            {
                return updateFast<Profile::CcittFalse().polynomial>(currentRemainder, data, length, params);
            }
        }

        return updateBitwise(currentRemainder, data, length, params);
    }

    /**
     * @brief Finalizes an ongoing CRC calculation.
     *
     * @param[in] currentRemainder Current intermediate remainder.
     * @param[in] params           CRC algorithm configuration.
     * @return UIntType            Finalized CRC checksum.
     */
    static UIntType finalize(UIntType currentRemainder, const Parameters& params)
    {
        UIntType result = currentRemainder;
        if (params.reflectOutput)
        {
            result = reflectWord(result);
        }
        return static_cast<UIntType>(result ^ params.finalXor);
    }

    /**
     * @brief Computes a complete CRC checksum across a buffer in a single shot.
     *
     * @param[in] data   Buffer containing data bytes.
     * @param[in] length Number of bytes to process.
     * @param[in] params CRC algorithm configuration.
     * @return UIntType  Computed CRC checksum.
     */
    static UIntType calculate(const uint8_t* data, size_t length, const Parameters& params = Profile::Ieee802_3())
    {
        const UIntType remainder = update(params.initialValue, data, length, params);
        return finalize(remainder, params);
    }

public:
    /*========================================================================*/
    /* STATEFUL INSTANCE APIS                                                 */
    /*========================================================================*/

    /**
     * @brief Constructs a stateful CRC engine configured with specific parameters.
     */
    explicit Crc(const Parameters& params = Profile::Ieee802_3())
        : _params(params)
        , _remainder(params.initialValue)
    {
    }

    ~Crc() = default;

    /**
     * @brief Resets the internal accumulator back to the initial value.
     */
    void reset()
    {
        _remainder = _params.initialValue;
    }

    /**
     * @brief Ingests an incoming data buffer into the active CRC accumulator.
     */
    void update(const uint8_t* data, size_t length)
    {
        _remainder = update(_remainder, data, length, _params);
    }

    /**
     * @brief Ingests a single byte into the active CRC accumulator.
     */
    void update(uint8_t byte)
    {
        update(&byte, 1);
    }

    /**
     * @brief Obtains the current finalized CRC without resetting the state.
     */
    UIntType get() const
    {
        return finalize(_remainder, _params);
    }

    /**
     * @brief Obtains the finalized CRC and resets the accumulator for the next payload.
     */
    UIntType finalize()
    {
        const UIntType result = get();
        reset();
        return result;
    }

private:
    Parameters _params;
    UIntType   _remainder;
};

/*============================================================================*/
/* CONVENIENCE TYPE DEFINITIONS                                               */
/*============================================================================*/
using Crc32 = Crc<uint32_t>;
using Crc16 = Crc<uint16_t>;
using Crc8  = Crc<uint8_t>;
