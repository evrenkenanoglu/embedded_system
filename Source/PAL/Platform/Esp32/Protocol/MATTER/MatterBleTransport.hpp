/** @file       MatterBleTransport.hpp
 *  @brief      BLE transport adapter for Matter commissioning (BTP framing,
 *              advertising payload encoding, commissioning lifecycle).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */
#ifndef MATTER_BLE_TRANSPORT_HPP
#define MATTER_BLE_TRANSPORT_HPP

#include "HAL/IHAL/IHal_Cpx_Ble.h"

#include <cstddef>
#include <cstdint>

/** MACROS ********************************************************************/

/** TYPEDEFS ******************************************************************/

/**
 * @brief Matter BTP (Bluetooth Transport Protocol) packet header flags.
 */
enum class matter_btp_flags_t : uint8_t
{
    NONE         = 0x00,
    SYNCHRONIZE  = 0x01, /*!< Handshake / session start */
    FINISH       = 0x02, /*!< Session close */
    ACK          = 0x04, /*!< Acknowledgement present */
    CONTINUATION = 0x08, /*!< Fragment continuation */
    MANAGEMENT   = 0x10  /*!< Management channel */
};

/**
 * @brief Commissioning session state.
 */
enum class matter_commissioning_state_t : uint8_t
{
    IDLE = 0,
    ADVERTISING,
    CONNECTED,
    HANDSHAKE,
    SESSION_ACTIVE,
    COMMISSIONED,
    FAILED
};

/**
 * @brief Commissioning parameters (discriminator, passcode, timeouts).
 */
typedef struct
{
    uint16_t discriminator;              /*!< 12-bit setup discriminator */
    uint32_t passcode;                   /*!< 27-bit setup passcode */
    uint16_t vendor_id;                  /*!< 16-bit Vendor ID */
    uint16_t product_id;                 /*!< 16-bit Product ID */
    uint32_t timeout_sec;                /*!< Commissioning window timeout in seconds */
    bool     deinit_ble_on_commissioned; /*!< Release BLE resources after commissioning */
} matter_commissioning_config_t;

/**
 * @brief Callback invoked when a complete Matter BTP message is reassembled.
 */
typedef void (*matter_btp_message_cb_t)(const uint8_t* message, size_t len, void* userData);

/**
 * @brief Callback invoked on commissioning lifecycle transitions.
 */
typedef void (*matter_commissioning_cb_t)(matter_commissioning_state_t state, void* userData);

/**
 * @class MatterBleTransport
 * @brief BLE transport adapter bridging IHal_Cpx_Ble to the Matter BTP
 *        session layer.
 *
 * Responsibilities:
 *  - Encode the Matter commissioning advertising payload (Service Data with
 *    12-bit discriminator, Vendor ID, Product ID).
 *  - Reassemble fragmented BTP packets (Flags, Seq, Ack, Length) up to the
 *    negotiated MTU and dispatch complete messages to the session sink.
 *  - Manage the commissioning lifecycle (advertise -> connect -> handshake ->
 *    session -> commissioned) and release BLE resources when configured.
 */
class MatterBleTransport
{
public:
    MatterBleTransport(IHal_Cpx_Ble& hal);
    ~MatterBleTransport();

    /* Rule of Five: non-copyable/non-movable */
    MatterBleTransport(const MatterBleTransport&)            = delete;
    MatterBleTransport& operator=(const MatterBleTransport&) = delete;
    MatterBleTransport(MatterBleTransport&&)                 = delete;
    MatterBleTransport& operator=(MatterBleTransport&&)      = delete;

    /**
     * @brief Configure commissioning parameters.
     */
    sys_error_t setCommissioningConfig(const matter_commissioning_config_t& config);

    /**
     * @brief Register the BTP message sink (complete reassembled messages).
     */
    sys_error_t setMessageCallback(matter_btp_message_cb_t callback, void* userData = nullptr);

    /**
     * @brief Register the commissioning lifecycle callback.
     */
    sys_error_t setStateCallback(matter_commissioning_cb_t callback, void* userData = nullptr);

    /**
     * @brief Start the commissioning flow: encode advertising payload and
     *        start BLE advertising.
     */
    sys_error_t startCommissioning();

    /**
     * @brief Abort commissioning and release BLE resources.
     */
    sys_error_t abortCommissioning();

    /**
     * @brief Notify the transport that commissioning completed successfully.
     *        Stops advertising and (optionally) deinitializes BLE.
     */
    sys_error_t notifyCommissioningComplete();

    /**
     * @brief Send a Matter BTP message (fragmented to the negotiated MTU).
     */
    sys_error_t sendMessage(const uint8_t* message, size_t len);

    /**
     * @brief Get the current commissioning state.
     */
    matter_commissioning_state_t getState() const;

private:
    IHal_Cpx_Ble&                 _hal;
    matter_commissioning_config_t _config;
    matter_btp_message_cb_t       _messageCb;
    void*                         _messageUserData;
    matter_commissioning_cb_t     _stateCb;
    void*                         _stateUserData;
    matter_commissioning_state_t  _state;
    uint16_t                      _mtu;
    uint8_t                       _txSeq;
    uint8_t                       _rxSeq;
    uint16_t                      _expectedLen;
    uint16_t                      _assembledLen;
    uint8_t                       _assemblyBuf[2048];
    bool                          _assembling;

    sys_error_t _encodeAdvertisingPayload(uint8_t* out, size_t outMax, size_t& outLen);
    sys_error_t _handleBtpPacket(const uint8_t* packet, size_t len);
    sys_error_t _reassemble(const uint8_t* fragment, size_t len, uint8_t flags);
    void        _setState(matter_commissioning_state_t state);
    static void _halEventThunk(hal_ble_event_t event, const hal_ble_event_data_t& data, void* userData);
    void        _onHalEvent(hal_ble_event_t event, const hal_ble_event_data_t& data);
};

#endif /* MATTER_BLE_TRANSPORT_HPP */
