#pragma once
#include "ContainerOverlay.h"

class CreativeOverlay : public ContainerOverlay {
public:
    static CreativeOverlay& Get() { static CreativeOverlay i; return i; }
    void ShowCreative();
protected:
    void OnTick() override;
};
