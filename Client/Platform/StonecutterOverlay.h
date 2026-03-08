#pragma once
#include "ContainerOverlay.h"

class StonecutterOverlay : public ContainerOverlay {
public:
    static StonecutterOverlay& Get() { static StonecutterOverlay i; return i; }
protected:
    void OnTick() override;
};
