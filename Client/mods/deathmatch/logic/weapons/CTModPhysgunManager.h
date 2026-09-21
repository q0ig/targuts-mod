#pragma once

#include <CVector.h>

class btRigidBody;
class btPoint2PointConstraint;

class CTModPhysgunManager {
public:
    static CTModPhysgunManager& GetSingleton() {
        static CTModPhysgunManager instance;
        return instance;
    }

    void Init();
    void Shutdown();
    void DoPulse();

private:
    CTModPhysgunManager() = default;

    bool m_bActive = false;
    btRigidBody* m_pGrabbedBody = nullptr;
    btPoint2PointConstraint* m_pConstraint = nullptr;
    float m_fGrabDistance = 0.0f;
    float m_fTargetDistance = 0.0f;
};
