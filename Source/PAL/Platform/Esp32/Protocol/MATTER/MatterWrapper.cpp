#include "MatterWrapper.hpp"
#include <string.h>

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;

namespace
{
/**
 * @brief Convert attribute callback type to IMatterStack event type
 * @param type Attribute callback type
 * @return MatterTypes::Event Converted event type
 */
static MatterTypes::Event convert_event_type(attribute::callback_type_t type)
{
    switch (type)
    {
        case attribute::PRE_UPDATE:
            return MatterTypes::Event::PRE_UPDATE;
        case attribute::POST_UPDATE:
            return MatterTypes::Event::POST_UPDATE;
        case attribute::READ:
            return MatterTypes::Event::READ;
        default:
            return MatterTypes::Event::READ;
    }
}

/**
 * @brief Convert ESP Matter generic value to Core AttributeValue_t
 * @param val Pointer to ESP Matter attribute value
 * @return MatterTypes::AttributeValue_t Complete structure with Type and Value
 */
static MatterTypes::AttributeValue_t convert_to_system_value(const esp_matter_attr_val_t* const val)
{
    // 1. Initialize with safe defaults
    MatterTypes::AttributeValue_t result;
    memset(&result, 0, sizeof(MatterTypes::AttributeValue_t));
    result.type = MatterTypes::Value::INVALID;

    // 2. Safety check
    if (!val)
        return result;

    switch (val->type)
    {
        // --- Boolean ---
        case ESP_MATTER_VAL_TYPE_BOOLEAN:
            result.type    = MatterTypes::Value::BOOLEAN;
            result.value.b = val->val.b;
            break;

        // --- Integers (Signed) ---
        case ESP_MATTER_VAL_TYPE_INTEGER:
            result.type    = MatterTypes::Value::INTEGER;
            result.value.i = val->val.i;
            break;
        case ESP_MATTER_VAL_TYPE_INT8:
            result.type     = MatterTypes::Value::INT8;
            result.value.i8 = val->val.i8;
            break;
        case ESP_MATTER_VAL_TYPE_INT16:
            result.type      = MatterTypes::Value::INT16;
            result.value.i16 = val->val.i16;
            break;
        case ESP_MATTER_VAL_TYPE_INT32:
            result.type      = MatterTypes::Value::INT32;
            result.value.i32 = val->val.i32;
            break;
        case ESP_MATTER_VAL_TYPE_INT64:
            result.type      = MatterTypes::Value::INT64;
            result.value.i64 = val->val.i64;
            break;

        // --- 8-Bit Unsigned / Enum / Bitmap ---
        case ESP_MATTER_VAL_TYPE_UINT8:
            result.type     = MatterTypes::Value::UINT8;
            result.value.u8 = val->val.u8;
            break;
        case ESP_MATTER_VAL_TYPE_ENUM8:
            result.type     = MatterTypes::Value::ENUM8;
            result.value.u8 = val->val.u8;
            break;
        case ESP_MATTER_VAL_TYPE_BITMAP8:
            result.type     = MatterTypes::Value::BITMAP8;
            result.value.u8 = val->val.u8;
            break;

        // --- 16-Bit Unsigned / Enum / Bitmap ---
        case ESP_MATTER_VAL_TYPE_UINT16:
            result.type      = MatterTypes::Value::UINT16;
            result.value.u16 = val->val.u16;
            break;
        case ESP_MATTER_VAL_TYPE_ENUM16:
            result.type      = MatterTypes::Value::ENUM16;
            result.value.u16 = val->val.u16;
            break;
        case ESP_MATTER_VAL_TYPE_BITMAP16:
            result.type      = MatterTypes::Value::BITMAP16;
            result.value.u16 = val->val.u16;
            break;

        // --- 32-Bit Unsigned / Bitmap ---
        case ESP_MATTER_VAL_TYPE_UINT32:
            result.type      = MatterTypes::Value::UINT32;
            result.value.u32 = val->val.u32;
            break;
        case ESP_MATTER_VAL_TYPE_BITMAP32:
            result.type      = MatterTypes::Value::BITMAP32;
            result.value.u32 = val->val.u32;
            break;

        // --- 64-Bit Unsigned ---
        case ESP_MATTER_VAL_TYPE_UINT64:
            result.type      = MatterTypes::Value::UINT64;
            result.value.u64 = val->val.u64;
            break;

        // --- Floating Point ---
        case ESP_MATTER_VAL_TYPE_FLOAT:
            result.type    = MatterTypes::Value::FLOAT;
            result.value.f = val->val.f;
            break;

        // --- Arrays & Strings ---
        case ESP_MATTER_VAL_TYPE_ARRAY:
            result.type      = MatterTypes::Value::ARRAY;
            result.value.a.b = (uint8_t*)val->val.a.b;
            result.value.a.s = val->val.a.s;
            result.value.a.t = val->val.a.t;
            break;

        case ESP_MATTER_VAL_TYPE_CHAR_STRING:
            result.type      = MatterTypes::Value::CHAR_STRING;
            result.value.a.b = (uint8_t*)val->val.a.b;
            result.value.a.s = val->val.a.s;
            result.value.a.t = val->val.a.t;
            break;

        case ESP_MATTER_VAL_TYPE_OCTET_STRING:
            result.type      = MatterTypes::Value::OCTET_STRING;
            result.value.a.b = (uint8_t*)val->val.a.b;
            result.value.a.s = val->val.a.s;
            result.value.a.t = val->val.a.t;
            break;

        case ESP_MATTER_VAL_TYPE_LONG_CHAR_STRING:
            result.type      = MatterTypes::Value::LONG_CHAR_STRING;
            result.value.a.b = (uint8_t*)val->val.a.b;
            result.value.a.s = val->val.a.s;
            result.value.a.t = val->val.a.t;
            break;

        case ESP_MATTER_VAL_TYPE_LONG_OCTET_STRING:
            result.type      = MatterTypes::Value::LONG_OCTET_STRING;
            result.value.a.b = (uint8_t*)val->val.a.b;
            result.value.a.s = val->val.a.s;
            result.value.a.t = val->val.a.t;
            break;

        default:
            result.type = MatterTypes::Value::INVALID;
            break;
    }

    return result;
}

/**
 * @brief Convert generic MatterTypes value to ESP Matter SDK value
 * @param custom_val Pointer to your generic AttributeValue_t
 * @return esp_matter_attr_val_t The SDK specific struct
 */
static esp_matter_attr_val_t convert_to_esp_value(const MatterTypes::AttributeValue_t* const custom_val)
{
    // Initialize with "Invalid" defaults to be safe
    esp_matter_attr_val_t esp_val = {.type = ESP_MATTER_VAL_TYPE_INVALID, .val = {0}};

    if (!custom_val)
    {
        return esp_val;
    }

    switch (custom_val->type)
    {
        // --- Boolean ---
        case MatterTypes::Value::BOOLEAN:
            esp_val.type  = ESP_MATTER_VAL_TYPE_BOOLEAN;
            esp_val.val.b = custom_val->value.b;
            break;

        // --- Integers (Signed) ---
        case MatterTypes::Value::INTEGER:
            esp_val.type  = ESP_MATTER_VAL_TYPE_INTEGER;
            esp_val.val.i = custom_val->value.i;
            break;
        case MatterTypes::Value::INT8:
            esp_val.type   = ESP_MATTER_VAL_TYPE_INT8;
            esp_val.val.i8 = custom_val->value.i8;
            break;
        case MatterTypes::Value::INT16:
            esp_val.type    = ESP_MATTER_VAL_TYPE_INT16;
            esp_val.val.i16 = custom_val->value.i16;
            break;
        case MatterTypes::Value::INT32:
            esp_val.type    = ESP_MATTER_VAL_TYPE_INT32;
            esp_val.val.i32 = custom_val->value.i32;
            break;
        case MatterTypes::Value::INT64:
            esp_val.type    = ESP_MATTER_VAL_TYPE_INT64;
            esp_val.val.i64 = custom_val->value.i64;
            break;

        // --- Integers (Unsigned) & Bitmaps/Enums ---
        case MatterTypes::Value::UINT8:
            esp_val.type   = ESP_MATTER_VAL_TYPE_UINT8;
            esp_val.val.u8 = custom_val->value.u8;
            break;
        case MatterTypes::Value::ENUM8:
            esp_val.type   = ESP_MATTER_VAL_TYPE_ENUM8;
            esp_val.val.u8 = custom_val->value.u8;
            break;
        case MatterTypes::Value::BITMAP8:
            esp_val.type   = ESP_MATTER_VAL_TYPE_BITMAP8;
            esp_val.val.u8 = custom_val->value.u8;
            break;

        case MatterTypes::Value::UINT16:
            esp_val.type    = ESP_MATTER_VAL_TYPE_UINT16;
            esp_val.val.u16 = custom_val->value.u16;
            break;
        case MatterTypes::Value::ENUM16:
            esp_val.type    = ESP_MATTER_VAL_TYPE_ENUM16;
            esp_val.val.u16 = custom_val->value.u16;
            break;
        case MatterTypes::Value::BITMAP16:
            esp_val.type    = ESP_MATTER_VAL_TYPE_BITMAP16;
            esp_val.val.u16 = custom_val->value.u16;
            break;

        case MatterTypes::Value::UINT32:
            esp_val.type    = ESP_MATTER_VAL_TYPE_UINT32;
            esp_val.val.u32 = custom_val->value.u32;
            break;
        case MatterTypes::Value::BITMAP32:
            esp_val.type    = ESP_MATTER_VAL_TYPE_BITMAP32;
            esp_val.val.u32 = custom_val->value.u32;
            break;

        case MatterTypes::Value::UINT64:
            esp_val.type    = ESP_MATTER_VAL_TYPE_UINT64;
            esp_val.val.u64 = custom_val->value.u64;
            break;

        // --- Floating Point ---
        case MatterTypes::Value::FLOAT:
            esp_val.type  = ESP_MATTER_VAL_TYPE_FLOAT;
            esp_val.val.f = custom_val->value.f;
            break;

        // --- Complex Types (Strings / Arrays) ---
        case MatterTypes::Value::CHAR_STRING:
            esp_val.type    = ESP_MATTER_VAL_TYPE_CHAR_STRING;
            esp_val.val.a.b = custom_val->value.a.b;
            esp_val.val.a.s = custom_val->value.a.s;
            break;

        case MatterTypes::Value::OCTET_STRING:
            esp_val.type    = ESP_MATTER_VAL_TYPE_OCTET_STRING;
            esp_val.val.a.b = custom_val->value.a.b;
            esp_val.val.a.s = custom_val->value.a.s;
            break;

        case MatterTypes::Value::ARRAY:
            esp_val.type    = ESP_MATTER_VAL_TYPE_ARRAY;
            esp_val.val.a.b = custom_val->value.a.b;
            esp_val.val.a.s = custom_val->value.a.s;
            // Note: 't' or 'n' (total/count) might be needed depending on ESP SDK version
            // esp_val.val.a.t = custom_val->value.a.t;
            break;

        case MatterTypes::Value::LONG_CHAR_STRING:
            esp_val.type    = ESP_MATTER_VAL_TYPE_LONG_CHAR_STRING;
            esp_val.val.a.b = custom_val->value.a.b;
            esp_val.val.a.s = custom_val->value.a.s;
            break;

        case MatterTypes::Value::LONG_OCTET_STRING:
            esp_val.type    = ESP_MATTER_VAL_TYPE_LONG_OCTET_STRING;
            esp_val.val.a.b = custom_val->value.a.b;
            esp_val.val.a.s = custom_val->value.a.s;
            break;

        default:
            // Type is INVALID or unhandled
            esp_val.type = ESP_MATTER_VAL_TYPE_INVALID;
            break;
    }

    return esp_val;
}

} // namespace
// namespace

/**
 * @brief Attribute callback
 * @param type Callback type
 * @param endpoint_id Endpoint ID
 * @param cluster_id Cluster ID
 * @param attribute_id Attribute ID
 * @param val Pointer to attribute value
 * @param priv_data Private data
 * @return esp_err_t ESP_OK on success
 *
 */
esp_err_t MatterWrapper::onAttributeCallback(
    attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t* val, void* priv_data)
{
    SYS_LOG_D("Attribute Callback: type=%d, endpoint_id=%d, cluster_id=0x%08X, attribute_id=0x%08X", type, endpoint_id, cluster_id, attribute_id);

    SYS_LOG_D("Testing Device Retrieval from priv_data");
    // Retrieve the IMatterDevice object from priv_data
    IMatterDevice* device = static_cast<IMatterDevice*>(priv_data);

    SYS_LOG_D("Validating Device Object");
    // Validate device
    RETURN_IF_ERROR((device == nullptr || val == nullptr), ESP_ERR_INVALID_ARG, SYS_LOG_E("Invalid device in attribute callback"));

    // Convert to system type AttributeValue_t
    MatterTypes::AttributeValue_t attribute_value = convert_to_system_value(val);

    // Call the device's onRemoteCommand method for POST_UPDATE events
    if (type == attribute::POST_UPDATE)
    {
        device->onRemoteCommand(convert_event_type(type), &attribute_value);
    }

    return ESP_OK;
}

/**
 * @brief Identification cluster callback
 * @param type Callback type
 * @param endpoint_id Endpoint ID
 * @param effect_id Effect ID
 * @param effect_variant Effect Variant
 * @param priv_data Private data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t MatterWrapper::identificationCallback(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id, uint8_t effect_variant, void* priv_data)
{

    SYS_LOG_I("Identification Callback: type=%d, endpoint_id=%d, effect_id=%d, effect_variant=%d", type, endpoint_id, effect_id, effect_variant);
    return ESP_OK;
}

static void app_event_cb(const ChipDeviceEvent* event, intptr_t arg)
{
    switch (event->Type)
    {
        case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
            SYS_LOG_I("Interface IP Address Changed");
            break;

        case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
            SYS_LOG_I("Commissioning complete");
            break;

        case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
            SYS_LOG_I("Commissioning failed, fail safe timer expired");
            break;

        case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
            SYS_LOG_I("Commissioning session started");
            break;

        case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
            SYS_LOG_I("Commissioning session stopped");
            break;

        case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
            SYS_LOG_I("Commissioning window opened");
            break;

        case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
            SYS_LOG_I("Commissioning window closed");
            break;

        case chip::DeviceLayer::DeviceEventType::kBindingsChangedViaCluster:
        {
            SYS_LOG_I("Binding entry changed");
#if CONFIG_SUBSCRIBE_TO_ON_OFF_SERVER_AFTER_BINDING
            if (do_subscribe)
            {
                for (const auto& binding : chip::BindingTable::GetInstance())
                {
                    SYS_LOG_I(
                        "Read cached binding type=%d fabrixIndex=%d nodeId=0x" ChipLogFormatX64 " groupId=%d local endpoint=%d remote endpoint=%d cluster=" ChipLogFormatMEI,
                        binding.type,
                        binding.fabricIndex,
                        ChipLogValueX64(binding.nodeId),
                        binding.groupId,
                        binding.local,
                        binding.remote,
                        ChipLogValueMEI(binding.clusterId.value_or(0)));
                    if (binding.type == MATTER_UNICAST_BINDING && event->BindingsChanged.fabricIndex == binding.fabricIndex)
                    {
                        SYS_LOG_I(TAG, "Matched accessingFabricIndex with nodeId=0x" ChipLogFormatX64, ChipLogValueX64(binding.nodeId));

                        uint32_t                 attribute_id = chip::app::Clusters::OnOff::Attributes::OnOff::Id;
                        client::request_handle_t req_handle;
                        req_handle.type           = esp_matter::client::SUBSCRIBE_ATTR;
                        req_handle.attribute_path = {binding.remote, binding.clusterId.value(), attribute_id};
                        auto& server              = chip::Server::GetInstance();
                        client::connect(server.GetCASESessionManager(), binding.fabricIndex, binding.nodeId, &req_handle);
                        break;
                    }
                }
                do_subscribe = false;
            }
#endif
        }
        break;

        default:
            break;
    }
}

MatterWrapper::MatterWrapper()
    : _node_handle(nullptr)
{
}

MatterWrapper& MatterWrapper::get()
{
    static MatterWrapper instance;
    return instance;
}

sys_error_t MatterWrapper::init()
{
    // Create a Matter node and add the mandatory Root Node device type on endpoint 0
    node::config_t node_config;
    // Register the GLOBAL callback here
    _node_handle = node::create(&node_config, onAttributeCallback, identificationCallback);

    // Validation
    RETURN_IF_ERROR(_node_handle == nullptr, ERROR_FAIL, SYS_LOG_E("Failed to create Matter Node"));

    // Initialize the ESP Matter stack
    return ERROR_SUCCESS;
}

sys_error_t MatterWrapper::start()
{
    RETURN_IF_ERROR(esp_matter::start(app_event_cb) != ESP_OK, ERROR_FAIL, SYS_LOG_E("Failed to start ESP Matter"));

    SYS_LOG_I("Matter Stack Started");
    return ERROR_SUCCESS;
}

sys_error_t MatterWrapper::addDevice(IMatterDevice& device)
{
    // Ensure Matter Stack is initialized
    RETURN_IF_ERROR(_node_handle == nullptr, ERROR_INVALID_STATE, SYS_LOG_E("Matter Stack not initialized"));

    // Endpoint handle
    esp_matter::endpoint_t* ep_handle = nullptr;

    // Use the pointer to the generic interface as priv_data.
    // This allows the callback to know WHICH object belongs to this endpoint.
    void* priv_data = static_cast<void*>(&device);

    switch (device.getDeviceType())
    {
        case MatterTypes::Device::ON_OFF_PLUG_IN_UNIT_DEVICE:
        {
            // Configure On/Off Plugin Unit (Relay)
            on_off_plug_in_unit::config_t config;
            config.on_off.on_off = false;

            // Create the endpoint and register priv_data
            ep_handle = on_off_plug_in_unit::create(_node_handle, &config, ENDPOINT_FLAG_NONE, priv_data);

            // Set device cluster_id
            device.setClusterId(OnOff::Id);
            // Set attribute_id
            device.setAttributeId(OnOff::Attributes::OnOff::Id);
        }
        break;

        case MatterTypes::Device::ON_OFF_LIGHT_DEVICE:
        {
            // Configure generic On/Off Light (Actuator)
            // Note: If you meant a physical wall controller (remote), use on_off_light_switch
            // But usually for IoT projects "LightSwitch" implies a relay controlling a light.
            on_off_light::config_t config;
            config.on_off.on_off                   = false;
            config.on_off_lighting.start_up_on_off = nullptr;

            // Create the endpoint and register priv_data
            ep_handle = on_off_light::create(_node_handle, &config, ENDPOINT_FLAG_NONE, priv_data);

            // Set device cluster_id
            device.setClusterId(OnOff::Id);
            // Set attribute_id
            device.setAttributeId(OnOff::Attributes::OnOff::Id);
        }
        break;

            // Add other cases (Sensors, Dimmers) here...

        default:
            SYS_LOG_E("Unsupported Device Type: %d", (int)device.getDeviceType());
            return ERROR_INVALID_ARG;
    }

    // Validation
    RETURN_IF_ERROR(ep_handle == nullptr, ERROR_FAIL, SYS_LOG_E("Failed to create endpoint"));

    // Final Setup: Tell the device object what its new Endpoint ID is
    uint16_t endpoint_id = endpoint::get_id(ep_handle);
    device.setEndpointId(endpoint_id);

    SYS_LOG_I("Device Registered: Type=%d, EndpointID=%d", (int)device.getDeviceType(), endpoint_id);

    return ERROR_SUCCESS;
}

sys_error_t MatterWrapper::updateLocalState(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, MatterTypes::AttributeValue_t* value)
{
    // Find the endpoint
    esp_matter::endpoint_t* ep_handle = endpoint::get(endpoint_id);
    RETURN_IF_ERROR(ep_handle == nullptr, ERROR_INVALID_ARG, SYS_LOG_E("Invalid Endpoint ID: %d", endpoint_id));

    // Convert to ESP Matter value
    esp_matter_attr_val_t esp_value = convert_to_esp_value(value);

    // Update the attribute value in the stack
    attribute::update(endpoint_id, cluster_id, attribute_id, &esp_value);

    SYS_LOG_I("Local state updated: EndpointID=%d, ClusterID=0x%08X, AttributeID=0x%08X", endpoint_id, cluster_id, attribute_id);

    return ERROR_SUCCESS;
}