#pragma once
#include "ContainerOverlay.h"

class CartographyOverlay : public ContainerOverlay {
public:
    static CartographyOverlay& Get() { static CartographyOverlay i; return i; }
protected:
    void OnTick() override;
};
