#pragma once
#include "ContainerOverlay.h"

class SmithingOverlay : public ContainerOverlay {
public:
    static SmithingOverlay& Get() { static SmithingOverlay i; return i; }
protected:
    void OnTick() override;
};
