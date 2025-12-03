/**
 * @file cpx_mcp23x17.hpp
 * @brief Header file for cpx_mcp23x17
 *
 * This file contains declarations for the cpx_mcp23x17 class and related data types and functions.
 */

#ifndef CPX_MCP23X17_HPP
#define CPX_MCP23X17_HPP

#include "HAL\IHAL\IHal_Io_Gpio.h"
#include <map>

/////////////////////////////////////////////////////////////////////////////////////////////////
// MACRO DEFINITIONS
/////////////////////////////////////////////////////////////////////////////////////////////////

#define MCP23017_ADDRESS_PINA0_VALUE 0 // MCP23017 I2C Address Value for PINA0
#define MCP23017_ADDRESS_PINA1_VALUE 0 // MCP23017 I2C Address Value for PINA1
#define MCP23017_ADDRESS_PINA2_VALUE 0 // MCP23017 I2C Address Value for PINA2

#define MCP23017_I2C_ADDRESS_DEFAULT 0x20 // Default I2C address for MCP23017
#define MCP23017_I2C_ADDRESS     \
    MCP23017_I2C_ADDRESS_DEFAULT \
    | (MCP23017_ADDRESS_PINA0_VALUE << 0) | (MCP23017_ADDRESS_PINA1_VALUE << 1) | (MCP23017_ADDRESS_PINA2_VALUE << 2)

/**
 * @brief Macro for Register access functions for MCP23X17.
 *
 * These functions allow you to set or get the value of a register or a specific pin in a register.
 *
 * @param port The port to access (A or B).
 * @param pinNo The pin number to access (0-7). Used for No-suffix functions.
 * @param value The value to set or get. For set functions, this is the value to write. For get functions, this is a reference to store the read value.
 *        - For pinNo functions: bool (true/false)
 *        - For port functions: uint8_t (bitmask for all pins in the port)
 * @return sys_error_t The error code indicating the success or failure of the operation.
 *
 * Functions declared:
 *   - set<RegisterName>No(PORT port, uint8_t pinNo, bool value)
 *   - get<RegisterName>No(PORT port, uint8_t pinNo, bool& value)
 *   - set<RegisterName>(PORT port, uint8_t value)
 *   - get<RegisterName>(PORT port, uint8_t& value)
 */
#define CPX_MCP23X17_REG_DECL(NAME)                                   \
    sys_error_t set##NAME##No(PORT port, uint8_t pinNo, bool value);  \
    sys_error_t get##NAME##No(PORT port, uint8_t pinNo, bool& value); \
    sys_error_t set##NAME(PORT port, uint8_t value);                  \
    sys_error_t get##NAME(PORT port, uint8_t& value);

/**
 * @brief Macro for Read-Only Register access functions for MCP23X17.
 *
 * These functions allow you to get the value of a read-only register or a specific pin in a read-only register.
 *
 * @param port The port to access (A or B).
 * @param pinNo The pin number to access (0-7). Used for No-suffix functions.
 * @param value The value to get. For get functions, this is a reference to store the read value.
 *       - For pinNo functions: bool (true/false)
 *      - For port functions: uint8_t (bitmask for all pins in the port)
 * @return sys_error_t The error code indicating the success or failure of the operation.
 * * Functions declared:
 *  - get<RegisterName>No(PORT port, uint8_t pinNo, bool& value)
 *  - get<RegisterName>(PORT port, uint8_t& value)
 *
 *
 */
#define CPX_MCP23X17_REG_DECL_READONLY(NAME)                          \
    sys_error_t get##NAME##No(PORT port, uint8_t pinNo, bool& value); \
    sys_error_t get##NAME(PORT port, uint8_t& value);

// IOCON Register Definitions
#define SEQENTIAL_OPERATION_ENABLED    0x00 // Sequential operation enabled
#define SEQENTIAL_OPERATION_DISABLED   0x01 // Sequential operation disabled

#define MIRROR_ENABLED                 0x01 // Mirror enabled : The INT pins are internally connected
#define MIRROR_DISABLED                0x00 // Mirror disabled : The INT pins are not connected. INTA is associated with PORTA and INTB is associated with PORTB

#define SLEW_RATE_DISABLED             0x01 // Slew rate disabled
#define SLEW_RATE_ENABLED              0x00 // Slew rate enabled

#define HAEN_ENABLED                   0x01 // Hardware address enabled
#define HAEN_DISABLED                  0x00 // Hardware address disabled

#define ODR_ENABLED                    0x01 // Open-drain output (overrides the INTPOL bit.)
#define ODR_DISABLED                   0x00 // Active driver output (INTPOL bit sets the polarity.)

#define INTPOL_ACTIVE_HIGH             0x01 // Active high interrupt
#define INTPOL_ACTIVE_LOW              0x00 // Active low interrupt

// Direction Register Definitions
#define INPUT_MODE                     0x01 // Input mode
#define OUTPUT_MODE                    0x00 // Output mode

// Polarity Register Definitions
#define POLARITY_INVERTED              0x01 // Inverted polarity : GPIO register bit reflects the opposite logic state of the input pin
#define POLARITY_NORMAL                0x00 // Normal polarity : GPIO register bit reflects the same logic state of the input pin.

// GPIO PORT Register Definitions
#define LOGIC_HIGH                     0x01 // GPIO pin set to high
#define LOGIC_LOW                      0x00 // GPIO pin set to low

// Interrupt-on-Change Enable Register Definitions
#define INTERRUPT_ON_CHANGE_ENABLED    0x01 // Interrupt-on-change enabled
#define INTERRUPT_ON_CHANGE_DISABLED   0x00 // Interrupt-on-change disabled

// Interrupt-on-Change Control Register Definitions
#define COMPARE_AGAINST_DEFVAL_STATE   0x01 // Pin value is compared against the associated bit in the DEFVAL register.
#define COMPARE_AGAINST_PREVIOUS_STATE 0x00 // Pin value is compared against the previous pin value.

// Pull-Up Resistor Register Definitions
#define PULL_UP_RESISTOR_ENABLED       0x01 // Pull-up resistor enabled
#define PULL_UP_RESISTOR_DISABLED      0x00 // Pull-up resistor disabled

// Interrupt Flag Register Definitions
#define PIN_CAUSED_INTERRUPT           0x01 // Pin caused interrupt
#define INTERRUPT_NOT_PENDING          0x00 // Interrupt not pending

/////////////////////////////////////////////////////////////////////////////////////////////////
// CLASS DECLARATION
/////////////////////////////////////////////////////////////////////////////////////////////////

class cpx_mcp23x17 : public IHAL_CPX
{
public:
    enum class PORT : uint8_t
    {
        A = 0, // Port A
        B = 1, // Port B
        MAX,
    };

    /**
     * @brief Enumeration for the bank mode of the MCP23X17.
     *
     * The  16-bit  I/O  port  functionally  consists  of  two  8-bitports  (PORTA  and  PORTB).
     * The  MCP23X17  can  beconfigured  to  operate  in  the  8-bit  or  16-bit modes  via IOCON.BANK.
     */
    enum class REG_BANK_MODE : uint8_t
    {
        SEQUENTIAL     = 0, // Bank 0 (Sequential mode)  byte mode
        NON_SEQUENTIAL = 1, // Bank 1 (Non-sequential mode)
        MAX,
    };

private:
    // Sequential mode  enables  automatic  address  pointer incrementing.
    // If IOCON.BANK is set to 0, the address pointer will increment after each read or write operation.
    enum class REG_BANK0 : uint8_t
    {
        IODIRA   = 0x00,
        IODIRB   = 0x01,
        IPOLA    = 0x02,
        IPOLB    = 0x03,
        GPINTENA = 0x04,
        GPINTENB = 0x05,
        DEFVALA  = 0x06,
        DEFVALB  = 0x07,
        INTCONA  = 0x08,
        INTCONB  = 0x09,
        IOCONA   = 0x0A,
        IOCONB   = 0x0B,
        GPPUA    = 0x0C,
        GPPUB    = 0x0D,
        INTFA    = 0x0E,
        INTFB    = 0x0F,
        INTCAPA  = 0x10,
        INTCAPB  = 0x11,
        GPIOA    = 0x12,
        GPIOB    = 0x13,
        OLATA    = 0x14,
        OLATB    = 0x15
    };

    // If IOCON.BANK is set to 1, no automatic address pointer incrementing occurs.
    enum class REG_BANK1 : uint8_t
    {
        // Register addresses for MCP23017
        IODIRA   = 0x00,
        IODIRB   = 0x10,
        IPOLA    = 0x01,
        IPOLB    = 0x11,
        GPINTENA = 0x02,
        GPINTENB = 0x12,
        DEFVALA  = 0x03,
        DEFVALB  = 0x13,
        INTCONA  = 0x04,
        INTCONB  = 0x14,
        IOCONA   = 0x05,
        IOCONB   = 0x15,
        GPPUA    = 0x06,
        GPPUB    = 0x16,
        INTFA    = 0x07,
        INTFB    = 0x17,
        INTCAPA  = 0x08,
        INTCAPB  = 0x18,
        GPIOA    = 0x09,
        GPIOB    = 0x19,
        OLATA    = 0x0A,
        OLATB    = 0x1A,
    };

    enum class REG_TYPE : uint8_t
    {
        IODIR   = 0,  // I/O Direction Register
        IPOL    = 1,  // Input Polarity Register
        GPINTEN = 2,  // GPIO Interrupt-on-Change Enable Register
        DEFVAL  = 3,  // Default Value Register
        INTCON  = 4,  // Interrupt-on-Change Control Register
        IOCON   = 5,  // I/O Expander Configuration Register
        GPPU    = 6,  // GPIO Pull-up Resistor Register
        INTF    = 7,  // Interrupt Flag Register
        INTCAP  = 8,  // Interrupt Captured Value Register
        GPIO    = 9,  // General Purpose I/O Port Register
        OLAT    = 10, // Output Latch Register
        MAX,
    };

    static constexpr uint8_t REG_TYPE_COUNT      = static_cast<uint8_t>(REG_TYPE::MAX);
    static constexpr uint8_t REG_BANK_MODE_COUNT = static_cast<uint8_t>(REG_BANK_MODE::MAX);
    static constexpr uint8_t PORT_COUNT          = static_cast<uint8_t>(PORT::MAX);

    static constexpr uint8_t regMap[REG_TYPE_COUNT][REG_BANK_MODE_COUNT][PORT_COUNT] = {

        // [IODIR   = 0] I/O Direction Register
        {{static_cast<uint8_t>(REG_BANK0::IODIRA), static_cast<uint8_t>(REG_BANK0::IODIRB)},  // BANK0
         {static_cast<uint8_t>(REG_BANK1::IODIRA), static_cast<uint8_t>(REG_BANK1::IODIRB)}}, // BANK1

        // [IPOL    = 1] Input Polarity Register
        {{static_cast<uint8_t>(REG_BANK0::IPOLA), static_cast<uint8_t>(REG_BANK0::IPOLB)},  // BANK0
         {static_cast<uint8_t>(REG_BANK1::IPOLA), static_cast<uint8_t>(REG_BANK1::IPOLB)}}, // BANK1

        // [GPINTEN = 2] GPIO Interrupt-on-Change Enable Register
        {{static_cast<uint8_t>(REG_BANK0::GPINTENA), static_cast<uint8_t>(REG_BANK0::GPINTENB)},  // BANK0
         {static_cast<uint8_t>(REG_BANK1::GPINTENA), static_cast<uint8_t>(REG_BANK1::GPINTENB)}}, // BANK1

        // [DEFVAL  = 3] Default Value Register
        {{static_cast<uint8_t>(REG_BANK0::DEFVALA), static_cast<uint8_t>(REG_BANK0::DEFVALB)},  // BANK0
         {static_cast<uint8_t>(REG_BANK1::DEFVALA), static_cast<uint8_t>(REG_BANK1::DEFVALB)}}, // BANK1

        // [INTCON  = 4] Interrupt-on-Change Control Register
        {{static_cast<uint8_t>(REG_BANK0::INTCONA), static_cast<uint8_t>(REG_BANK0::INTCONB)},  // BANK0
         {static_cast<uint8_t>(REG_BANK1::INTCONA), static_cast<uint8_t>(REG_BANK1::INTCONB)}}, // BANK1

        // [IOCON   = 5] I/O Expander Configuration Register
        {{static_cast<uint8_t>(REG_BANK0::IOCONA), static_cast<uint8_t>(REG_BANK0::IOCONB)},  // BANK0
         {static_cast<uint8_t>(REG_BANK1::IOCONA), static_cast<uint8_t>(REG_BANK1::IOCONB)}}, // BANK1

        // [GPPU    = 6] GPIO Pull-up Resistor Register
        {{static_cast<uint8_t>(REG_BANK0::GPPUA), static_cast<uint8_t>(REG_BANK0::GPPUB)},  // BANK0
         {static_cast<uint8_t>(REG_BANK1::GPPUA), static_cast<uint8_t>(REG_BANK1::GPPUB)}}, // BANK1

        // [INTF    = 7] Interrupt Flag Register
        {{static_cast<uint8_t>(REG_BANK0::INTFA), static_cast<uint8_t>(REG_BANK0::INTFB)},  // BANK0
         {static_cast<uint8_t>(REG_BANK1::INTFA), static_cast<uint8_t>(REG_BANK1::INTFB)}}, // BANK1

        // [INTCAP  = 8] Interrupt Captured Value Register
        {{static_cast<uint8_t>(REG_BANK0::INTCAPA), static_cast<uint8_t>(REG_BANK0::INTCAPB)},  // BANK0
         {static_cast<uint8_t>(REG_BANK1::INTCAPA), static_cast<uint8_t>(REG_BANK1::INTCAPB)}}, // BANK1

        // [GPIO    = 9] General Purpose I/O Port Register
        {{static_cast<uint8_t>(REG_BANK0::GPIOA), static_cast<uint8_t>(REG_BANK0::GPIOB)},  // BANK0
         {static_cast<uint8_t>(REG_BANK1::GPIOA), static_cast<uint8_t>(REG_BANK1::GPIOB)}}, // BANK1

        // [OLAT    = 10] Output Latch Register
        {{static_cast<uint8_t>(REG_BANK0::OLATA), static_cast<uint8_t>(REG_BANK0::OLATB)},  // BANK0
         {static_cast<uint8_t>(REG_BANK1::OLATA), static_cast<uint8_t>(REG_BANK1::OLATB)}}, // BANK1
    };

    // IOCON Register Bits
    enum class IOCON_BITS : uint8_t
    {
        BANK   = 0x80, // Controls how the registers are addressed
        MIRROR = 0x40, // INT pins mirror bit
        SEQOP  = 0x20, // Sequential operation mode bit
        DISSLW = 0x10, // Slew rate control bit for SDA output
        HAEN   = 0x08, // Hardware address enable bit (MCP23S17 only)
        ODR    = 0x04, // Configures INT pin as open-drain output
        INTPOL = 0x02, // Polarity of INT output pin
        INTCC  = 0x01  // Interrupt clearing control
    };

    typedef struct
    {
        uint8_t directionA; // I/O direction for port A (0 = input, 1 = output)
        uint8_t directionB; // I/O direction for port B (0 = input, 1 = output)
        uint8_t pullupA;    // Pull-up resistor configuration for port A
        uint8_t pullupB;    // Pull-up resistor configuration for port B
        uint8_t polarityA;  // Input polarity for port A
        uint8_t polarityB;  // Input polarity for port B
        uint8_t intEnableA; // Interrupt enable for port A
        uint8_t intEnableB; // Interrupt enable for port B
    } mcp23017_config_t;

    typedef enum class get_command_type_t
    {
        GET_NUMBER_OF_PORTS, // Get number of ports
        GET_NUMBER_OF_PINS,  // Get number of pins

        GET_DIRECTION,          // Get I/O direction
        GET_POLARITY,           // Get input polarity
        GET_INTERRUPT_ENABLE,   // Get interrupt enable status
        GET_DEFAULT_VALUE,      // Get default value for pins
        GET_INTERRUPT_CONTROL,  // Get interrupt control settings
        GET_PULL_UP_RESISTOR,   // Get pull-up resistor status
        GET_INTERRUPT_FLAG,     // Get interrupt flag status
        GET_INTERRUPT_CAPTURED, // Get captured interrupt value
        GET_GPIO_PORT,          // Get GPIO port value
        GET_OUTPUT_LATCH,       // Get output latch value
    } get_command_type_t;

private:
    bool          _isInitialized; // True if the MCP23017 is initialized
    bool          _started;
    IHAL_COM&     _comInterface;  // Reference to the communication interface
    IHAL_IO_GPIO* _interruptPinA; // Pointer to the interrupt pin A
    IHAL_IO_GPIO* _interruptPinB; // Pointer to the interrupt pin B
    uint8_t       _deviceAddress; // I2C address of the MCP23017
    // IOCON register bits
    REG_BANK_MODE _bankMode;                      // True if banked mode is enabled
    bool          _isMirrorEnabled;               // True if mirror mode is enabled
    bool          _isSequentialOperationDisabled; // True if sequential mode is disabled
    bool          _isSlewRateDisabled;            // True if slew rate is disabled
    bool          _isHardwareAddressEnabled;      // True if hardware address is enabled (A0, A1, A2)
    bool          _isOpenDrainEnabled;            // True if open-drain output is enabled
    bool          _isIntPolarityActiveHigh;       // True if interrupt polarity is active high

    // Last known input states for change detection
    uint8_t _lastInputStatePortA;
    uint8_t _lastInputStatePortB;

    TaskHandle_t _interruptTaskHandle; // Task handle for interrupt handling task

    /**
     * @brief Map to store interrupt handlers for input pins.
     *
     * The key is a pair of PORT and pin number.
     * The value is a pair of function pointer and parameters.
     * The function pointer is of type void (*)(void*, bool) where the first parameter
     * is a void* for user-defined parameters and the second parameter is a bool indicating the pin state will be passed to the handler.
     */
    std::map<std::pair<PORT, uint8_t>, std::pair<void (*)(void*, bool), void*>> _inputPinInterruptHandlers;

public: // Interface methods
    cpx_mcp23x17(IHAL_COM& comInterface, REG_BANK_MODE bankMode, IHAL_IO_GPIO* interruptPinA, IHAL_IO_GPIO* interruptPinB);
    ~cpx_mcp23x17();

    sys_error_t init(void* params) override;

    sys_error_t start() override;

    sys_error_t get(void* data) override;

    sys_error_t set(void* data) override;

    sys_error_t stop() override;

    sys_error_t deInit() override;

public: // User-defined methods
    /**
     * @brief Register an interrupt handler for a specific input pin.
     *
     * @param port Port A or B
     * @param pinNo Pin number
     * @param handler Function pointer to the interrupt handler
     * @param params Parameters to pass to the handler
     * @return sys_error_t
     */
    sys_error_t registerInputPinInterruptHandler(PORT port, uint8_t pinNo, void (*handler)(void* params, bool capturedLevel), void* params);

    sys_error_t   handleInterruptA(void* params);
    sys_error_t   handleInterruptB(void* params);
    sys_error_t   handleInterrupt(PORT port, void* params);
    IHAL_IO_GPIO* getGpioA();
    IHAL_IO_GPIO* getGpioB();

    /**
     * @brief Set the I/O direction for a specific port or pin.
     * @param value Use INPUT_MODE or OUTPUT_MODE
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    CPX_MCP23X17_REG_DECL(Direction);

    /**
     * @brief Set the polarity for a specific port or pin.
     * @param value Use POLARITY_INVERTED or POLARITY_NORMAL
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    CPX_MCP23X17_REG_DECL(Polarity);

    /**
     * @brief Enable or disable interrupt on change for a specific port or pin.
     * @param value Use INTERRUPT_ON_CHANGE_ENABLED or INTERRUPT_ON_CHANGE_DISABLED
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    CPX_MCP23X17_REG_DECL(InterruptEnable);

    /**
     * @brief Set the default value for a specific port or pin.
     * @param value Use LOGIC_HIGH or LOGIC_LOW
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    CPX_MCP23X17_REG_DECL(DefaultValue);

    /**
     * @brief Set the interrupt control for a specific port or pin.
     * @param value Use COMPARE_AGAINST_DEFVAL_STATE or COMPARE_AGAINST_PREVIOUS_STATE
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    CPX_MCP23X17_REG_DECL(InterruptControl);

    /**
     * @brief Set the pull-up resistor for a specific port or pin.
     * If a bit is set and the corresponding pin is configured as an input, the corresponding port pin is
     * internally pulled up with a 100 kOhm resistor.
     * @param value Use PULL_UP_RESISTOR_ENABLED or PULL_UP_RESISTOR_DISABLED
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    CPX_MCP23X17_REG_DECL(PullUpResistor);

    /**
     * @brief Read the interrupt flag for a specific port or pin. Read-only register.
     * The INTF register reflects the interrupt condition on the port pins of any pin that is enabled for interrupts via the
     * GPINTEN register. A set bit indicates that the associated pin caused the interrupt.
     * This register is read-only. Writes to this register will be ignored.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    CPX_MCP23X17_REG_DECL_READONLY(InterruptFlag);

    /**
     * @brief Read the interrupt captured for a specific port or pin. Read-only register.
     * The INTCAP register captures the GPIO port value at the time the interrupt occurred. The register is
     * read-only and is updated only when an interrupt occurs. The register remains unchanged until the
     * interrupt is cleared via a read of INTCAP or GPIO.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    CPX_MCP23X17_REG_DECL_READONLY(InterruptCaptured);

    /**
     * @brief Read or write the GPIO port for a specific port or pin.
     * @param value Use LOGIC_HIGH or LOGIC_LOW
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    CPX_MCP23X17_REG_DECL(Gpio);

    /**
     * @brief Read or write the latch for a specific port or pin.
     * The OLAT register reflects the output latch state of the port pins. A write to this register will
     * update the output latch and the GPIO register.
     * @param value Use LOGIC_HIGH or LOGIC_LOW
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    CPX_MCP23X17_REG_DECL(Latch);

    // IOCON Register Methods
    sys_error_t setBankMode(REG_BANK_MODE bankMode);
    sys_error_t setIOCONRegister();

private: // Register operations
    /**
     * @brief Write a value to a specific register.
     *
     * @param reg The register address to write to.
     * @param value The value to write to the register.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    sys_error_t writeRegister(const uint8_t reg, const uint8_t value);

    /**
     * @brief Read a value from a specific register.
     *
     * @param reg The register address to read from.
     * @param value Reference to store the read value.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    sys_error_t readRegister(const uint8_t reg, uint8_t& value);
    sys_error_t readRegisterByTypeByte(const REG_TYPE regType, const PORT port, uint8_t& value);
    sys_error_t readRegisterByTypeBit(const REG_TYPE regType, const PORT port, const uint8_t bitNo, bool& value);
    /**
     * @brief Update a specific register with a REG_TYPE, PORT, MASK, and VALUE.
     *
     * @param reg The register address to update.
     * @param mask The mask to apply to the register value.
     * @param value The value to set in the register after applying the mask.
     * @param regType The type of register to update (IODIR, IPOL, etc.).
     * @param port The port to update (A or B).
     * @param pinNo The pin number to update (0-7).
     * @param value The value to set in the register after applying the mask.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    sys_error_t updateRegisterMasked(const uint8_t reg, const uint8_t mask, const uint8_t value, const bool verify);
    sys_error_t updateRegisterByTypePortMaskByte(const REG_TYPE regType, const PORT port, const uint8_t mask, const uint8_t value, const bool verify);
    sys_error_t updateRegisterByTypePortBit(const REG_TYPE regType, const PORT port, const uint8_t bitNo, const bool value, const bool verify);

    sys_error_t verifyRegister(const uint8_t reg, const uint8_t expectedValue);

    sys_error_t checkParameters(const REG_TYPE regType, const PORT port, const uint8_t bitNo);
};
#endif /* CPX_MCP23X17_HPP */
