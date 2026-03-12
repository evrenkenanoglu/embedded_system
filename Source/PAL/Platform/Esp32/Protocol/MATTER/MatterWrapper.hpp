#pragma once
#include "PAL/Protocols/MATTER/IMatterStack.hpp"
#include "PAL/Protocols/MATTER/MatterDevice.hpp"
#include <esp_matter.h>
class MatterWrapper : public IMatterStack
{
public:
    // Singleton Accessor
    static MatterWrapper& get();

    sys_error_t init() override;
    sys_error_t start() override;
    sys_error_t addDevice(IMatterDevice& device) override;

    // Internal: Routes generic ESP events to specific C++ objects
    static esp_err_t onAttributeCallback(
        esp_matter::attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t* val, void* priv_data);

    static esp_err_t
    identificationCallback(esp_matter::identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id, uint8_t effect_variant, void* priv_data);

    sys_error_t updateLocalState(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, MatterTypes::AttributeValue_t* value) override;

private:
    MatterWrapper();
    esp_matter::node_t* _node_handle;
};