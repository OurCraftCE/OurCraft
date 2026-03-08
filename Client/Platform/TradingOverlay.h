#pragma once
#include "ContainerOverlay.h"

class TradingOverlay : public ContainerOverlay {
public:
    static TradingOverlay& Get() { static TradingOverlay i; return i; }
protected:
    void OnTick() override;
};
