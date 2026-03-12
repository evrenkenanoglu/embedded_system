#pragma once
#include "System/system.h"
#include <stdint.h>

// Namespace for Matter Types
namespace MatterTypes
{
    // Supported Device Types
    enum class Device
    {
        ROOT_NODE_DEVICE,
        OTA_REQUESTOR_DEVICE,
        OTA_PROVIDER_DEVICE,
        POWER_SOURCE_DEVICE,
        AGGREGATOR_DEVICE,
        BRIDGED_NODE_DEVICE,
        CONTROL_BRIDGE_DEVICE,
        ON_OFF_LIGHT_DEVICE,
        DIMMABLE_LIGHT_DEVICE,
        COLOR_TEMPERATURE_LIGHT_DEVICE,
        EXTENDED_COLOR_LIGHT_DEVICE,
        ON_OFF_LIGHT_SWITCH_DEVICE,
        DIMMER_SWITCH_DEVICE,
        COLOR_DIMMER_SWITCH_DEVICE,
        GENERIC_SWITCH_DEVICE,
        ON_OFF_PLUG_IN_UNIT_DEVICE,
        DIMMABLE_PLUG_IN_UNIT_DEVICE,
        MOUNTED_ON_OFF_CONTROL_DEVICE,
        MOUNTED_DIMMABLE_LOAD_CONTROL_DEVICE,
        TEMPERATURE_SENSOR_DEVICE,
        OCCUPANCY_SENSOR_DEVICE,
        CONTACT_SENSOR_DEVICE,
        LIGHT_SENSOR_DEVICE,
        PRESSURE_SENSOR_DEVICE,
        FLOW_SENSOR_DEVICE,
        HUMIDITY_SENSOR_DEVICE,
        ROOM_AIR_CONDITIONER_DEVICE,
        REFRIGERATOR_DEVICE,
        TEMPERATURE_CONTROLLED_CABINET_DEVICE,
        LAUNDRY_WASHER_DEVICE,
        DISH_WASHER_DEVICE,
        MICROWAVE_OVEN_DEVICE,
        SMOKE_CO_ALARM_DEVICE,
        LAUNDRY_DRYER_DEVICE,
        FAN_DEVICE,
        THERMOSTAT_DEVICE,
        AIR_QUALITY_SENSOR_DEVICE,
        AIR_PURIFIER_DEVICE,
        DOOR_LOCK_DEVICE,
        WINDOW_COVERING_DEVICE,
        PUMP_DEVICE,
        PUMP_CONTROLLER_DEVICE,
        MODE_SELECT_DEVICE,
        ROBOTIC_VACUUM_CLEANER_DEVICE,
        WATER_LEAK_DETECTOR_DEVICE,
        RAIN_SENSOR_DEVICE,
        COOK_SURFACE_DEVICE,
        COOKTOP_DEVICE,
        ELECTRICAL_SENSOR_DEVICE,
        OVEN_DEVICE,
        WATER_FREEZE_DETECTOR_DEVICE,
        ENERGY_EVSE_DEVICE,
        EXTRACTOR_HOOD_DEVICE,
        WATER_VALVE_DEVICE,
        DEVICE_ENERGY_MANAGEMENT_DEVICE,
        SECONDARY_NETWORK_INTERFACE_DEVICE,
        WATER_HEATER_DEVICE,
        SOLAR_POWER_DEVICE,
        BATTERY_STORAGE_DEVICE,
        THREAD_BORDER_ROUTER_DEVICE,
        HEAT_PUMP_DEVICE,
        THERMOSTAT_CONTROLLER_DEVICE,
        CAMERA_DEVICE,
        CLOSURE_CONTROLLER_DEVICE,
        CLOSURE_DEVICE,
        CLOSURE_PANEL_DEVICE,
        CHIME_DEVICE,
    };

    // Supported Event Types
    enum class Event
    {
        PRE_UPDATE,
        POST_UPDATE,
        READ,
    };

    // Supported Attribute Value Types
    enum class Value
    {
        INVALID           = 0,
        BOOLEAN           = 1,
        INTEGER           = 2,
        FLOAT             = 3,
        ARRAY             = 4,
        CHAR_STRING       = 5,
        OCTET_STRING      = 6,
        INT8              = 7,
        UINT8             = 8,
        INT16             = 9,
        UINT16            = 10,
        INT32             = 11,
        UINT32            = 12,
        INT64             = 13,
        UINT64            = 14,
        ENUM8             = 15,
        BITMAP8           = 16,
        BITMAP16          = 17,
        BITMAP32          = 18,
        ENUM16            = 19,
        LONG_CHAR_STRING  = 20,
        LONG_OCTET_STRING = 21,
    };

    // Attribute
    typedef union
    {
        /** Boolean */
        bool b;
        /** Integer */
        int i;
        /** Float */
        float f;
        /** 8 bit signed integer */
        int8_t i8;
        /** 8 bit unsigned integer */
        uint8_t u8;
        /** 16 bit signed integer */
        int16_t i16;
        /** 16 bit unsigned integer */
        uint16_t u16;
        /** 32 bit signed integer */
        int32_t i32;
        /** 32 bit unsigned integer */
        uint32_t u32;
        /** 64 bit signed integer */
        int64_t i64;
        /** 64 bit unsigned integer */
        uint64_t u64;
        /** Array */
        struct
        {
            /** Buffer */
            uint8_t* b;
            /** Data size */
            uint16_t s;
            /** Data max size */
            uint16_t max;
            /** Total size */
            uint16_t t;
        } a;
        /** Pointer */
        void* p;
    } Value_t;

    typedef struct
    {
        /** Type of Value */
        Value type;
        /** Actual value. Depends on the type */
        Value_t value;
    } AttributeValue_t;

    using OnRemoteUpdate_t = sys_error_t (*)(Event event, AttributeValue_t* value, void* params);

    using OnLocalStateUpdate_t = sys_error_t (*)(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, AttributeValue_t* value);

}; // namespace MatterTypes