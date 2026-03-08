#pragma once
#include "ContainerOverlay.h"

class AnvilOverlay : public ContainerOverlay {
public:
    static AnvilOverlay& Get() { static AnvilOverlay i; return i; }
protected:
    void OnTick() override;
};
