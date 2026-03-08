#pragma once
#include "ContainerOverlay.h"

class EnchantingOverlay : public ContainerOverlay {
public:
    static EnchantingOverlay& Get() { static EnchantingOverlay i; return i; }
protected:
    void OnTick() override;
};
