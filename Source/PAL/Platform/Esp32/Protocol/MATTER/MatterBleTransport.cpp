/** @file       MatterBleTransport.cpp
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

/*** INCLUDES ****************************************************************/

#include "MatterBleTransport.hpp"

#include "System/error_definitions.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

#include <cstring>

/*** MACROS ******************************************************************/

#define MATTER_BTP_HEADER_LEN       4U
#define MATTER_ADV_FLAGS_LEN        2U
#define MATTER_ADV_SVC_DATA_HDR_LEN 3U
#define MATTER_ADV_MAX_LEN          31U

/*** CLASS IMPLEMENTATION ****************************************************/

MatterBleTransport::MatterBleTransport(IHal_Cpx_Ble& hal)
    : _hal(hal)
    , _config{}
    , _messageCb(nullptr)
    , _messageUserData(nullptr)
    , _stateCb(nullptr)
    , _stateUserData(nullptr)
    , _state(matter_commissioning_state_t::IDLE)
    , _mtu(23)
    , _txSeq(0)
    , _rxSeq(0)
    , _expectedLen(0)
    , _assembledLen(0)
    , _assemblyBuf{}
    , _assembling(false)
{
    std::memset(&_config, 0, sizeof(_config));
    (void)_hal.setEventCallback(_halEventThunk, this);
}

MatterBleTransport::~MatterBleTransport()
{
    (void)_hal.setEventCallback(nullptr, nullptr);
}

sys_error_t MatterBleTransport::setCommissioningConfig(const matter_commissioning_config_t& config)
{
    if (config.discriminator > 0x0FFFU)
    {
        return ERROR_INVALID_ARG;
    }
    if (config.passcode > 0x0FFFFFFFU)
    {
        return ERROR_INVALID_ARG;
    }
    _config = config;
    return ERROR_SUCCESS;
}

sys_error_t MatterBleTransport::setMessageCallback(matter_btp_message_cb_t callback, void* userData)
{
    _messageCb       = callback;
    _messageUserData = userData;
    return ERROR_SUCCESS;
}

sys_error_t MatterBleTransport::setStateCallback(matter_commissioning_cb_t callback, void* userData)
{
    _stateCb       = callback;
    _stateUserData = userData;
    return ERROR_SUCCESS;
}

sys_error_t MatterBleTransport::startCommissioning()
{
    if (_state != matter_commissioning_state_t::IDLE && _state != matter_commissioning_state_t::FAILED)
    {
        return ERROR_INVALID_STATE;
    }

    uint8_t     payload[MATTER_ADV_MAX_LEN];
    size_t      payloadLen = 0;
    sys_error_t err        = _encodeAdvertisingPayload(payload, sizeof(payload), payloadLen);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    err = _hal.startAdvertising(payload, payloadLen);
    if (err != ERROR_SUCCESS)
    {
        SYS_LOG_E("MatterBleTransport: startAdvertising failed (%d)", (int)err);
        return err;
    }

    _setState(matter_commissioning_state_t::ADVERTISING);
    return ERROR_SUCCESS;
}

sys_error_t MatterBleTransport::abortCommissioning()
{
    (void)_hal.stopAdvertising();
    if (_hal.isConnected())
    {
        /* Connection handle is managed by the HAL; disconnect via HAL state. */
    }
    _assembling   = false;
    _assembledLen = 0;
    _setState(matter_commissioning_state_t::FAILED);
    return ERROR_SUCCESS;
}

sys_error_t MatterBleTransport::notifyCommissioningComplete()
{
    (void)_hal.stopAdvertising();

    if (_config.deinit_ble_on_commissioned)
    {
        (void)_hal.deInit();
    }

    _setState(matter_commissioning_state_t::COMMISSIONED);
    return ERROR_SUCCESS;
}

sys_error_t MatterBleTransport::sendMessage(const uint8_t* message, size_t len)
{
    if (message == nullptr || len == 0)
    {
        return ERROR_INVALID_ARG;
    }
    if (_state != matter_commissioning_state_t::SESSION_ACTIVE && _state != matter_commissioning_state_t::HANDSHAKE)
    {
        return ERROR_INVALID_STATE;
    }

    const uint16_t maxPayload = (uint16_t)((_mtu > MATTER_BTP_HEADER_LEN) ? (_mtu - MATTER_BTP_HEADER_LEN) : 1U);
    size_t         offset     = 0;
    bool           first      = true;

    while (offset < len)
    {
        size_t chunk = len - offset;
        if (chunk > maxPayload)
        {
            chunk = maxPayload;
        }

        uint8_t packet[MATTER_BTP_HEADER_LEN + 512];
        uint8_t flags = 0;
        if (!first)
        {
            flags |= (uint8_t)matter_btp_flags_t::CONTINUATION;
        }

        packet[0] = flags;
        packet[1] = _txSeq++;
        packet[2] = _rxSeq;
        packet[3] = (uint8_t)(len & 0xFFU);
        std::memcpy(&packet[MATTER_BTP_HEADER_LEN], &message[offset], chunk);

        sys_error_t err = _hal.sendIndication(0, packet, MATTER_BTP_HEADER_LEN + chunk);
        if (err != ERROR_SUCCESS)
        {
            return err;
        }

        offset += chunk;
        first = false;
    }

    return ERROR_SUCCESS;
}

matter_commissioning_state_t MatterBleTransport::getState() const
{
    return _state;
}

sys_error_t MatterBleTransport::_encodeAdvertisingPayload(uint8_t* out, size_t outMax, size_t& outLen)
{
    if (out == nullptr)
    {
        return ERROR_NULL_POINTER;
    }

    size_t idx = 0;

    /* Flags AD structure */
    out[idx++] = 0x02;
    out[idx++] = 0x01;
    out[idx++] = 0x06;

    /* Service Data AD structure: 16-bit UUID + 12-bit discriminator + Vendor ID + Product ID */
    const size_t svcDataLen = 2U + 2U + 2U + 2U; /* UUID16 + disc/vendor/product packed */
    if (idx + 2U + svcDataLen > outMax)
    {
        return ERROR_BUFFER_OVERFLOW;
    }

    out[idx++] = (uint8_t)(1U + svcDataLen);
    out[idx++] = 0x16;
    out[idx++] = (uint8_t)(0xF6U & 0xFFU);
    out[idx++] = (uint8_t)((0xFFF6U >> 8) & 0xFFU);

    /* 12-bit discriminator (bits 15..4) + 4-bit reserved/vendor high nibble */
    uint16_t discField = (uint16_t)((_config.discriminator & 0x0FFFU) << 4);
    out[idx++]         = (uint8_t)(discField & 0xFFU);
    out[idx++]         = (uint8_t)((discField >> 8) & 0xFFU);

    out[idx++] = (uint8_t)(_config.vendor_id & 0xFFU);
    out[idx++] = (uint8_t)((_config.vendor_id >> 8) & 0xFFU);
    out[idx++] = (uint8_t)(_config.product_id & 0xFFU);
    out[idx++] = (uint8_t)((_config.product_id >> 8) & 0xFFU);

    outLen = idx;
    return ERROR_SUCCESS;
}

sys_error_t MatterBleTransport::_handleBtpPacket(const uint8_t* packet, size_t len)
{
    if (packet == nullptr || len < MATTER_BTP_HEADER_LEN)
    {
        return ERROR_INVALID_ARG;
    }

    const uint8_t  flags  = packet[0];
    const uint8_t  seq    = packet[1];
    const uint8_t  ack    = packet[2];
    const uint16_t msgLen = packet[3];
    (void)ack;

    _rxSeq = seq;

    const uint8_t* fragment    = &packet[MATTER_BTP_HEADER_LEN];
    const size_t   fragmentLen = len - MATTER_BTP_HEADER_LEN;

    return _reassemble(fragment, fragmentLen, flags);
}

sys_error_t MatterBleTransport::_reassemble(const uint8_t* fragment, size_t len, uint8_t flags)
{
    if (fragment == nullptr && len > 0)
    {
        return ERROR_NULL_POINTER;
    }

    const bool isContinuation = (flags & (uint8_t)matter_btp_flags_t::CONTINUATION) != 0;

    if (!isContinuation)
    {
        _assembledLen = 0;
        _assembling   = true;
    }

    if (!_assembling)
    {
        return ERROR_INVALID_STATE;
    }

    if (_assembledLen + len > sizeof(_assemblyBuf))
    {
        _assembling = false;
        return ERROR_BUFFER_OVERFLOW;
    }

    std::memcpy(&_assemblyBuf[_assembledLen], fragment, len);
    _assembledLen += (uint16_t)len;

    if (!isContinuation)
    {
        /* Single-packet message complete */
        _assembling = false;
        if (_messageCb != nullptr)
        {
            _messageCb(_assemblyBuf, _assembledLen, _messageUserData);
        }
        _assembledLen = 0;
    }

    return ERROR_SUCCESS;
}

void MatterBleTransport::_setState(matter_commissioning_state_t state)
{
    _state = state;
    if (_stateCb != nullptr)
    {
        _stateCb(state, _stateUserData);
    }
}

void MatterBleTransport::_halEventThunk(hal_ble_event_t event, const hal_ble_event_data_t& data, void* userData)
{
    if (userData != nullptr)
    {
        static_cast<MatterBleTransport*>(userData)->_onHalEvent(event, data);
    }
}

void MatterBleTransport::_onHalEvent(hal_ble_event_t event, const hal_ble_event_data_t& data)
{
    switch (event)
    {
        case hal_ble_event_t::CONNECTED:
            _setState(matter_commissioning_state_t::CONNECTED);
            break;

        case hal_ble_event_t::DISCONNECTED:
            _assembling   = false;
            _assembledLen = 0;
            if (_state != matter_commissioning_state_t::COMMISSIONED)
            {
                _setState(matter_commissioning_state_t::FAILED);
            }
            break;

        case hal_ble_event_t::MTU_UPDATED:
            _mtu = (data.mtu >= 23U) ? data.mtu : 23U;
            break;

        case hal_ble_event_t::DATA_RECEIVED:
            if (data.payload != nullptr && data.payload_len > 0)
            {
                (void)_handleBtpPacket(data.payload, data.payload_len);
            }
            break;

        case hal_ble_event_t::ADVERTISING_STOPPED:
            if (_state == matter_commissioning_state_t::ADVERTISING)
            {
                _setState(matter_commissioning_state_t::IDLE);
            }
            break;

        default:
            break;
    }
}
