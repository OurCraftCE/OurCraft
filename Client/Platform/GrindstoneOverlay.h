#pragma once
#include "ContainerOverlay.h"

class GrindstoneOverlay : public ContainerOverlay {
public:
    static GrindstoneOverlay& Get() { static GrindstoneOverlay i; return i; }
protected:
    void OnTick() override;
};
