#pragma once
#include "ContainerOverlay.h"

class BeaconOverlay : public ContainerOverlay {
public:
    static BeaconOverlay& Get() { static BeaconOverlay i; return i; }
protected:
    void OnTick() override;
};
